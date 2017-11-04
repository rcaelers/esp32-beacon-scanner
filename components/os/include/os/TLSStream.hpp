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

#ifndef OS_TLSSTREAM_HPP
#define OS_TLSSTREAM_HPP

#include <string>
#include <system_error>

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

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "os/Mutex.hpp"
#include "os/Callback.hpp"
#include "os/Property.hpp"
#include "os/Resolver.hpp"
#include "os/Slot.hpp"
#include "os/Buffer.hpp"

#include "esp_log.h"

namespace os
{
  class TLSStream : public std::enable_shared_from_this<TLSStream>
  {
  public:
    using connect_function_t = void (std::error_code ec);
    using io_function_t = void (std::error_code ec, std::size_t bytes_transferred);
    using connect_callback_t = std::function<connect_function_t>;
    using io_callback_t = std::function<io_function_t>;
    using connect_slot_t = os::Slot<connect_function_t>;
    using io_slot_t = os::Slot<io_function_t>;

    TLSStream();
    ~TLSStream();

    void set_client_certificate(const char *cert, const char *key);
    void set_ca_certificate(const char *cert);
    void connect(std::string host, int port, connect_callback_t callback);
    void connect(std::string host, int port, connect_slot_t slot);
    void write_async(Buffer buf, io_callback_t callback);
    void write_async(Buffer buf, io_slot_t slot);
    void read_async(Buffer buf, std::size_t count, io_callback_t callback);
    void read_async(Buffer buf, std::size_t count, io_slot_t slot);
    void close();

    os::Property<bool> &connected();

  private:
    void init_entropy();
    void parse_cert(mbedtls_x509_crt *crt, const char *cert_str);
    void parse_key(mbedtls_pk_context *key, const char *key_str);
    void log_failure(std::string msg, int error_code);
    void throw_if_failure(std::string msg, int error_code);
    void throw_if_failure(std::string msg, std::error_code ec);

    void on_resolved(std::string host, struct addrinfo *res, connect_slot_t slot);
    void on_connected(connect_slot_t slot);
    void on_handshake(connect_slot_t slot);

    void do_wait_write_async();
    void do_write_async();
    void do_read_async(Buffer buf, std::size_t count, std::size_t bytes_transferred, io_slot_t slot);

    static int verify_certificate(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags);

  private:
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config config;
    mbedtls_x509_crt ca_crt;
    mbedtls_x509_crt client_crt;
    mbedtls_pk_context client_key;

    os::Resolver &resolver;
    std::shared_ptr<os::MainLoop> loop;
    os::Property<bool> connected_property { false };

    class WriteOperation
    {
    public:
      WriteOperation(Buffer buffer, io_slot_t slot)
        : buffer_(std::move(buffer)), slot_(std::move(slot))
      {
      }

      Buffer &buffer()
      {
        return buffer_;
      }

      int bytes_transferred() const
      {
        return bytes_transferred_;
      }

      void advance(int bytes)
      {
        bytes_transferred_ += bytes;
      }

      bool done() const
      {
        return buffer_.size() >= bytes_transferred_;
      }

      void call(std::error_code ec)
      {
        return slot_.call(ec, bytes_transferred_);
      }

    private:
      Buffer buffer_;
      int bytes_transferred_ = 0;
      io_slot_t slot_;
    };

    std::deque<WriteOperation> write_op_queue;
  };
}

#endif // OS_TLSSTREAM_HPP
