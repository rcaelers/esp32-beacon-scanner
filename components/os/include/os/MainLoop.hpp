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

#ifndef OS_MAINLOOP_HPP
#define OS_MAINLOOP_HPP

#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "os/Closure.hpp"
#include "os/Queue.hpp"
#include "os/Trigger.hpp"
#include "os/ThreadLocal.hpp"

namespace os
{
  class MainLoop
  {
  public:
    using io_callback = std::function<void()>;
    using queue_callback = std::function<void ()>;

    MainLoop() = default;
    ~MainLoop();

    MainLoop(const MainLoop&) = delete;
    MainLoop &operator=(const MainLoop&) = delete;
    MainLoop(MainLoop&&) = delete;
    MainLoop &operator=(MainLoop&&) = delete;

    static MainLoop *current();

    void post(std::shared_ptr<ClosureBase> closure);
    void notify_read(int fd, io_callback read_cb);
    void notify_write(int fd, io_callback write_cb);
    void unnotify_read(int fd);
    void unnotify_write(int fd);
    void unnotify(int fd);
    void run();
    void terminate();

  private:
    enum class IoType { Read, Write };
    struct PollData
    {
      int fd;
      IoType type;
      io_callback callback;
    };
    using poll_list_type = std::list<PollData>;

    poll_list_type::iterator find(int fd, IoType type);

    void notify(int fd, IoType type, io_callback cb);
    void unnotify(int fd, IoType type);

    int init_fd_set(poll_list_type &pollfds_copy, fd_set &read_set, fd_set &write_set);
    poll_list_type copy_poll_list() const;

    void process_queue();

    static ThreadLocal<MainLoop> &get_thread_local();

  private:
    mutable os::Mutex poll_list_mutex;
    poll_list_type poll_list;
    os::Queue<std::shared_ptr<os::ClosureBase>> queue;
    os::Trigger trigger;
  };
}

#endif // OS_MAINLOOP_HPP
