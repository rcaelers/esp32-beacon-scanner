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

#ifndef LOOPP_HTTP_REQUEST_HPP
#define LOOPP_HTTP_REQUEST_HPP

#include <string>
#include <map>

#include "loopp/http/Headers.hpp"
#include "loopp/http/Uri.hpp"

namespace loopp
{
  namespace http
  {
    class Request
    {
    public:
      Request(const std::string &method, const std::string &uri)
        : method_(method)
        , uri_(uri)
      {
      }

      Request() = default;
      Request(const Request &other) = default;
      Request(Request &&other) = default;
      Request &operator=(const Request &other) = default;
      Request &operator=(Request &&other) = default;

      ~Request() = default;

      void uri(std::string u)
      {
        uri_ = Uri(u);
      }

      Uri &uri()
      {
        return uri_;
      }

      const Uri &uri() const
      {
        return uri_;
      }

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

      std::string scheme() const
      {
        return uri_.scheme();
      }

      std::string path() const
      {
        return uri_.path();
      }

      void content(std::string content)
      {
        content_ = content;
      }

      void append_content(std::string content)
      {
        content_ += content;
      }

      std::string content() const
      {
        return content_;
      }

      Headers &headers()
      {
        return headers_;
      }

      const Headers &headers() const
      {
        return headers_;
      }

      friend std::ostream &operator<<(std::ostream &stream, const Request &request);

    private:
      std::string method_;
      std::string http_version_;
      Uri uri_;
      Headers headers_;
      std::string content_;
    };

    std::ostream &operator<<(std::ostream &stream, const Request &request);

  } // namespace http
} // namespace loopp

#endif // LOOPP_HTTP_REQUEST_HPP
