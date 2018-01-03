// Copyright (C) 2017,2018 Rob Caelers <rob.caelers@gmail.com>
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

#include "os/TCPStream.hpp"

#include <string>

#include "lwip/sockets.h"
#include "lwip/sys.h"

using namespace os;

TCPStream::TCPStream(std::shared_ptr<os::MainLoop> loop) :
  Stream(loop)
{
}

int
TCPStream::socket_read(uint8_t *buffer, std::size_t count)
{
  int ret = read(get_socket(), buffer, count);

  if (ret == -1)
    {
      ret = -errno;
    }

  return ret;
}

int
TCPStream::socket_write(uint8_t *buffer, std::size_t count)
{
  int ret = write(get_socket(), buffer, count);

  if (ret == -1)
    {
      ret = -errno;
    }

  return ret;
}

void
TCPStream::socket_close()
{
  ::close(get_socket());
}

void
TCPStream::socket_on_connected(std::string host, connect_slot_t slot)
{
  connected_property.set(true);
  slot.call(std::error_code());
}
