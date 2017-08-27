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

#ifndef OS_QUEUE_HPP
#define OS_QUEUE_HPP

#include <chrono>
#include <vector>

#include "os/Mutex.hpp"
#include "os/Semaphore.hpp"
#include "os/ScopedLock.hpp"

namespace os
{
  template<typename T>
  class Queue
  {
  public:
    Queue(int max_size)
      : produce_sem(max_size, max_size),
        consume_sem(max_size, 0),
        max_size(max_size)
    {
      queue_data.reserve(max_size);
    }

    ~Queue()
    {
    }

    Queue(const Queue&) = delete;
    Queue &operator=(const Queue&) = delete;

    Queue(Queue &&lhs)
      : mutex(std::move(lhs.mutex)),
        produce_sem(std::move(lhs.produce_sem)),
        consume_sem(std::move(lhs.consume_sem)),
        queue_data(std::move(lhs.queue_data)),
        max_size(lhs.max_size)
    {
    }

    Queue &operator=(Queue &&lhs)
    {
      if (this != &lhs)
        {
          mutex = std::move(lhs.mutex);
          produce_sem = std::move(lhs.produce_sem);
          consume_sem = std::move(lhs.consume_sem);
          queue_data = std::move(lhs.queue_data);
          max_size = lhs.max_size; 
       }
      return *this;
    }

    bool push(const T &obj)
    {
      return push(obj, std::chrono::milliseconds(portMAX_DELAY));
    }

    bool push(const T &obj, std::chrono::milliseconds timeout_duration)
    {
      bool ok = produce_sem.take(timeout_duration);
      if (ok)
        {
          mutex.lock();
          queue_data.push_back(obj);
          mutex.unlock();
          consume_sem.give();
        }
      return ok;
    }

    bool push(T &&obj)
    {
      return push(std::move(obj), std::chrono::milliseconds(portMAX_DELAY));
    }

    bool push(T &&obj, std::chrono::milliseconds timeout_duration)
    {
      bool ok = produce_sem.take(timeout_duration);
      if (ok)
        {
          {
            ScopedLock l(mutex);
            queue_data.push_back(std::move(obj));
          }
          consume_sem.give();
        }
      return ok;
    }

    // TODO: add pop with move semantics
    bool pop(T &obj)
    {
      return pop(obj, std::chrono::milliseconds(portMAX_DELAY));
    }

    bool pop(T &obj, std::chrono::milliseconds timeout_duration)
    {
      bool ok = consume_sem.take(timeout_duration);
      if (ok)
        {
          {
            ScopedLock l(mutex);
            obj = queue_data.front();
            queue_data.erase(queue_data.begin());
          }
          produce_sem.give();
        }
      return ok;
    }

    int size() const
    {
      return max_size;
    }

    os::Semaphore::native_handle_type native_handle()
    {
      return consume_sem.native_handle();
    }


  private:
    os::Mutex mutex;
    os::Semaphore produce_sem;
    os::Semaphore consume_sem;
    std::vector<T> queue_data;
    int max_size = 0;
  };
}

#endif // OS_QUEUE_HPP
