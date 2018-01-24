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

#include "loopp/mqtt/MqttErrors.hpp"

namespace
{
  struct MqttErrCategory : std::error_category
  {
    const char* name() const noexcept override;
    std::string message(int ev) const override;
  };

  const char* MqttErrCategory::name() const noexcept
  {
    return "mqtt";
  }

  std::string MqttErrCategory::message(int ev) const
  {
    switch (static_cast<loopp::mqtt::MqttErrc>(ev))
      {
      case loopp::mqtt::MqttErrc::Timeout:
        return "network timeout";
      case loopp::mqtt::MqttErrc::InternalError:
        return "internal error";
      case loopp::mqtt::MqttErrc::ProtocolError:
        return "protocol error";
      case loopp::mqtt::MqttErrc::NotConnected:
        return "not connected";
      default:
        return "(unrecognized error)";
      }
  }

  const MqttErrCategory theMqttErrCategory {};
}

namespace loopp
{
  namespace mqtt
  {
    std::error_code make_error_code(mqtt::MqttErrc ec)
    {
      return {static_cast<int>(ec), theMqttErrCategory};
    }
  }
}
