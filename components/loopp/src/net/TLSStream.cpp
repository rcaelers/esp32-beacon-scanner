// Copyright (C) 2017, 2018 Rob Caelers <rob.caelers@gmail.com>
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

#include "loopp/net/TLSStream.hpp"

#include <string>
#include <system_error>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "loopp/net/NetworkErrors.hpp"

#include "lwip/sockets.h"
#include "lwip/sys.h"

extern "C"
{
#include "esp_heap_caps.h"
#include "mbedtls/esp_debug.h"
}

#include "esp_log.h"

static const char *tag = "NET";

using namespace loopp;
using namespace loopp::net;

TLSStream::TLSStream(std::shared_ptr<loopp::core::MainLoop> loop)
  : Stream(loop)
  , loop(loop)
{
  mbedtls_net_init(&server_fd);
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&config);

  mbedtls_x509_crt_init(&client_crt);
  mbedtls_x509_crt_init(&ca_crt);
  mbedtls_pk_init(&client_key);

#ifdef CONFIG_MBEDTLS_DEBUG
  mbedtls_esp_enable_debug_log(&config, 4);
#endif

  init_entropy();
}

TLSStream::~TLSStream()
{
  if (server_fd.fd != -1)
    {
      ::close(server_fd.fd);
      loop->unnotify(server_fd.fd);
    }

  mbedtls_net_free(&server_fd);
  mbedtls_x509_crt_free(&client_crt);
  mbedtls_x509_crt_free(&ca_crt);
  mbedtls_pk_free(&client_key);
  mbedtls_ssl_config_free(&config);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
  mbedtls_ssl_free(&ssl);
}

void
TLSStream::set_client_certificate(const char *cert, const char *key)
{
  assert(cert != nullptr && key != nullptr);
  parse_cert(&client_crt, cert);
  parse_key(&client_key, key);
  have_client_cert = true;
}

void
TLSStream::set_ca_certificate(const char *cert)
{
  assert(cert != nullptr);
  parse_cert(&ca_crt, cert);
  have_ca_cert = true;
}

void
TLSStream::init_entropy()
{
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, nullptr, 0);
  throw_if_failure("mbedtls_ctr_drbg_seed", ret);
}

void
TLSStream::parse_cert(mbedtls_x509_crt *crt, const char *cert_str)
{
  mbedtls_x509_crt_init(crt);
  int ret = mbedtls_x509_crt_parse(crt, reinterpret_cast<const unsigned char *>(cert_str), strlen(cert_str) + 1);
  throw_if_failure("mbedtls_x509_crt_parse", ret);
}

void
TLSStream::parse_key(mbedtls_pk_context *key, const char *key_str)
{
  mbedtls_pk_init(key);
  int ret = mbedtls_pk_parse_key(key,
                                 reinterpret_cast<const unsigned char *>(key_str),
                                 strlen(key_str) + 1,
                                 reinterpret_cast<const unsigned char *>(""),
                                 0);
  throw_if_failure("mbedtls_x509_crt_parse", ret);
}

int
TLSStream::socket_read(uint8_t *buffer, std::size_t count)
{
  int ret = mbedtls_ssl_read(&ssl, buffer, count);

  if (ret == MBEDTLS_ERR_SSL_WANT_READ)
    {
      ret = -EAGAIN;
    }

  return ret;
}

int
TLSStream::socket_write(uint8_t *buffer, std::size_t count)
{
  int ret = mbedtls_ssl_write(&ssl, buffer, count);

  if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
      ret = -EAGAIN;
    }

  return ret;
}

void
TLSStream::socket_close()
{
  int ret = 0;
  do
    {
      ret = mbedtls_ssl_close_notify(&ssl);
    }
  while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);
  ::close(get_socket());
}

void
TLSStream::socket_on_connected(const std::string &host, const connect_callback_t &callback)
{
  ESP_LOGD(tag, "Connected. Starting handshake");

  server_fd.fd = get_socket();

  try
    {
      int ret = mbedtls_ssl_set_hostname(&ssl, host.c_str());
      throw_if_failure("mbedtls_ssl_set_hostname", ret);

      ret = mbedtls_ssl_config_defaults(&config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
      throw_if_failure("mbedtls_ssl_config_defaults", ret);

      mbedtls_ssl_conf_verify(&config, verify_certificate, nullptr);
      mbedtls_ssl_conf_rng(&config, mbedtls_ctr_drbg_random, &ctr_drbg);
      mbedtls_ssl_conf_authmode(&config, have_ca_cert ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_OPTIONAL);
      if (have_ca_cert)
        {
          ESP_LOGD(tag, "Using CA cert");
          mbedtls_ssl_conf_ca_chain(&config, &ca_crt, nullptr);
        }
      if (have_client_cert)
        {
          ESP_LOGD(tag, "Using Client cert");
          ret = mbedtls_ssl_conf_own_cert(&config, &client_crt, &client_key);
          throw_if_failure("mbedtls_ssl_conf_own_cert", ret);
        }
      mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, nullptr);

      ret = mbedtls_ssl_setup(&ssl, &config);
      throw_if_failure("mbedtls_ssl_setup", ret);

      on_handshake(callback);
    }
  catch (const std::system_error &ex)
    {
      callback(ex.code());
    }
}

void
TLSStream::on_handshake(const connect_callback_t &callback)
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
            loop->notify_read(server_fd.fd, [this, self, callback](std::error_code ec) {
              if (!ec)
                {
                  on_handshake(callback);
                }
              else
                {
                  callback(ec);
                }
            });
            break;

          case MBEDTLS_ERR_SSL_WANT_WRITE:
            loop->notify_write(server_fd.fd, [this, self, callback](std::error_code ec) {
              if (!ec)
                {
                  on_handshake(callback);
                }
              else
                {
                  callback(ec);
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

          if (mbedtls_ssl_get_peer_cert(&ssl) != nullptr)
            {
              char buf[512];
              mbedtls_x509_crt_info(static_cast<char *>(buf), sizeof(buf) - 1, "", mbedtls_ssl_get_peer_cert(&ssl));
              ESP_LOGD(tag, "Peer certificate information: %s", buf);
            }

          connected_property.set(true);
          callback(std::error_code());
        }
    }
  catch (const std::system_error &ex)
    {
      callback(ex.code());
    }
}

void
TLSStream::log_failure(const std::string &msg, int error_code)
{
  if (error_code != 0)
    {
      char error_msg[512];
      mbedtls_strerror(error_code, error_msg, sizeof(error_msg));
      ESP_LOGE(tag, "Error: %s %s %d", msg.c_str(), error_msg, error_code);
    }
}

void
TLSStream::throw_if_failure(const std::string &msg, int error_code)
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

int
TLSStream::verify_certificate(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
  char buf[1024];
  (void)data;
  (void)flags;

  ESP_LOGD(tag, "Verify requested for (depth %d):", depth);
  mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
  ESP_LOGD(tag, "%s", buf);

  return 0;
}
