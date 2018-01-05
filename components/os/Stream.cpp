// Copyright (C) 2017, 2018 Rob Caelers <rob.caelers@gmail.com>
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

#define _GLIBCXX_USE_C99

#include "os/Stream.hpp"

#include <string>
#include <system_error>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include "os/Resolver.hpp"
#include "os/NetworkErrors.hpp"

#include "lwip/sockets.h"
#include "lwip/sys.h"

extern "C"
{
#include "esp_heap_caps.h"
}

#include "esp_log.h"

static const char tag[] = "NET";

using namespace os;

Stream::Stream(std::shared_ptr<os::MainLoop> loop) :
    sock(-1),
    loop(loop),
    resolver(Resolver::instance())
{
}

Stream::~Stream()
{
  if (sock != -1)
    {
      ::close(sock);
      loop->unnotify(sock);
    }
}

void
Stream::connect(std::string host, int port, connect_callback_t callback)
{
  connect(std::move(host), port, os::make_slot(loop, callback));
}

void
Stream::connect(std::string host, int port, connect_slot_t slot)
{
  ESP_LOGI(tag, "Connecting to %s:%s", host.c_str(), std::to_string(port).c_str());
  auto self = shared_from_this();
  resolver.resolve_async(host, std::to_string(port), os::make_slot(loop, [this, self, host, slot] (std::error_code ec, struct addrinfo *addr_list) {
        if (!ec)
          {
            on_resolved(host, addr_list, slot);
          }
        else
          {
            slot.call(ec);
          }

        if (addr_list != NULL)
          {
            freeaddrinfo(addr_list);
          }
      }));
}

void
Stream::on_resolved(std::string host, struct addrinfo *addr_list, connect_slot_t slot)
{
  try
    {
      struct addrinfo *addr = nullptr;

      // TODO: try next one on failure.
      for (struct addrinfo *cur = addr_list; cur != nullptr &&  addr == nullptr; cur = cur->ai_next)
        {
          if (cur->ai_family == AF_INET || cur->ai_family == AF_INET6)
            {
              addr = cur;
            }
        }

      if (addr == nullptr)
        {
          throw std::system_error(NetworkErrc::NameResolutionFailed, "No suitable IP address");
        }

      sock = socket(addr->ai_family, SOCK_STREAM, IPPROTO_TCP);
      if (sock < 0)
        {
          throw std::system_error(NetworkErrc::InternalError, "Could not create socket");
        }

      int ret = fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK );
      if (ret < 0)
        {
          throw std::system_error(NetworkErrc::InternalError, "Could not set non-blocking");
        }

      ret = ::connect(sock, addr->ai_addr, addr->ai_addrlen);
      ESP_LOGD(tag, "connect %s: ret = %x errno %d", host.c_str(), -ret, errno);

      if (ret < 0 && errno != EINPROGRESS)
        {
          ::close(sock);
          sock = -1;
          // TODO: use errno
          throw std::system_error(NetworkErrc::InternalError, "Could not connect");
        }

      auto self = shared_from_this();
      loop->notify_write(sock, [this, self, host, slot](std::error_code ec) {
          if (!ec)
            {
              socket_on_connected(host, slot);
            }
          else
            {
              slot.call(ec);
            }
        },
        // TODO: make configurable
        std::chrono::milliseconds(5000));
    }
  catch (const std::system_error &ex)
    {
      ESP_LOGD(tag, "connect exception %d %s", ex.code().value(), ex.what());
      slot.call(ex.code());
    }
}

void
Stream::write_async(StreamBuffer &buffer, io_callback_t callback)
{
  write_async(buffer, os::make_slot(loop, callback));
}

void
Stream::write_async(StreamBuffer &buffer, io_slot_t slot)
{
  if (!connected_property.get())
  {
    slot.call(NetworkErrc::ConnectionClosed, 0);
  }
  else
  {
    auto self = shared_from_this();

    loop->post([this, self, &buffer, slot] () {
        write_op_queue.emplace_back(buffer, slot);
        if (write_op_queue.size() == 1)
        {
          do_write_async();
        }
      });
  }
}

void
Stream::read_async(StreamBuffer &buffer, std::size_t count, io_callback_t callback)
{
  read_async(buffer, count, os::make_slot(loop, callback));
}

void
Stream::read_async(StreamBuffer &buffer, std::size_t count, io_slot_t slot)
{
  do_read_async(buffer, count, 0, slot);
}

void
Stream::do_write_async()
{
  auto &write_op = write_op_queue.front();

  std::error_code ec;
  do
    {
      int ret = socket_write(reinterpret_cast<uint8_t *>(write_op.buffer().consume_data()), write_op.buffer().consume_size());

      if (ret > 0)
        {
          write_op.buffer().consume_commit(ret);
        }
      else if (ret == -EAGAIN)
        {
          do_wait_write_async();
          return;
        }
      else
        {
          ec = NetworkErrc::WriteError;
        }
    }
  while (!ec && write_op.buffer().consume_size() > 0);

  write_op.call(ec, write_op.buffer().consume_size());
  write_op_queue.pop_front();
  do_wait_write_async();
}

void
Stream::do_wait_write_async()
{
  if (write_op_queue.size() > 0)
  {
    auto self = shared_from_this();
    loop->notify_write(sock, [this, self](std::error_code ec) {
        if (!ec)
        {
          do_write_async();
        }
        else
        {
          auto &write_op = write_op_queue.front();
          write_op.call(ec, 0);
          write_op_queue.pop_front();
          do_wait_write_async();
        }
      });
  }
}

void
Stream::do_read_async(StreamBuffer &buf, std::size_t count, std::size_t bytes_transferred, io_slot_t slot)
{
  auto self = shared_from_this();
  std::error_code ec;

  if (!connected_property.get())
  {
    ec = NetworkErrc::ConnectionClosed;
  }

  while (!ec && (bytes_transferred < count))
    {
      std::size_t left_to_read = count - bytes_transferred;
      uint8_t *data = reinterpret_cast<uint8_t *>(buf.produce_data(left_to_read));
      int ret = socket_read(data, left_to_read);

      if (ret > 0)
        {
          buf.produce_commit(ret);
          bytes_transferred += ret;
        }
      else if (ret == 0)
        {
          ESP_LOGI(tag, "Connection closed");
          connected_property.set(false);
          ec = NetworkErrc::ConnectionClosed;
        }
      else if (ret == -EAGAIN)
        {
          loop->notify_read(sock, [this, self, &buf, bytes_transferred, count, slot](std::error_code ec) {
              if (!ec)
                {
                  do_read_async(buf, count, bytes_transferred, slot);
                }
              else
                {
                  slot.call(ec, bytes_transferred);
                }
            });
          return;
        }
      else
        {
          ec = NetworkErrc::ReadError;
        }
    }

  slot.call(ec, bytes_transferred);
}


void
Stream::close()
{
  socket_close();
  loop->unnotify(sock);
  connected_property.set(false);
}

os::Property<bool> &
Stream::connected()
{
  return connected_property;
}
