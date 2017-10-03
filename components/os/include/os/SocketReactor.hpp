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

#ifndef OS_SOCKETREACTOR_HPP
#define OS_SOCKETREACTOR_HPP

#include <chrono>
#include <list>
#include <functional>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "os/Task.hpp"
#include "os/Signal.hpp"
#include "os/Property.hpp"
#include "os/Trigger.hpp"

namespace os
{
  class SocketReactor
  {
  public:
    using callback_type = std::function<void()>;

    static SocketReactor &instance()
    {
      static SocketReactor instance;
      return instance;
    }

    void notify_read(int fd, callback_type read_cb);
    void notify_write(int fd, callback_type write_cb);
    void unnotify_read(int fd);
    void unnotify_write(int fd);

  private:
    SocketReactor();
    ~SocketReactor();

  private:
    enum class IoType { Read, Write };
    struct PollData
    {
      int fd;
      IoType type;
      callback_type callback;
    };
    using poll_list_type = std::list<PollData>;

  private:
    void run();

    poll_list_type::iterator find(int fd, IoType type);

    void notify(int fd, IoType type, callback_type cb);
    void unnotify(int fd, IoType type);

    int init_fd_set(poll_list_type &pollfds_copy, fd_set &read_set, fd_set &write_set);
    void interrupt();
    poll_list_type copy_poll_list() const;

  private:
    mutable os::Mutex mutex;
    poll_list_type poll_list;
    os::Trigger trigger;
    os::Task task;
  };
}

#endif // OS_SOCKETREACTOR_HPP
