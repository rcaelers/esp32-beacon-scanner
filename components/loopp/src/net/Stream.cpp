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

#include "loopp/net/Stream.hpp"

#include <string>
#include <system_error>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include "loopp/net/Resolver.hpp"
#include "loopp/net/NetworkErrors.hpp"

#include "lwip/sockets.h"
#include "lwip/sys.h"

extern "C"
{
#include "esp_heap_caps.h"
}

#include "esp_log.h"

static const char *tag = "NET";

using namespace loopp;
using namespace loopp::net;

Stream::Stream(std::shared_ptr<loopp::core::MainLoop> loop)
  : sock(-1)
  , loop(std::move(loop))
  , resolver(Resolver::instance())
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
Stream::connect(const std::string &host, int port, const connect_callback_t &callback)
{
  ESP_LOGI(tag, "Connecting to %s:%s", host.c_str(), std::to_string(port).c_str());
  auto self = shared_from_this();

  auto l = [this, self, host, callback](std::error_code ec, struct addrinfo *addr_list) {
    if (!ec)
      {
        on_resolved(host, addr_list, callback);
      }
    else
      {
        callback(ec);
      }

    if (addr_list != nullptr)
      {
        freeaddrinfo(addr_list);
      }
  };
  auto x = loopp::core::bind_loop(loop, l);

  resolver.resolve_async(host, std::to_string(port), x);
}

void
Stream::on_resolved(const std::string &host, struct addrinfo *addr_list, const connect_callback_t &callback)
{
  try
    {
      struct addrinfo *addr = nullptr;

      // TODO: try next one on failure.
      for (struct addrinfo *cur = addr_list; cur != nullptr && addr == nullptr; cur = cur->ai_next)
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

      int ret = fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
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
      loop->notify_write(sock,
                         [this, self, host, callback](std::error_code ec) {
                           if (!ec)
                             {
                               socket_on_connected(host, callback);
                             }
                           else
                             {
                               callback(ec);
                             }
                         },
                         // TODO: make configurable
                         std::chrono::milliseconds(5000));
    }
  catch (const std::system_error &ex)
    {
      ESP_LOGD(tag, "connect exception %d %s", ex.code().value(), ex.what());
      callback(ex.code());
    }
}

void
Stream::write_async(StreamBuffer &buffer, const io_callback_t &callback)
{
  if (!connected_property.get())
    {
      callback(NetworkErrc::ConnectionClosed, 0);
    }
  else
    {
      auto self = shared_from_this();

      loop->invoke([this, self, &buffer, callback]() {
        write_op_queue.emplace_back(buffer, callback);
        if (write_op_queue.size() == 1)
          {
            do_write_async();
          }
      });
    }
}

void
Stream::read_async(StreamBuffer &buffer, std::size_t count, const io_callback_t &callback)
{
  do_read_async(buffer, count, 0, callback);
}

void
Stream::read_until_async(StreamBuffer &buffer, const std::string &until, const io_callback_t &callback)
{
  do_read_until_async(buffer, until, 0, callback);
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
  if (!write_op_queue.empty())
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
Stream::do_read_async(StreamBuffer &buf, std::size_t count, std::size_t bytes_transferred, const io_callback_t &callback)
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
      auto data = reinterpret_cast<uint8_t *>(buf.produce_data(left_to_read));
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
          loop->notify_read(sock, [this, self, &buf, bytes_transferred, count, callback](std::error_code ec) {
            if (!ec)
              {
                do_read_async(buf, count, bytes_transferred, callback);
              }
            else
              {
                callback(ec, bytes_transferred);
              }
          });
          return;
        }
      else
        {
          ec = NetworkErrc::ReadError;
        }
    }

  callback(ec, bytes_transferred);
}

bool
Stream::match_until(StreamBuffer &buf, std::size_t &start_pos, const std::string &match)
{
  char *data = buf.consume_data() + start_pos;
  char *data_end = buf.consume_data() + buf.consume_size() - match.size();

  for (; data <= data_end; data++)
    {
      int m = memcmp(data, match.c_str(), match.size());
      if (m == 0)
        {
          return true;
        }
    }

  if (buf.consume_size() >= match.size())
    {
      start_pos = buf.consume_size() - match.size() + 1;
    }
  return false;
}

void
Stream::do_read_until_async(StreamBuffer &buf, const std::string &until, std::size_t bytes_transferred, const io_callback_t &callback)
{
  auto self = shared_from_this();
  std::error_code ec;

  if (!connected_property.get())
    {
      ec = NetworkErrc::ConnectionClosed;
    }

  std::size_t start_pos = 0;
  while (!ec && !match_until(buf, start_pos, until))
    {
      std::size_t left_to_read = 512; // TODO:
      auto data = reinterpret_cast<uint8_t *>(buf.produce_data(left_to_read));
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
          loop->notify_read(sock, [this, self, &buf, bytes_transferred, until, callback](std::error_code ec) {
            if (!ec)
              {
                do_read_until_async(buf, until, bytes_transferred, callback);
              }
            else
              {
                callback(ec, bytes_transferred);
              }
          });
          return;
        }
      else
        {
          ec = NetworkErrc::ReadError;
        }
    }

  callback(ec, bytes_transferred);
}

void
Stream::close()
{
  if (connected_property.get())
    {
      connected_property.set(false);

      auto self = shared_from_this();
      loop->invoke([this, self]() {
        socket_close();
        loop->cancel(sock);
        sock = -1;
      });
    }
}

loopp::core::Property<bool> &
Stream::connected()
{
  return connected_property;
}
