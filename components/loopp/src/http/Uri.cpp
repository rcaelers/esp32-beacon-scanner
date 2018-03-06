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

#include "loopp/http/Uri.hpp"

#include "boost_xtensa.hpp"

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "loopp/http/HttpErrors.hpp"

using namespace std;
using namespace loopp::http;

namespace
{
  inline unsigned char to_hex(unsigned char c)
  {
    return c + (c > 9 ? ('A' - 10) : '0');
  }
} // namespace

const std::string
Uri::escape(const std::string &in)
{
  std::string ret;
  for (unsigned char ch : in)
    {
      if ((isalnum(ch) != 0) || (ch == '-') || (ch == '_') || (ch == '.') || (ch == '~'))
        {
          ret += ch;
        }
      else if (ch == ' ')
        {
          ret += '+';
        }
      else
        {
          ret += '%';
          ret += to_hex(ch >> 4);
          ret += to_hex(ch % 16);
        }
    }
  return ret;
}

const std::string
Uri::unescape(const std::string &in)
{
  string ret = in;
  // TODO:
  return ret;
}

void
Uri::parse(const std::string &uri)
{
  const string uri_regex = "((?<scheme>[^:/?#]+)://)?"
                           "((?<username>[^:@]+)(:(?<password>[^@]+))?@)?"
                           "(?<host>[a-zA-Z0-9.-]*)"
                           "(:(?<port>[\\d]{2,5}))?"
                           "(?<fullpath>"
                           "(?<path>[/\\\\][^?#]*)?"
                           "(\\?(?<query>[^#]*))?"
                           "(#(?<fragment>.*))?"
                           ")";

  boost::regex re(uri_regex, boost::regex::icase);

  boost::smatch matches;
  bool found = boost::regex_match(uri, matches, re);
  if (!found)
    {
      throw std::system_error(HttpErrc::InvalidURI, "URI invalid");
    }

  scheme_ = matches["scheme"];
  if (scheme_.empty())
    {
      scheme_ = "http";
    }

  username_ = matches["username"];
  password_ = matches["password"];

  host_ = matches["host"];
  std::string portstr = matches["port"];
  if (!portstr.empty())
    {
      port_ = boost::lexical_cast<uint16_t>(portstr);
    }
  else
    {
      port_ = (scheme_ == "https") ? 443 : 80;
    }

  path_ = matches["path"];
  query_ = matches["query"];
  fragment_ = matches["fragment"];
  fullpath_ = matches["fullpath"];

  if (!query_.empty())
    {
      vector<string> query_params;
      boost::split(query_params, query_, boost::is_any_of("&"));

      for (auto & query_param : query_params)
        {
          vector<string> param_elements;
          boost::split(param_elements, query_param, boost::is_any_of("="));

          if (param_elements.size() == 2)
            {
              query_parameters_[param_elements[0]] = param_elements[1];
            }
        }
    }
}

namespace loopp
{
  namespace http
  {
    std::ostream &operator<<(std::ostream &stream, const Uri &uri)
    {
      stream << "[URI scheme = " << uri.scheme() << " "
             << "username = " << uri.username() << " "
             << "password = " << uri.password() << " "
             << "port = " << uri.port() << " "
             << "path = " << uri.path() << " "
             << "fragment = " << uri.fragment() << " "
             << "query = " << uri.query() << " "
             << "fullpath = " << uri.fullpath() << "]";
      return stream;
    }
  } // namespace http
} // namespace loopp
