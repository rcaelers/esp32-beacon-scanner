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

#include "loopp/core/MainLoop.hpp"

#include <string>
#include <system_error>
#include <algorithm>
#include <cstring>

#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "esp_log.h"

#include "loopp/net/NetworkErrors.hpp"

#include "loopp/utils/hexdump.hpp"

static const char tag[] = "MAINLOOP";

using namespace loopp;
using namespace loopp::core;

MainLoop::~MainLoop()
{
  get_thread_local().remove();
}

void
MainLoop::invoke_func(std::function<void()> func)
{
  ScopedLock l(queue_mutex);
  queue.push(func);
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

void
MainLoop::cancel(int fd)
{
  cancel(fd, IoType::Read);
  cancel(fd, IoType::Write);
}

MainLoop::poll_list_type::iterator
MainLoop::find(int fd, IoType type)
{
  return std::find_if(poll_list.begin(), poll_list.end(), [fd, type](PollData pd) { return pd.fd == fd && pd.type == type; });
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
      pd.cancelled = false;
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
      trigger.signal();
    }
}

void
MainLoop::cancel(int fd, IoType type)
{
  ScopedLock l(poll_list_mutex);
  auto iter = find(fd, type);

  if (iter != poll_list.end())
    {
      iter->cancelled = true;
      trigger.signal();
    }
}

MainLoop::timer_id
MainLoop::add_timer(std::chrono::milliseconds duration, timer_callback callback)
{
  ScopedLock l(timer_list_mutex);
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

  TimerData data;
  data.id = next_timer_id++;
  data.expire_time = now + duration;
  data.period = std::chrono::milliseconds::max();
  data.callback = callback;

  timers.push_back(data);
  trigger.signal();

  return data.id;
}

MainLoop::timer_id
MainLoop::add_periodic_timer(std::chrono::milliseconds period, timer_callback callback)
{
  ScopedLock l(timer_list_mutex);
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

  TimerData data;
  data.id = next_timer_id++;
  data.expire_time = now + period;
  data.period = period;
  data.callback = callback;

  timers.push_back(data);
  trigger.signal();

  return data.id;
}

void
MainLoop::cancel_timer(timer_id id)
{
  ScopedLock l(timer_list_mutex);
  auto iter = std::find_if(timers.begin(), timers.end(), [id](const TimerData &t) { return t.id == id; });
  if (iter != timers.end())
    {
      timers.erase(iter);
    }
  trigger.signal();
}

MainLoop::timer_list_type::iterator
MainLoop::get_first_expiring_timer()
{
  auto iter = std::min_element(timers.begin(), timers.end(), [](TimerData &a, TimerData &b) { return a.expire_time < b.expire_time; });

  return iter;
}

std::chrono::milliseconds
MainLoop::get_first_expiring_timer_duration()
{
  ScopedLock l(timer_list_mutex);

  auto iter = get_first_expiring_timer();
  if (iter != timers.end())
    {
      return std::chrono::duration_cast<std::chrono::milliseconds>(iter->expire_time - std::chrono::system_clock::now());
    }
  else
    {
      return std::chrono::milliseconds::max();
    }
}

int
MainLoop::do_select(poll_list_type &poll_list_copy)
{
  FD_ZERO(&read_set);
  FD_ZERO(&write_set);

  int trigger_fd = trigger.get_poll_fd();
  int max_fd = trigger_fd;
  FD_SET(trigger_fd, &read_set);

  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  std::chrono::milliseconds timeout = get_first_expiring_timer_duration();

  for (auto &pd : poll_list_copy)
    {
      max_fd = std::max(max_fd, pd.fd);

      if (pd.timeout_duration != std::chrono::milliseconds::max())
        {
          auto t = std::chrono::duration_cast<std::chrono::milliseconds>(pd.start_time - now + pd.timeout_duration);
          timeout = std::max(std::chrono::milliseconds(0), std::min(timeout, t));
        }

      if (!pd.cancelled)
        {
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
    }

  int r = 0;

#if DEBUG_SELECT
  hexdump(tag, "IN r: ", reinterpret_cast<uint8_t *>(&read_set), sizeof(read_set));
  hexdump(tag, "IN w: ", reinterpret_cast<uint8_t *>(&write_set), sizeof(write_set));
#endif

  if (timeout != std::chrono::milliseconds::max())
    {
      timeval tv;
      tv.tv_sec = timeout.count() / 1000;
      tv.tv_usec = (timeout.count() % 1000) * 1000;
#if DEBUG_SELECT
      ESP_LOGD(tag, "Select timeout %d %d:", static_cast<int>(tv.tv_sec), static_cast<int>(tv.tv_usec / 1000));
#endif
      r = select(max_fd + 1, &read_set, &write_set, nullptr, &tv);
    }
  else
    {
      r = select(max_fd + 1, &read_set, &write_set, nullptr, nullptr);
    }

#if DEBUG_SELECT
  ESP_LOGD(tag, "Select ret = %d:", r);
  hexdump(tag, "OUT r: ", reinterpret_cast<uint8_t *>(&read_set), sizeof(read_set));
  hexdump(tag, "OUT w: ", reinterpret_cast<uint8_t *>(&write_set), sizeof(write_set));
#endif

  return r;
}

MainLoop::poll_list_type
MainLoop::get_poll_list() const
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
      poll_list_type poll_list_copy = get_poll_list();

      int r = do_select(poll_list_copy);

      if (r == -1)
        {
          const char *error = strerror(errno);
          ESP_LOGE(tag, "Error during select: %s", error);
        }
      else if (r == 0)
        {
          handle_timeout(poll_list_copy);
          handle_timers();
        }
      else if (r > 0)
        {
          if (FD_ISSET(trigger.get_poll_fd(), &read_set))
            {
              handle_queue();
            }

          handle_io(poll_list_copy);
          handle_timers();
        }
    }
}

