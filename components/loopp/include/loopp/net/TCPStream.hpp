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

#ifndef LOOPP_NET_TCPSTREAM_HPP
#define LOOPP_NET_TCPSTREAM_HPP

#include <string>

#include "Stream.hpp"

#include "loopp/core/Mainloop.hpp"

namespace loopp
{
  namespace net
  {
    class TCPStream : public Stream
    {
    public:
      TCPStream(std::shared_ptr<loopp::core::MainLoop> loop);
      virtual ~TCPStream() = default;

    private:
      virtual int socket_read(uint8_t *buffer, std::size_t count);
      virtual int socket_write(uint8_t *buffer, std::size_t count);
      virtual void socket_close();
      virtual void socket_on_connected(std::string host, connect_slot_t slot);
    };
  }
}

#endif // LOOPP_NET_TCPSTREAM_HPP
