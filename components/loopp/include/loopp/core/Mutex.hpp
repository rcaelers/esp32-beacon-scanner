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

#ifndef LOOPP_CORE_MUTEX_HPP
#define LOOPP_CORE_MUTEX_HPP

#include <chrono>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace loopp
{
  namespace core
  {
    class Mutex
    {
    public:
      typedef SemaphoreHandle_t native_handle_type;

      Mutex() noexcept
      {
        mutex_handle = xSemaphoreCreateMutex();
      }

      ~Mutex()
      {
        if (mutex_handle)
          {
            vSemaphoreDelete(mutex_handle);
          }
      }

      Mutex(const Mutex &) = delete;
      Mutex &operator=(const Mutex &) = delete;

      Mutex(Mutex &&lhs)
        : mutex_handle(lhs.mutex_handle)
      {
        lhs.mutex_handle = nullptr;
      }

      Mutex &operator=(Mutex &&lhs)
      {
        if (this != &lhs)
          {
            mutex_handle = lhs.mutex_handle;
            lhs.mutex_handle = nullptr;
          }
        return *this;
      }

      void lock()
      {
        assert(mutex_handle != nullptr);
        xSemaphoreTake(mutex_handle, portMAX_DELAY);
      }

      bool try_lock()
      {
        return try_lock(std::chrono::milliseconds(0));
      }

      bool try_lock(std::chrono::milliseconds timeout_duration)
      {
        assert(mutex_handle != nullptr);
        return xSemaphoreTake(mutex_handle, timeout_duration.count() / portTICK_PERIOD_MS) == pdTRUE;
      }

      void unlock()
      {
        assert(mutex_handle != nullptr);
        xSemaphoreGive(mutex_handle);
      }

      native_handle_type native_handle()
      {
        return mutex_handle;
      }

    private:
      SemaphoreHandle_t mutex_handle;
    };
  } // namespace core
} // namespace loopp

#endif // LOOPP_CORE_MUTEX_HPP
