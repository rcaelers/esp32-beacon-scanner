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

#ifndef LOOPP_NET_STREAM_HPP
#define LOOPP_NET_STREAM_HPP

#include <string>
#include <system_error>
#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "loopp/core/MainLoop.hpp"
#include "loopp/core/Property.hpp"
#include "loopp/net/Resolver.hpp"
#include "loopp/net/StreamBuffer.hpp"

namespace loopp
{
  namespace net
  {
    class Stream : public std::enable_shared_from_this<Stream>
    {
    public:
      using connect_function_t = void(std::error_code ec);
      using io_function_t = void(std::error_code ec, std::size_t bytes_transferred);
      using connect_callback_t = std::function<connect_function_t>;
      using io_callback_t = std::function<io_function_t>;

      Stream(std::shared_ptr<loopp::core::MainLoop> loop);
      virtual ~Stream();

      void connect(std::string host, int port, connect_callback_t callback);
      void write_async(StreamBuffer &buf, io_callback_t callback);
      void read_async(StreamBuffer &buf, std::size_t count, io_callback_t callback);
      void read_until_async(StreamBuffer &buf, std::string until, io_callback_t callback);
      void close();

      loopp::core::Property<bool> &connected();

    protected:
      virtual int socket_read(uint8_t *buffer, std::size_t count) = 0;
      virtual int socket_write(uint8_t *buffer, std::size_t count) = 0;
      virtual void socket_close() = 0;
      virtual void socket_on_connected(std::string host, connect_callback_t callback) = 0;

      int
      get_socket() const
      {
        return sock;
      }

    private:
      void log_failure(std::string msg, int error_code);
      void on_resolved(std::string host, struct addrinfo *res, connect_callback_t callback);
      void do_wait_write_async();
      void do_write_async();
      void do_read_async(StreamBuffer &buf, std::size_t count, std::size_t bytes_transferred, io_callback_t callback);
      void do_read_until_async(StreamBuffer &buf, std::string until, std::size_t bytes_transferred, io_callback_t callback);
      bool match_until(StreamBuffer &buf, std::size_t &start_pos, std::string match);

    protected:
      loopp::core::Property<bool> connected_property{ false };

    private:
      int sock;
      std::shared_ptr<loopp::core::MainLoop> loop;
      loopp::net::Resolver &resolver;

      class WriteOperation
      {
      public:
        WriteOperation(StreamBuffer &buffer, io_callback_t callback)
          : buffer_(buffer)
          , callback_(std::move(callback))
        {
        }

        StreamBuffer &
        buffer()
        {
          return buffer_;
        }

        void
        call(std::error_code ec, std::size_t bytes_transferred)
        {
          return callback_(ec, bytes_transferred);
        }

      private:
        StreamBuffer &buffer_;
        io_callback_t callback_;
      };

      std::deque<WriteOperation> write_op_queue;
    };
  } // namespace net
} // namespace loopp

#endif // LOOPP_NET_STREAM_HPP
