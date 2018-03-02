// Copyright (C) 2018 Rob Caelers <rob.caelers@gmail.com>
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

#include "loopp/http/HttpErrors.hpp"

namespace
{
  struct HttpErrCategory : std::error_category
  {
    const char *name() const noexcept override;
    std::string message(int ev) const override;
  };

  const char *HttpErrCategory::name() const noexcept
  {
    return "http";
  }

  std::string HttpErrCategory::message(int ev) const
  {
    switch (static_cast<loopp::http::HttpErrc>(ev))
      {
        case loopp::http::HttpErrc::Timeout:
          return "network timeout";
        case loopp::http::HttpErrc::InternalError:
          return "internal error";
        case loopp::http::HttpErrc::ProtocolError:
          return "protocol error";
        case loopp::http::HttpErrc::InvalidURI:
          return "invalid URI";
        default:
          return "(unrecognized error)";
      }
  }

  const HttpErrCategory theHttpErrCategory{};
} // namespace

namespace loopp
{
  namespace http
  {
    std::error_code make_error_code(HttpErrc ec)
    {
      return { static_cast<int>(ec), theHttpErrCategory };
    }
  } // namespace http
} // namespace loopp
