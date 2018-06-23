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

#include "loopp/http/Headers.hpp"

#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/format.hpp>

using namespace loopp::http;

bool
Headers::has(const std::string &name) const
{
  return headers.count(name) > 0;
}

void
Headers::set(const std::string &name, const std::string &value)
{
  headers[name] = value;
}

void
Headers::emplace(const std::string &name, const std::string &value)
{
  headers.emplace(name, value);
}

void
Headers::remove(const std::string &name)
{
  headers.erase(name);
}

void
Headers::clear()
{
  headers.clear();
}

std::string &Headers::operator[](const std::string &name)
{
  return headers[name];
}

const std::string &Headers::operator[](const std::string &name) const
{
  return headers.at(name);
}

Headers::HeadersMap::iterator
Headers::begin()
{
  return headers.begin();
}

Headers::HeadersMap::const_iterator
Headers::begin() const
{
  return headers.begin();
}

Headers::HeadersMap::iterator
Headers::end()
{
  return headers.end();
}

Headers::HeadersMap::const_iterator
Headers::end() const
{
  return headers.end();
}

void
Headers::parse(std::istream &stream)
{
  std::string line;
  while (std::getline(stream, line) && line != "\r")
    {
      size_t colon = line.find(':');
      if (colon != std::string::npos)
        {
          std::string name = line.substr(0, colon);
          boost::algorithm::trim(name);
          boost::algorithm::to_lower(name);

          std::string value = line.substr(colon + 1);
          boost::algorithm::trim(value);

          headers[name] = value;
        }
    }
}

namespace loopp
{
  namespace http
  {
    std::ostream &operator<<(std::ostream &stream, const Headers &headers)
    {
      stream << boost::algorithm::join(headers.headers | boost::adaptors::transformed([](Headers::HeadersMap::value_type pv) {
                                         return (boost::format("%1%=%2%") % pv.first % pv.second).str();
                                       }),
                                       ",");

      return stream;
    }
  } // namespace http
} // namespace loopp
