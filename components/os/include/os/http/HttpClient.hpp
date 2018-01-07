// Copyright (C) 2015, 2018 Rob Caelers <rob.caelers@gmail.com>
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

#ifndef OS_HTTP_CLIENT_HPP
#define OS_HTTP_CLIENT_HPP

#include <string>
#include <memory>
#include <list>
#include <system_error>

#include "os/Stream.hpp"
#include "os/http/Request.hpp"
#include "os/http/Response.hpp"

namespace os
{
  namespace http
  {
    class HttpClient : public std::enable_shared_from_this<HttpClient>
    {
    public:
      using request_complete_function_t = void(std::error_code, Response);
      using request_complete_callback_t = std::function<request_complete_function_t>;
      using request_complete_slot_t = os::Slot<request_complete_function_t>;

      using body_function_t = void(std::error_code, os::StreamBuffer *buffer);
      using body_callback_t = std::function<body_function_t>;
      using body_slot_t = os::Slot<body_function_t>;

      HttpClient(std::shared_ptr<MainLoop> loop);
      ~HttpClient() = default;

      void set_client_certificate(const char *cert, const char *key);
      void set_ca_certificate(const char *cert);
      void execute(Request request, request_complete_slot_t slot);
      void read_body_async(std::size_t size, body_callback_t slot);
      void read_body_async(std::size_t size, body_slot_t slot);

    private:
      void send_request();
      void update_request_headers();
      void send_body();
      void read_response();
      void handle_response();
      void parse_status_line(std::istream &response_stream);
      void parse_headers(std::istream &response_stream);
      void handle_error(std::string what, std::error_code ec);

    private:
      std::shared_ptr<MainLoop> loop;
      std::shared_ptr<os::Stream> sock;
      os::http::Request request;
      os::http::Response response;
      request_complete_slot_t complete_slot;

      bool keep_alive = false;
      std::size_t body_length = 0;
      std::size_t body_length_left = 0;

      os::StreamBuffer request_buffer;
      os::StreamBuffer response_buffer;

      const char *client_cert = nullptr;
      const char *client_key = nullptr;
      const char *ca_cert = nullptr;
    };
  }
}

#endif // OS_HTTP_CLIENT_HPP
