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

#ifndef OS_HTTP_URI_HPP
#define OS_HTTP_URI_HPP

#include <string>
#include <map>

namespace os
{
  namespace http
  {
    class Uri
    {
    public:
      typedef std::map<std::string, std::string> QueryParams;

      Uri() = default;
      Uri(const Uri &other) = default;
      Uri(Uri &&other) = default;
      Uri &operator=(const Uri &other) = default;
      Uri &operator=(Uri &&other) = default;

      Uri(const std::string &uri)
      {
        parse(uri);
      }

      void set(const std::string &uri)
      {
        parse(uri);
      }

      std::string str() const
      {
        return uri_;
      }

      std::string scheme() const
      {
        return scheme_;
      }

      std::string host() const
      {
        return host_;
      }

      uint16_t  port() const
      {
        return port_;
      }

      std::string path() const
      {
        return path_;
      }

      std::string fragment() const
      {
        return fragment_;
      }

      std::string query() const
      {
        return query_;
      }

      std::string fullpath() const
      {
        return fullpath_;
      }

      std::string username() const
      {
        return username_;
      }

      std::string password() const
      {
        return password_;
      }

      QueryParams query_params() const
      {
        return query_parameters_;
      }

      static const std::string escape(const std::string &in);
      static const std::string unescape(const std::string &in);

      friend std::ostream& operator<< (std::ostream& stream, const Uri& uri);

    private:
      void parse(const std::string &uri);

    private:
      std::string uri_;
      std::string scheme_;
      std::string host_;
      uint16_t port_ = 0;
      std::string path_;
      std::string query_;
      std::string fragment_;
      std::string fullpath_;
      std::string username_;
      std::string password_;
      QueryParams query_parameters_;
    };
  }
}

#endif // OS_HTTP_URI_HPP