void
MainLoop::handle_timeout(poll_list_type &poll_list_copy)
{
  std::error_code ec;
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

  for (auto &pd : poll_list_copy)
    {
      if (pd.timeout_duration != std::chrono::milliseconds::max() && now > pd.start_time + pd.timeout_duration)
        {
          unnotify(pd.fd, pd.type);
          try
            {
              pd.callback(loopp::net::NetworkErrc::Timeout);
            }
          catch (const std::system_error &ex)
            {
              ESP_LOGE(tag,
                       "System error while handling timeout %d/%d %d %s",
                       pd.fd,
                       static_cast<std::underlying_type<IoType>::type>(pd.type),
                       ex.code().value(),
                       ex.what());
            }
          catch (const std::exception &ex)
            {
              ESP_LOGE(tag, "Exception while handling timeout %d/%d %s", pd.fd, static_cast<std::underlying_type<IoType>::type>(pd.type), ex.what());
            }
          catch (...)
            {
              ESP_LOGE(tag, "Exception while handling timeout %d/%d", pd.fd, static_cast<std::underlying_type<IoType>::type>(pd.type));
            }
        }
    }
}

void
MainLoop::handle_io(poll_list_type &poll_list_copy)
{
  std::error_code ec;
  for (auto &pd : poll_list_copy)
    {
      if ((pd.type == IoType::Read && FD_ISSET(pd.fd, &read_set)) || (pd.type == IoType::Write && FD_ISSET(pd.fd, &write_set)) || pd.cancelled)
        {
          unnotify(pd.fd, pd.type);
          try
            {
              if (pd.cancelled)
                {
                  ec = loopp::net::NetworkErrc::Cancelled;
                }
              pd.callback(ec);
            }
          catch (const std::system_error &ex)
            {
              ESP_LOGE(tag,
                       "System error while handling %d/%d %d %s",
                       pd.fd,
                       static_cast<std::underlying_type<IoType>::type>(pd.type),
                       ex.code().value(),
                       ex.what());
            }
          catch (const std::exception &ex)
            {
              ESP_LOGE(tag, "Exception while handling %d/%d %s", pd.fd, static_cast<std::underlying_type<IoType>::type>(pd.type), ex.what());
            }
          catch (...)
            {
              ESP_LOGE(tag, "Exception while handling %d/%d", pd.fd, static_cast<std::underlying_type<IoType>::type>(pd.type));
            }
        }
    }
}

void
MainLoop::handle_timers()
{
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  timer_list_type notify_list;

  timer_list_mutex.lock();
  auto first = get_first_expiring_timer();
  while (first != timers.end() && now >= first->expire_time)
    {
      notify_list.push_back(*first);
      if (first->period != std::chrono::milliseconds::max())
        {
          first->expire_time += first->period;
        }
      else
        {
          timers.erase(first);
        }
      first = get_first_expiring_timer();
    }
  timer_list_mutex.unlock();

  for (auto t : notify_list)
    {
      t.callback();
    }
}

void
MainLoop::handle_queue()
{
  int size = 0;

  {
    ScopedLock l(queue_mutex);
    size = queue.size();
    trigger.confirm();
  }

  for (int i = 0; i < size; i++)
    {
      nonstd::optional<deferred_func> obj(queue.pop_for(std::chrono::milliseconds(0)));
      if (obj)
        {
          deferred_func func = *obj;
          try
            {
              func();
            }
          catch (const std::system_error &ex)
            {
              ESP_LOGE(tag, "System error while handling invoked function%d %s", ex.code().value(), ex.what());
            }
          catch (const std::exception &ex)
            {
              ESP_LOGE(tag, "Exception while handling invoked function: %s", ex.what());
            }
          catch (...)
            {
              ESP_LOGE(tag, "Exception while handling invoked function");
            }
        }
    }
}
