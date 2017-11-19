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

#ifndef OS_MQTTPACKET_HPP
#define OS_MQTTPACKET_HPP

#include <string>

#include "os/Buffer.hpp"
#include "os/bitmask.hpp"

namespace os
{
  enum class PacketType
    {
      Connect = 1,
      ConnAck = 2,
      Publish = 3,
      PubAck = 4,
      PubRec = 5,
      PubRel = 6,
      PubComp = 7,
      Subscribe = 8,
      SubAck = 9,
      Unsubscribe = 10,
      UnsubAck = 11,
      PingReq = 12,
      PingResp = 13,
      Disconnect = 14,
    };

  enum class ConnectFlags : uint8_t
    {
      None          = 0,
      CleanSession  = 0b00000010u,
      Will          = 0b00000100u,
      WillRetain    = 0b00100000u,
      Password      = 0b01000000u,
      UserName      = 0b10000000u
    };
  DEFINE_BITMASK(ConnectFlags);

  enum class PublishFlags : uint8_t
    {
      None          = 0,
      Duplicate     = 0b00001000u,
      Retain        = 0b00000001u,
      QosMask       = 0b00000110u,
      Qos0          = 0b00000000u,
      Qos1          = 0b00000010u,
      Qos2          = 0b00000100u,
    };
  DEFINE_BITMASK(PublishFlags);

  class MqttPacket
  {
  public:
    MqttPacket() = default;

    void add(uint8_t value);
    void append(std::string s);
    void add(std::string str);
    void add_length(std::size_t size);
    void add_fixed_header(os::PacketType type, std::uint8_t flags);
    Buffer to_buffer();
    uint8_t *data() noexcept;
    std::size_t size() const noexcept;

  private:
    std::string data_;
  };
}

#endif // OS_MQTTPACKET_HPP
