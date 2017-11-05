// Copyright (C) 2017 Rob Caelers <rob.caelers@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GLIBCXX_USE_C99

#include "os/TLSStream.hpp"

#include <string>
#include <system_error>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include "os/Resolver.hpp"
#include "os/NetworkErrors.hpp"

#include "lwip/sockets.h"
#include "lwip/sys.h"

extern "C"
{
#include "mbedtls/esp_debug.h"
}

#include "esp_log.h"

static const char tag[] = "NET";

using namespace os;

TLSStream::TLSStream()
  : resolver(Resolver::instance())
{
  mbedtls_net_init(&server_fd);
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&config);

#ifdef CONFIG_MBEDTLS_DEBUG
  mbedtls_esp_enable_debug_log(&config, 4);
#endif

  init_entropy();
}

TLSStream::~TLSStream()
{
  if (server_fd.fd != -1)
    {
      loop->unnotify(server_fd.fd);
    }

  mbedtls_net_free(&server_fd);
  mbedtls_x509_crt_free(&client_crt);
  mbedtls_x509_crt_free(&ca_crt);
  mbedtls_pk_free(&client_key);
  mbedtls_ssl_config_free(&config);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
}

void
TLSStream::set_client_certificate(const char *cert, const char *key)
{
  parse_cert(&client_crt, cert);
  parse_key(&client_key, key);
}

void
TLSStream::set_ca_certificate(const char *cert)
{
  parse_cert(&ca_crt, cert);
}

void
TLSStream::init_entropy()
{
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
  throw_if_failure("mbedtls_ctr_drbg_seed", ret);
}

void
TLSStream::parse_cert(mbedtls_x509_crt *crt, const char *cert_str)
{
  mbedtls_x509_crt_init(crt);
  int ret = mbedtls_x509_crt_parse(crt, (const unsigned char *)cert_str, strlen(cert_str) + 1);
  throw_if_failure("mbedtls_x509_crt_parse", ret);
}

void
TLSStream::parse_key(mbedtls_pk_context *key, const char *key_str)
{
  mbedtls_pk_init(key);
  int ret = mbedtls_pk_parse_key(key, (const unsigned char *)key_str, strlen(key_str)+1, (const unsigned char *)"", 0);
  throw_if_failure("mbedtls_x509_crt_parse", ret);
}

void
TLSStream::connect(std::string host, int port, connect_callback_t callback)
{
  loop = MainLoop::current();
  assert(loop != nullptr && "TLSStream can only be used in MainLoop thread");

  connect(std::move(host), port, os::make_slot(loop, callback));
}

void
TLSStream::connect(std::string host, int port, connect_slot_t slot)
{
  ESP_LOGI(tag, "Connecting to %s:%s", host.c_str(), std::to_string(port).c_str());

  loop = MainLoop::current();
  assert(loop != nullptr && "TLSStream can only be used in MainLoop thread");

  auto self = shared_from_this();
  resolver.resolve_async(host, std::to_string(port), os::make_slot(loop, [this, self, host, slot] (std::error_code ec, struct addrinfo *addr_list) {
        if (!ec)
          {
            on_resolved(host, addr_list, slot);
          }
        else
          {
            slot.call(ec);
          }

        if (addr_list != NULL)
          {
            freeaddrinfo(addr_list);
          }
      }));
}

void
TLSStream::on_resolved(std::string host, struct addrinfo *addr_list, connect_slot_t slot)
{
  try
    {
      struct addrinfo *addr = nullptr;

      // TODO: try next one on failure.
      for (struct addrinfo *cur = addr_list; cur != nullptr &&  addr == nullptr; cur = cur->ai_next)
        {
          if (cur->ai_family == AF_INET || cur->ai_family == AF_INET6)
            {
              addr = cur;
            }
        }

      if (addr == nullptr)
        {
          throw std::system_error(NetworkErrc::NameResolutionFailed, "No suitable IP address");
        }

      int fd = socket(addr->ai_family, SOCK_STREAM, IPPROTO_TCP);
      if (fd < 0)
        {
          throw std::system_error(NetworkErrc::InternalError, "Could not create socket");
        }

      server_fd.fd = fd;

      int ret = mbedtls_net_set_nonblock(&server_fd);
      throw_if_failure("mbedtls_net_set_nonblock", ret);

      ret = lwip_connect(fd, addr->ai_addr, addr->ai_addrlen);
      ESP_LOGD(tag, "connect %s: ret = %x errno %d", host.c_str(), -ret, errno);

      if (ret < 0 && errno != EINPROGRESS)
        {
          lwip_close(fd);
          server_fd.fd = -1;
          // TODO: use errno
          throw std::system_error(NetworkErrc::InternalError, "Could not connect");
        }

      ret = mbedtls_ssl_set_hostname(&ssl, host.c_str());
      throw_if_failure("mbedtls_ssl_set_hostname", ret);

      auto self = shared_from_this();
      loop->notify_write(server_fd.fd, [this, self, slot](std::error_code ec) {
          if (!ec)
            {
              on_connected(slot);
            }
          else
            {
              slot.call(ec);
            }
        },
        // TODO: make configurable
        std::chrono::milliseconds(5000));
    }
  catch (const std::system_error &ex)
    {
      ESP_LOGD(tag, "connect exception %d %s", ex.code().value(), ex.what());
      slot.call(ex.code());
    }
}

