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

#ifndef OS_TLSSTREAM_HPP
#define OS_TLSSTREAM_HPP

#include <string>
#include <system_error>

#include "Stream.hpp"

#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"

namespace os
{
  class TLSStream : public Stream
  {
  public:
    TLSStream(std::shared_ptr<os::MainLoop> loop);
    virtual ~TLSStream();

    void set_client_certificate(const char *cert, const char *key);
    void set_ca_certificate(const char *cert);

  private:
    void init_entropy();
    void parse_cert(mbedtls_x509_crt *crt, const char *cert_str);
    void parse_key(mbedtls_pk_context *key, const char *key_str);
    void throw_if_failure(std::string msg, int error_code);
    void on_handshake(connect_slot_t slot);
    void log_failure(std::string msg, int error_code);

    virtual int socket_read(uint8_t *buffer, std::size_t count);
    virtual int socket_write(uint8_t *buffer, std::size_t count);
    virtual void socket_close();
    virtual void socket_on_connected(std::string host, connect_slot_t slot);

    static int verify_certificate(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags);

  private:
    std::shared_ptr<os::MainLoop> loop;
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config config;
    mbedtls_x509_crt ca_crt;
    mbedtls_x509_crt client_crt;
    mbedtls_pk_context client_key;
  };
}

#endif // OS_TLSSTREAM_HPP
