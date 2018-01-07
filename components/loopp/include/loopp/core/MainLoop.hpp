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

#ifndef LOOPP_CORE_MAINLOOP_HPP
#define LOOPP_CORE_MAINLOOP_HPP

#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <system_error>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "loopp/core/Closure.hpp"
#include "loopp/core/Queue.hpp"
#include "loopp/core/Trigger.hpp"
#include "loopp/core/ThreadLocal.hpp"

#include "lwip/sockets.h"
#include "lwip/sys.h"

namespace loopp
{
  namespace core
  {
    class MainLoop : public std::enable_shared_from_this<MainLoop>
    {
    public:
      using io_callback = std::function<void(std::error_code ec)>;
      using timer_callback = std::function<void()>;
      using timer_id = int;

      MainLoop() = default;
      ~MainLoop();

      MainLoop(const MainLoop&) = delete;
      MainLoop &operator=(const MainLoop&) = delete;
      MainLoop(MainLoop&&) = delete;
      MainLoop &operator=(MainLoop&&) = delete;

      static std::shared_ptr<MainLoop> current();

      void post(std::function<void()> func);
      void post(std::shared_ptr<ClosureBase> closure);

      void notify_read(int fd, io_callback read_cb, std::chrono::milliseconds timeout_duration = std::chrono::milliseconds::max());
      void notify_write(int fd, io_callback write_cb, std::chrono::milliseconds timeout_duration = std::chrono::milliseconds::max());
      void unnotify_read(int fd);
      void unnotify_write(int fd);
      void unnotify(int fd);

      void run();
      void terminate();

      timer_id add_timer(std::chrono::milliseconds duration, timer_callback callback);
      timer_id add_periodic_timer(std::chrono::milliseconds period, timer_callback callback);
      void cancel_timer(timer_id id);

    private:
      enum class IoType { Read, Write };
      struct PollData
      {
        int fd;
        IoType type;
        std::chrono::milliseconds timeout_duration;
        std::chrono::system_clock::time_point start_time;
        io_callback callback;
      };
      using poll_list_type = std::list<PollData>;

      struct TimerData
      {
        timer_id id;
        std::chrono::milliseconds period;
        std::chrono::system_clock::time_point expire_time;
        timer_callback callback;
      };
      using timer_list_type = std::list<TimerData>;

      poll_list_type::iterator find(int fd, IoType type);
      void notify(int fd, IoType type, io_callback cb, std::chrono::milliseconds timeout_duration);
      void unnotify(int fd, IoType type);
      std::chrono::milliseconds get_first_expiring_timer_duration();
      timer_list_type::iterator get_first_expiring_timer();
      int do_select(poll_list_type &poll_list_copy);
      poll_list_type get_poll_list() const;
      void handle_timeout(poll_list_type &poll_list_copy);
      void handle_io(poll_list_type &poll_list_copy);
      void handle_queue();
      void handle_timers();

      static ThreadLocal<std::shared_ptr<MainLoop>> &get_thread_local();

    private:
      fd_set read_set;
      fd_set write_set;
      mutable loopp::core::Mutex poll_list_mutex;
      poll_list_type poll_list;
      loopp::core::Queue<std::shared_ptr<loopp::core::ClosureBase>> queue;
      loopp::core::Trigger trigger;
      bool terminate_loop = false;
      timer_id next_timer_id = 1;
      mutable loopp::core::Mutex timer_list_mutex;
      timer_list_type timers;
    };
  }
}

#endif // LOOPP_CORE_MAINLOOP_HPP