void
TLSStream::on_connected(connect_slot_t slot)
{
  ESP_LOGD(tag, "Connected. Starting handshake");

  try
    {
      int ret = mbedtls_ssl_config_defaults(&config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
      throw_if_failure("mbedtls_ssl_config_defaults", ret);

      mbedtls_ssl_conf_verify(&config, verify_certificate, NULL);
      mbedtls_ssl_conf_rng(&config, mbedtls_ctr_drbg_random, &ctr_drbg);
      mbedtls_ssl_conf_authmode(&config, MBEDTLS_SSL_VERIFY_REQUIRED);
      mbedtls_ssl_conf_ca_chain(&config, &ca_crt, NULL);

      ret = mbedtls_ssl_conf_own_cert(&config, &client_crt, &client_key);
      throw_if_failure("mbedtls_ssl_conf_own_cert", ret);

      mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

      ret = mbedtls_ssl_setup(&ssl, &config);
      throw_if_failure("mbedtls_ssl_setup", ret);

      on_handshake(slot);
    }
  catch (const std::system_error &ex)
    {
      slot.call(ex.code());
    }
}

void
TLSStream::on_handshake(connect_slot_t slot)
{
  try
    {
      auto self = shared_from_this();

      int ret = mbedtls_ssl_handshake(&ssl);
      ESP_LOGD(tag, "Handshake ret = %x", -ret);
      switch (ret)
        {
        case 0:
          ESP_LOGD(tag, "TLS Handshake complete");
          break;

        case MBEDTLS_ERR_SSL_WANT_READ:
          loop->notify_read(server_fd.fd, [this, self, slot](std::error_code ec) {
              if (!ec)
                {
                  on_handshake(slot);
                }
              else
                {
                  slot.call(ec);
                }
            });
          break;

        case MBEDTLS_ERR_SSL_WANT_WRITE:
          loop->notify_write(server_fd.fd, [this, self, slot](std::error_code ec) {
              if (!ec)
                {
                  on_handshake(slot);
                }
              else
                {
                  slot.call(ec);
                }
            });
          break;

        default:
          ESP_LOGE(tag, "TLS Handshake failed: %x", -ret);
          throw_if_failure("mbedtls_ssl_handshake", ret);
        }

      if (ret == 0)
        {
          ret = mbedtls_ssl_get_verify_result(&ssl);
          if (ret != 0)
            {
              char buf[512];
              mbedtls_x509_crt_verify_info(buf, sizeof(buf), "", ret);
              ESP_LOGE(tag, "TLS Verify failed: %s", buf);
            }

          if (mbedtls_ssl_get_peer_cert(&ssl) != NULL)
            {
              char buf[512];
              mbedtls_x509_crt_info((char *) buf, sizeof(buf) - 1, "", mbedtls_ssl_get_peer_cert(&ssl));
              ESP_LOGD(tag, "Peer certificate information: %s", buf);
            }

          connected_property.set(true);
          slot.call(std::error_code());
        }
    }
  catch (const std::system_error &ex)
    {
      slot.call(ex.code());
    }
}

void
TLSStream::write_async(Buffer buf, io_callback_t callback)
{
  loop = MainLoop::current();
  assert(loop != nullptr && "TLSStream can only be used in MainLoop thread");

  write_async(std::move(buf), os::make_slot(loop, callback));
}

void
TLSStream::read_async(Buffer buf, std::size_t count, io_callback_t callback)
{
  loop = MainLoop::current();
  assert(loop != nullptr && "TLSStream can only be used in MainLoop thread");

  read_async(std::move(buf), count, os::make_slot(loop, callback));
}

void
TLSStream::write_async(Buffer buffer, io_slot_t slot)
{
  if (!connected_property.get())
    {
      throw std::runtime_error("Not connected");
    }

  auto self = shared_from_this();

  loop->post([this, self, buffer, slot] () {
      write_op_queue.emplace_back(buffer, slot);
      if (write_op_queue.size() == 1)
        {
          do_write_async();
        }
    });
}

void
TLSStream::do_wait_write_async()
{
  if (write_op_queue.size() > 0)
    {
      auto self = shared_from_this();
      loop->notify_write(server_fd.fd, [this, self](std::error_code ec) {
          if (!ec)
            {
              do_write_async();
            }
          else
            {
              auto &write_op = write_op_queue.front();
              write_op.call(ec);
              write_op_queue.pop_front();
              do_wait_write_async();
            }
        });
    }
}

void
TLSStream::do_write_async()
{
  auto &write_op = write_op_queue.front();

  std::error_code ec;
  do
    {
      int ret = mbedtls_ssl_write(&ssl,
                                  write_op.buffer().data() + write_op.bytes_transferred(),
                                  write_op.buffer().size() - write_op.bytes_transferred());

      if (ret > 0)
        {
          write_op.advance(ret);
        }
      else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
          do_wait_write_async();
          return;
        }
      else
        {
          log_failure("mbedtls_ssl_write", ret);
          ec = NetworkErrc::WriteError;
        }
    }
  while (!ec && !write_op.done());

  write_op.call(ec);
  write_op_queue.pop_front();
  do_wait_write_async();
}

