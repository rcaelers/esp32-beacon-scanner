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

#include "loopp/net/NetworkErrors.hpp"

namespace
{
  struct NetworkErrCategory : std::error_category
  {
    const char* name() const noexcept override;
    std::string message(int ev) const override;
  };

  const char* NetworkErrCategory::name() const noexcept
  {
    return "network";
  }

  std::string NetworkErrCategory::message(int ev) const
  {
    switch (static_cast<loopp::net::NetworkErrc>(ev))
      {
      case loopp::net::NetworkErrc::Timeout:
        return "network timeout";
      case loopp::net::NetworkErrc::InternalError:
        return "internal error";
      case loopp::net::NetworkErrc::NameResolutionFailed:
        return "name resolution failed";
      case loopp::net::NetworkErrc::InvalidAddress:
        return "invalid address";
      case loopp::net::NetworkErrc::TLSProtocolError:
        return "TLS protocol error";
      case loopp::net::NetworkErrc::ConnectionRefused:
        return "connection refused";
      case loopp::net::NetworkErrc::ConnectionClosed:
        return "connection closed";
      case loopp::net::NetworkErrc::ReadError:
        return "read error";
      case loopp::net::NetworkErrc::WriteError:
        return "write error";
      default:
        return "(unrecognized error)";
      }
  }

  const NetworkErrCategory theNetworkErrCategory {};
}

namespace loopp
{
  namespace net
  {
    std::error_code make_error_code(net::NetworkErrc ec)
    {
      return {static_cast<int>(ec), theNetworkErrCategory};
    }
  }
}
