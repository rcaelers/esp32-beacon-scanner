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

#ifndef OS_SEMAPHORE_HPP
#define OS_SEMAPHORE_HPP

#include <chrono>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace os
{
  class Semaphore
  {
  public:
    typedef SemaphoreHandle_t native_handle_type;

    Semaphore(int max, int initial) noexcept
    {
      semaphore_handle = xSemaphoreCreateCounting(max, initial);
    }

    ~Semaphore()
    {
      if (semaphore_handle)
        {
          vSemaphoreDelete(semaphore_handle);
        }
    }

    Semaphore(const Semaphore&) = default;
    Semaphore& operator=(const Semaphore&) = default;

    Semaphore(Semaphore &&lhs) :
      semaphore_handle(lhs.semaphore_handle)
    {
      lhs.semaphore_handle = nullptr;
   }

    Semaphore &operator=(Semaphore &&lhs)
    {
      if (this != &lhs)
        {
          semaphore_handle = lhs.semaphore_handle;
          lhs.semaphore_handle = nullptr;
        }
      return *this;
    }

    bool take()
    {
      return take(std::chrono::milliseconds(portMAX_DELAY));
    }

    bool take(std::chrono::milliseconds timeout_duration)
    {
      return xSemaphoreTake(semaphore_handle, timeout_duration.count() / portTICK_PERIOD_MS) == pdTRUE;
    }

    bool give()
    {
      return xSemaphoreGive(semaphore_handle) == pdTRUE;
    }

    native_handle_type native_handle()
    {
      return semaphore_handle;
    }

  private:
    SemaphoreHandle_t semaphore_handle;
  };
}

#endif // OS_SEMAPHORE_HPP