void
TLSStream::read_async(Buffer buffer, std::size_t count, io_slot_t slot)
{
  if (!connected_property.get())
    {
      throw std::runtime_error("Not connected");
    }
  do_read_async(buffer, count, 0, slot);
}

void
TLSStream::do_read_async(Buffer buf, std::size_t count, std::size_t bytes_transferred, io_slot_t slot)
{
  auto self = shared_from_this();
  std::error_code ec;

  do
    {
      int ret = mbedtls_ssl_read(&ssl, buf.data() + bytes_transferred, count - bytes_transferred);

      if (ret > 0)
        {
          bytes_transferred += ret;
        }
      else if (ret == 0)
        {
          ESP_LOGI(tag, "Connection closed");
          connected_property.set(false);
          ec = NetworkErrc::ConnectionClosed;
        }
      else if (ret == MBEDTLS_ERR_SSL_WANT_READ)
        {
          loop->notify_read(server_fd.fd, [this, self, buf, count, bytes_transferred, slot](std::error_code ec) {
              if (!ec)
                {
                  do_read_async(buf, count, bytes_transferred, slot);
                }
              else
                {
                  slot.call(ec, bytes_transferred);
                }
            });
          return;
        }
      else
        {
          log_failure("mbedtls_ssl_read", ret);
          ec = NetworkErrc::ReadError;
        }
    } while (!ec && (bytes_transferred < count));

  slot.call(ec, bytes_transferred);
}

// TODO: make async
void
TLSStream::close()
{
  loop->unnotify(server_fd.fd);
  connected_property.set(false);

  int ret = 0;
  do
    {
      ret = mbedtls_ssl_close_notify(&ssl);
    } while(ret == MBEDTLS_ERR_SSL_WANT_WRITE);
}

void
TLSStream::log_failure(std::string msg, int error_code)
{
  if (error_code != 0)
    {
      char error_msg[512];
      mbedtls_strerror(error_code, error_msg, sizeof(error_msg));
      ESP_LOGE(tag, "Error: %s %s %d", msg.c_str(), error_msg, error_code);
    }
}

void
TLSStream::throw_if_failure(std::string msg, int error_code)
{
  if (error_code != 0)
    {
      char error_msg[512];
      mbedtls_strerror(error_code, error_msg, sizeof(error_msg));
      ESP_LOGE(tag, "Error: %s %s %d", msg.c_str(), error_msg, error_code);

      // TODO: convert errors.
      std::error_code ec(NetworkErrc::TLSProtocolError);
      throw std::system_error(ec, msg);
    }
}

void
TLSStream::throw_if_failure(std::string msg, std::error_code ec)
{
  if (ec)
    {
      ESP_LOGE(tag, "Error: %s %s", msg.c_str(), ec.message().c_str());
      throw std::system_error(ec, msg);
    }
}

int
TLSStream::verify_certificate(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
  char buf[1024];
  ((void) data);

  ESP_LOGD(tag, "Verify requested for (depth %d):", depth);
  mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
  ESP_LOGD(tag, "%s", buf);

  return 0;
}

os::Property<bool> &
TLSStream::connected()
{
  return connected_property;

}
