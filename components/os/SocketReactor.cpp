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

#define _GLIBCXX_USE_C99
#include "os/SocketReactor.hpp"

#include <string>
#include <system_error>
#include <algorithm>
#include <cstring>

#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "esp_log.h"

#undef bind

static const char tag[] = "NET";

using namespace os;

SocketReactor::SocketReactor()
  : task("socket_reactor_task", std::bind(&SocketReactor::run, this), os::Task::CoreId::NoAffinity, 4096)
{
}

SocketReactor::~SocketReactor()
{
}

SocketReactor::poll_list_type::iterator
SocketReactor::find(int fd, IoType type)
{
  return std::find_if(poll_list.begin(), poll_list.end(),
                      [fd, type](PollData pd)
                      {
                        return pd.fd == fd && pd.type == type;
                      });
}

void SocketReactor::notify_read(int fd, callback_type cb)
{
  notify(fd, IoType::Read, cb);
}

void SocketReactor::notify_write(int fd, callback_type cb)
{
  notify(fd, IoType::Write, cb);
}

void SocketReactor::unnotify_read(int fd)
{
  unnotify(fd, IoType::Read);
}

void SocketReactor::unnotify_write(int fd)
{
  unnotify(fd, IoType::Write);
}

void SocketReactor::notify(int fd, IoType type, callback_type cb)
{
  ScopedLock l(mutex);
  auto iter = find(fd, type);

  if (iter != poll_list.end())
    {
      iter->callback = cb;
    }
  else
    {
      PollData pd;
      pd.fd = fd;
      pd.type = type;
      pd.callback = cb;
      poll_list.push_back(pd);
    }
  interrupt();
}

void SocketReactor::unnotify(int fd, IoType type)
{
  ScopedLock l(mutex);
  auto iter = find(fd, type);

  if (iter != poll_list.end())
    {
      poll_list.erase(iter);
    }
  trigger.signal();
}

void
SocketReactor::interrupt()
{
  trigger.signal();
}

int
SocketReactor::init_fd_set(poll_list_type &poll_list_copy, fd_set &read_set, fd_set &write_set)
{
  FD_ZERO(&read_set);
  FD_ZERO(&write_set);

  int trigger_fd = trigger.get_poll_fd();
  int max_fd = trigger_fd;
  FD_SET(trigger_fd, &read_set);

  ScopedLock l(mutex);
  for (auto &pd : poll_list_copy)
    {
      max_fd = std::max(max_fd, pd.fd);

      switch (pd.type)
        {
        case IoType::Read:
          FD_SET(pd.fd, &read_set);
          break;
        case IoType::Write:
          FD_SET(pd.fd, &write_set);
          break;
        }
    }

  return max_fd;
}

SocketReactor::poll_list_type
SocketReactor::copy_poll_list() const
{
  ScopedLock l(mutex);
  return poll_list;
}

void
SocketReactor::run()
{
  while (true)
    {
      fd_set read_set;
      fd_set write_set;

      poll_list_type poll_list_copy = copy_poll_list();

      int max_fd = init_fd_set(poll_list_copy, read_set, write_set);
      ESP_LOGI(tag, "Select %d", max_fd);
      int r = select(max_fd + 1, &read_set, &write_set, nullptr, nullptr);
      ESP_LOGI(tag, "Selected");

      if (r == -1)
        {
          const char* error = strerror(errno);
          ESP_LOGE(tag, "Error during select: %s", error);
        }

      else if (r > 0)
        {
          if (FD_ISSET(trigger.get_poll_fd(), &read_set))
            {
              trigger.confirm();
            }
          else
            {
              for (auto &pd : poll_list_copy)
                {
                  if ( (pd.type == IoType::Read && FD_ISSET(pd.fd, &read_set)) ||
                       (pd.type == IoType::Write && FD_ISSET(pd.fd, &write_set)))
                    {
                      ESP_LOGI(tag, "Select for %d/%d", pd.fd, static_cast<std::underlying_type<IoType>::type>(pd.type));
                      try
                        {
                          pd.callback();
                        }
                      catch(...)
                        {
                          ESP_LOGE(tag, "Exception while handling %d/%d", pd.fd, static_cast<std::underlying_type<IoType>::type>(pd.type));
                        }
                    }
                }
            }
        }
    }
}
