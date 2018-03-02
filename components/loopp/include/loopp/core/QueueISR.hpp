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

#ifndef LOOPP_CORE_QUEUEISR_HPP
#define LOOPP_CORE_QUEUEISR_HPP

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
    class QueueISR
    {
      static_assert(std::is_default_constructible<T>::value, "Template parameter T must be default constructible");
      static_assert(std::is_trivial<T>::value, "Template parameter T must be a trivial type");

    public:
      QueueISR(int max_size = 10)
      {
        queue_handle = xQueueCreate(max_size, sizeof(T));
      }

      ~QueueISR()
      {
        if (queue_handle)
          {
            vQueueDelete(queue_handle);
          }
      }

      QueueISR(const QueueISR &) = delete;
      QueueISR &operator=(const QueueISR &) = delete;
      QueueISR(QueueISR &&lhs) = delete;
      QueueISR &operator=(QueueISR &&lhs) = delete;

      void IRAM_ATTR push(const T &obj)
      {
        xQueueSendFromISR(queue_handle, &obj, NULL);
      }

      bool pop(T &obj)
      {
        return xQueueReceive(queue_handle, &obj, portMAX_DELAY) == pdTRUE;
      }

      nonstd::optional<T> pop()
      {
        nonstd::optional<T> ret;
        T obj;
        bool ok = xQueueReceive(queue_handle, &obj, 0) == pdTRUE;

        if (ok)
          {
            ret = nonstd::optional<T>(std::move(obj));
          }

        return ret;
      }

    private:
      QueueHandle_t queue_handle;
    };

  } // namespace core
} // namespace loopp

#endif // LOOPP_CORE_QUEUEISR_HPP
