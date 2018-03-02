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

#ifndef LOOPP_CORE_QUEUE_HPP
#define LOOPP_CORE_QUEUE_HPP

#include <chrono>
#include <deque>

#include "loopp/utils/optional.hpp"

#include "loopp/core/Mutex.hpp"
#include "loopp/core/Semaphore.hpp"
#include "loopp/core/ScopedLock.hpp"

namespace loopp
{
  namespace core
  {
    template<typename T>
    class Queue
    {
    public:
      Queue(int max_size = 100)
        : produce_sem(max_size, max_size)
        , consume_sem(max_size, 0)
        , max_size(max_size)
      {
      }

      ~Queue() = default;
      Queue(const Queue &) = delete;
      Queue &operator=(const Queue &) = delete;

      Queue(Queue &&lhs)
        : mutex(std::move(lhs.mutex))
        , produce_sem(std::move(lhs.produce_sem))
        , consume_sem(std::move(lhs.consume_sem))
        , queue_data(std::move(lhs.queue_data))
        , max_size(lhs.max_size)
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
        return push_for(obj, std::chrono::milliseconds(portMAX_DELAY));
      }

      bool push_for(const T &obj, std::chrono::milliseconds timeout_duration)
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
        return push_for(std::move(obj), std::chrono::milliseconds(portMAX_DELAY));
      }

      bool push_for(T &&obj, std::chrono::milliseconds timeout_duration)
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

      template<class... Args>
      bool emplace(Args &&... args)
      {
        return emplace_for(std::chrono::milliseconds(portMAX_DELAY), std::forward<Args>(args)...);
      }

      template<class... Args>
      bool emplace_for(std::chrono::milliseconds timeout_duration, Args &&... args)
      {
        bool ok = produce_sem.take(timeout_duration);
        if (ok)
          {
            {
              ScopedLock l(mutex);
              queue_data.emplace_back(std::forward<Args>(args)...);
            }
            consume_sem.give();
          }
        return ok;
      }

      bool pop(T &obj)
      {
        return pop_for(obj, std::chrono::milliseconds(portMAX_DELAY));
      }

      bool pop_for(T &obj, std::chrono::milliseconds timeout_duration)
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

      nonstd::optional<T> pop()
      {
        return pop_for(std::chrono::milliseconds(portMAX_DELAY));
      }

      nonstd::optional<T> pop_for(std::chrono::milliseconds timeout_duration)
      {
        bool ok = consume_sem.take(timeout_duration);
        nonstd::optional<T> ret;
        if (ok)
          {
            {
              ScopedLock l(mutex);
              ret = nonstd::optional<T>(std::move(queue_data.front()));
              queue_data.erase(queue_data.begin());
            }
            produce_sem.give();
          }

        return ret;
      }

      int size() const
      {
        ScopedLock l(mutex);
        return queue_data.size();
      }

    private:
      mutable loopp::core::Mutex mutex;
      loopp::core::Semaphore produce_sem;
      loopp::core::Semaphore consume_sem;
      std::deque<T> queue_data;
      int max_size = 0;
    };
  } // namespace core
} // namespace loopp

#endif // LOOPP_CORE_QUEUE_HPP
