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

#include <functional>
#include <map>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "os/Queue.hpp"

namespace os
{
  class MainLoop
  {
  public:
    using queue_callback = std::function<void ()>;

    MainLoop() = default;
    ~MainLoop() = default;

    MainLoop(const MainLoop&) = delete;
    MainLoop &operator=(const MainLoop&) = delete;
    MainLoop(MainLoop&&) = delete;
    MainLoop &operator=(MainLoop&&) = delete;

    // TODO: Create more generic function for registering event sources (like glib's GSource).
    template<class T>
    void register_queue(os::Queue<T> &queue, queue_callback fn)
    {
      assert(queue_set == nullptr && "Cannot add new queues once the main loop has started.");

      queues[queue.native_handle()] = fn;
      total_size += queue.size();
    }

    void terminate();
    void run();

  private:
    void prepare();

  private:
    std::map<SemaphoreHandle_t, queue_callback> queues;
    int total_size = 0;
    QueueSetHandle_t queue_set = nullptr;
  };
}

#endif // OS_MAINLOOP_HPP
