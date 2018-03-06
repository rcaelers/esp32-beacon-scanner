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

#ifndef LOOPP_CORE_TASK_HPP
#define LOOPP_CORE_TASK_HPP

#include <string>
#include <functional>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace loopp
{
  namespace core
  {
    class Task
    {
    public:
      typedef TaskHandle_t native_handle_type;

      enum class CoreId : BaseType_t
      {
        CPU0 = 0,
        CPU1 = 1,
        NoAffinity = tskNO_AFFINITY
      };

      Task(const std::string &name, std::function<void()>, CoreId core_id = CoreId::NoAffinity, uint16_t stack_size = 8192, UBaseType_t priority = 5);
      ~Task();

      Task(const Task &) = delete;
      Task &operator=(const Task &) = delete;

      native_handle_type native_handle()
      {
        return task_handle;
      }

    private:
      static void run(void *self);

    private:
      const std::string name;
      std::function<void()> func;
      native_handle_type task_handle = nullptr;
    };
  } // namespace core
} // namespace loopp

#endif // LOOPP_CORE_TASK_HPP
