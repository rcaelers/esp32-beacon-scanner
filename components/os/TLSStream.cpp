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

#include <string.h>

#include <string>
#include <system_error>

#include "os/TLSStream.hpp"

extern "C"
{
#include "mbedtls/esp_debug.h"
}

#include "esp_log.h"

static const char tag[] = "NET";

using namespace os;

TLSStream::TLSStream() :
  handshake_timeout(std::chrono::milliseconds(10000)),
  read_timeout(std::chrono::milliseconds(500)),
  write_timeout(std::chrono::milliseconds(500))
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
      std::shared_ptr<MainLoop> loop = MainLoop::current();
      assert(loop != nullptr && "TLSStream can only be used in MainLoop thread");
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

// TODO:make connect asynchronous
void
TLSStream::connect(const std::string &hostname, int port)
{
  ESP_LOGD(tag, "Connecting to %s:%s", hostname.c_str(), std::to_string(port).c_str());

  int ret = mbedtls_net_connect(&server_fd, hostname.c_str(), std::to_string(port).c_str(), MBEDTLS_NET_PROTO_TCP);
  throw_if_failure("mbembedtls_net_connecd", ret);

  ret = mbedtls_net_set_block(&server_fd);
  throw_if_failure("mbedtls_net_set_block", ret);

  ret = mbedtls_ssl_config_defaults(&config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
  throw_if_failure("mbedtls_ssl_config_defaults", ret);

  mbedtls_ssl_conf_verify(&config, verify_certificate, NULL);
  mbedtls_ssl_conf_rng(&config, mbedtls_ctr_drbg_random, &ctr_drbg);
  mbedtls_ssl_conf_authmode(&config, MBEDTLS_SSL_VERIFY_REQUIRED);
  mbedtls_ssl_conf_ca_chain(&config, &ca_crt, NULL);
  mbedtls_ssl_conf_read_timeout(&config, static_cast<uint32_t>(handshake_timeout.count()));

  ret = mbedtls_ssl_conf_own_cert(&config, &client_crt, &client_key);
  throw_if_failure("mbedtls_ssl_conf_own_cert", ret);

  ret = mbedtls_ssl_set_hostname(&ssl, hostname.c_str());
  throw_if_failure("mbedtls_ssl_set_hostname", ret);

  mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);

  ret = mbedtls_ssl_setup(&ssl, &config);
  throw_if_failure("mbedtls_ssl_setup", ret);

  while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
    {
      if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
          throw_if_failure("mbedtls_ssl_handshake", ret);
        }
    }

  ret = mbedtls_ssl_get_verify_result(&ssl);
  if (ret != 0)
  {
    char buf[512];
    mbedtls_x509_crt_verify_info(buf, sizeof(buf), "", ret);
    ESP_LOGE(tag, "Verify failed: %s", buf);
  }

  if (mbedtls_ssl_get_peer_cert(&ssl) != NULL)
    {
      char buf[512];
      mbedtls_x509_crt_info((char *) buf, sizeof(buf) - 1, "", mbedtls_ssl_get_peer_cert(&ssl));
      ESP_LOGD(tag, "Peer certificate information: %s", buf);
    }

  mbedtls_ssl_conf_read_timeout(&config, static_cast<uint32_t>(read_timeout.count()));
  ret = mbedtls_net_set_nonblock(&server_fd);
  throw_if_failure("mbedtls_net_set_nonblock", ret);

  std::shared_ptr<MainLoop> loop = MainLoop::current();
  assert(loop != nullptr && "TLSStream can only be used in MainLoop thread");
  loop->notify_read(server_fd.fd, [&]() { on_readable(); });
}

void
TLSStream::read(unsigned char *data, size_t count, size_t &bytes_read)
{
  bytes_read = 0;
  int ret = mbedtls_ssl_read(&ssl, data, count);

  if (ret > 0)
    {
      bytes_read = ret;
    }
  else if (ret == 0)
    {
      ESP_LOGI(tag, "Connection closed");
      close();
    }
  else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_TIMEOUT)
    {
      throw_if_failure("read", ret);
    }
}

void
TLSStream::write(unsigned char *data, size_t count, size_t &written)
{
  auto timeout = std::chrono::system_clock::now() + write_timeout;

  written = 0;
  do
    {
      int ret = mbedtls_ssl_write(&ssl, data + written, count - written);
      if (ret > 0)
        {
          written += ret;
        }
      else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
          throw_if_failure("write", ret);
        }
    }
  while (timeout < std::chrono::system_clock::now() && written < count);
}

void
TLSStream::close()
{
  std::shared_ptr<MainLoop> loop = MainLoop::current();
  assert(loop != nullptr && "TLSStream can only be used in MainLoop thread");
  loop->unnotify(server_fd.fd);

  int ret = 0;
  do
    {
      ret = mbedtls_ssl_close_notify(&ssl);
    } while(ret == MBEDTLS_ERR_SSL_WANT_WRITE);
}


void
TLSStream::throw_if_failure(std::string msg, int error_code)
{
  if (error_code != 0)
    {
      char error_msg[512];
      mbedtls_strerror(error_code, error_msg, sizeof(error_msg));

      ESP_LOGE(tag, "Error: %s %s %d", msg.c_str(), error_msg, error_code);
      throw std::runtime_error(msg);
    }
}

os::Callback<void()> &
TLSStream::io_callback()
{
  return callback_io;
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

void
TLSStream::on_readable()
{
  ESP_LOGD(tag, "on_readable");
  callback_io();
}

void
TLSStream::on_writable()
{
  ESP_LOGD(tag, "on_writable");
}
