// Copyright (C) 2015 Rob Caelers <rob.caelers@gmail.com>
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

#ifndef LOOPP_HTTP_RESPONSE_HPP
#define LOOPP_HTTP_RESPONSE_HPP

#include <string>

#include "loopp/http/Headers.hpp"

namespace loopp
{
  namespace http
  {
    class Response
    {
    public:
      Response() = default;
      Response(const Response &other) = default;
      Response(Response &&other) = default;
      Response &operator=(const Response &other) = default;
      ~Response() = default;

      std::string method() const
      {
        return method_;
      }

      void method(const std::string &method)
      {
        method_ = method;
      }

      std::string http_version() const
      {
        return http_version_;
      }

      void http_version(const std::string &http_version)
      {
        http_version_ = http_version;
      }

      int status_code() const
      {
        return status_code_;
      }

      void status_code(int status_code)
      {
        status_code_ = status_code;
      }

      std::string status_message() const
      {
        return status_message_;
      }

      void status_message(const std::string &status_message)
      {
        status_message_ = status_message;
      }

      Headers &headers()
      {
        return headers_;
      }

      const Headers &headers() const
      {
        return headers_;
      }

      friend std::ostream &operator<<(std::ostream &stream, const Response &response);

    private:
      std::string method_;
      std::string http_version_;
      int status_code_ = 0;
      std::string status_message_;
      Headers headers_;
    };
  } // namespace http
} // namespace loopp

#endif // LOOPP_HTTP_RESPONSE_HPP
