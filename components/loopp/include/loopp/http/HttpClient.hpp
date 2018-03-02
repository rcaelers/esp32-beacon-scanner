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

#ifndef LOOPP_HTTP_CLIENT_HPP
#define LOOPP_HTTP_CLIENT_HPP

#include <string>
#include <memory>
#include <list>
#include <system_error>

#include "loopp/net/Stream.hpp"
#include "loopp/http/Request.hpp"
#include "loopp/http/Response.hpp"

namespace loopp
{
  namespace http
  {
    class HttpClient : public std::enable_shared_from_this<HttpClient>
    {
    public:
      using request_complete_function_t = void(std::error_code, Response);
      using request_complete_callback_t = std::function<request_complete_function_t>;

      using body_function_t = void(std::error_code, loopp::net::StreamBuffer *buffer);
      using body_callback_t = std::function<body_function_t>;

      HttpClient(std::shared_ptr<loopp::core::MainLoop> loop);
      ~HttpClient() = default;

      void set_client_certificate(const char *cert, const char *key);
      void set_ca_certificate(const char *cert);
      void execute(Request request, request_complete_callback_t callback);
      void read_body_async(std::size_t size, body_callback_t callback);

      std::size_t get_body_length() const { return body_length; }
      std::size_t get_body_length_left() const { return body_length_left; }

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
      std::shared_ptr<loopp::core::MainLoop> loop;
      std::shared_ptr<loopp::net::Stream> sock;
      loopp::http::Request request;
      loopp::http::Response response;
      request_complete_callback_t complete_callback;

      bool keep_alive = false;
      std::size_t body_length = 0;
      std::size_t body_length_left = 0;

      loopp::net::StreamBuffer request_buffer;
      loopp::net::StreamBuffer response_buffer;

      const char *client_cert = nullptr;
      const char *client_key = nullptr;
      const char *ca_cert = nullptr;
    };
  }
}

#endif // LOOPP_HTTP_CLIENT_HPP
