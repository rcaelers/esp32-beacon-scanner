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
#include "os/MainLoop.hpp"

#include <string>
#include <system_error>
#include <algorithm>
#include <cstring>

#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "esp_log.h"

#include "os/NetworkErrors.hpp"

#undef bind

static const char tag[] = "MAINLOOP";

using namespace os;

MainLoop::~MainLoop()
{
  get_thread_local().remove();
}

void
MainLoop::post(std::function<void()> func)
{
  post(std::make_shared<os::FunctionClosure<void ()>>(func));
}

void
MainLoop::post(std::shared_ptr<ClosureBase> closure)
{
  queue.push(closure);
  trigger.signal();
}

void
MainLoop::terminate()
{
  terminate_loop = true;
  trigger.signal();
}

std::shared_ptr<MainLoop>
MainLoop::current()
{
  return get_thread_local().get();
}

ThreadLocal<std::shared_ptr<MainLoop>> &
MainLoop::get_thread_local()
{
  static ThreadLocal<std::shared_ptr<MainLoop>> local;
  return local;
}

void
MainLoop::notify_read(int fd, io_callback cb, std::chrono::milliseconds timeout_duration)
{
  notify(fd, IoType::Read, cb, timeout_duration);
}

void
MainLoop::notify_write(int fd, io_callback cb, std::chrono::milliseconds timeout_duration)
{
  notify(fd, IoType::Write, cb, timeout_duration);
}

void
MainLoop::unnotify_read(int fd)
{
  unnotify(fd, IoType::Read);
}

void
MainLoop::unnotify_write(int fd)
{
  unnotify(fd, IoType::Write);
}

void
MainLoop::unnotify(int fd)
{
  unnotify(fd, IoType::Read);
  unnotify(fd, IoType::Write);
}

MainLoop::poll_list_type::iterator
MainLoop::find(int fd, IoType type)
{
  return std::find_if(poll_list.begin(), poll_list.end(),
                      [fd, type](PollData pd)
                      {
                        return pd.fd == fd && pd.type == type;
                      });
}

void
MainLoop::notify(int fd, IoType type, io_callback cb, std::chrono::milliseconds timeout_duration)
{
  ScopedLock l(poll_list_mutex);
  auto iter = find(fd, type);

  if (iter != poll_list.end())
    {
      iter->callback = cb;
      iter->start_time = std::chrono::system_clock::now();
      iter->timeout_duration = timeout_duration;
    }
  else
    {
      PollData pd;
      pd.fd = fd;
      pd.type = type;
      pd.callback = cb;
      pd.start_time = std::chrono::system_clock::now();
      pd.timeout_duration = timeout_duration;
      poll_list.push_back(pd);
    }
  trigger.signal();
}

void
MainLoop::unnotify(int fd, IoType type)
{
  ScopedLock l(poll_list_mutex);
  auto iter = find(fd, type);

  if (iter != poll_list.end())
    {
      poll_list.erase(iter);
    }
  trigger.signal();
}

int
MainLoop::do_select(poll_list_type &poll_list_copy, fd_set &read_set, fd_set &write_set)
{
  FD_ZERO(&read_set);
  FD_ZERO(&write_set);

  int trigger_fd = trigger.get_poll_fd();
  int max_fd = trigger_fd;
  FD_SET(trigger_fd, &read_set);

  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  std::chrono::milliseconds timeout = std::chrono::milliseconds::max();

  ScopedLock l(poll_list_mutex);
  for (auto &pd : poll_list_copy)
    {
      max_fd = std::max(max_fd, pd.fd);

      if (pd.timeout_duration != std::chrono::milliseconds::max())
        {
          auto t = std::chrono::duration_cast<std::chrono::milliseconds>(pd.start_time - now + pd.timeout_duration);
          timeout = std::max(std::chrono::milliseconds(0), std::min(timeout, t));
        }

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

  int r = 0;

  if (timeout != std::chrono::milliseconds::max())
    {
      timeval tv;
      tv.tv_sec = timeout.count() / 1000;
      tv.tv_usec = (timeout.count() % 1000) / 1000;
      r = select(max_fd + 1, &read_set, &write_set, nullptr, &tv);
    }
  else
    {
      r = select(max_fd + 1, &read_set, &write_set, nullptr, nullptr);
    }

  return r;
}

MainLoop::poll_list_type
MainLoop::copy_poll_list() const
{
  ScopedLock l(poll_list_mutex);
  return poll_list;
}

void
MainLoop::run()
{
  get_thread_local().set(shared_from_this());

  while (true)
    {
      poll_list_type poll_list_copy = copy_poll_list();
      fd_set read_set;
      fd_set write_set;

      int r = do_select(poll_list_copy, read_set, write_set);

      if (r == -1)
        {
          const char* error = strerror(errno);
          ESP_LOGE(tag, "Error during select: %s", error);
        }
      else if (r == 0)
        {
          std::error_code ec;
          std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

          for (auto &pd : poll_list_copy)
            {
              unnotify(pd.fd, pd.type);

              if (pd.timeout_duration != std::chrono::milliseconds::max() &&
                  pd.start_time + pd.timeout_duration >= now)
                {
                  ESP_LOGE(tag, "Timeout %d/%d", pd.fd, static_cast<std::underlying_type<IoType>::type>(pd.type));
                  try
                    {
                      pd.callback(os::NetworkErrc::Timeout);
                    }
                  catch(...)
                    {
                      ESP_LOGE(tag, "Exception while handling timeout %d/%d", pd.fd, static_cast<std::underlying_type<IoType>::type>(pd.type));
                    }
                }
            }
        }
      else if (r > 0)
        {
          std::error_code ec;
          for (auto &pd : poll_list_copy)
            {
              if ( (pd.type == IoType::Read && FD_ISSET(pd.fd, &read_set)) ||
                   (pd.type == IoType::Write && FD_ISSET(pd.fd, &write_set)))
                {
                  unnotify(pd.fd, pd.type);
                  ESP_LOGD(tag, "Select for %d/%d", pd.fd, static_cast<std::underlying_type<IoType>::type>(pd.type));
                  try
                    {
                      pd.callback(ec);
                    }
                  catch(...)
                    {
                      ESP_LOGE(tag, "Exception while handling %d/%d", pd.fd, static_cast<std::underlying_type<IoType>::type>(pd.type));
                    }
                }
            }
          if (FD_ISSET(trigger.get_poll_fd(), &read_set))
            {
              process_queue();
              trigger.confirm();
            }
        }
    }
}

void
MainLoop::process_queue()
{
  while (true)
    {
      nonstd::optional<std::shared_ptr<os::ClosureBase>> obj(queue.pop_for(std::chrono::milliseconds(0)));
      if (! obj)
        {
          break;
        }
      std::shared_ptr<os::ClosureBase> closure = *obj;
      try
        {
          closure->operator()();
        }
      catch(...)
        {
          ESP_LOGE(tag, "Exception while handling invoked function");
        }
    };
}
