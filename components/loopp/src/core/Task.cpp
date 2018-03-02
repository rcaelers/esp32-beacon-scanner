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

#include <system_error>

#include "loopp/core/Task.hpp"

using namespace loopp;
using namespace loopp::core;

Task::Task(const std::string &name, std::function<void()> func, CoreId core_id, uint16_t stack_size, UBaseType_t priority)
  : name(name)
  , func(func)
  , stack_size(stack_size)
  , priority(priority)
{
  BaseType_t rc = pdPASS;
  if (core_id != CoreId::NoAffinity)
    {
      rc = xTaskCreate(&Task::run, name.c_str(), stack_size, this, priority, &task_handle);
    }
  else
    {
      rc = xTaskCreatePinnedToCore(&Task::run, name.c_str(), stack_size, this, priority, &task_handle, static_cast<std::underlying_type_t<CoreId>>(core_id));
    }

  if (rc != pdPASS)
    {
      throw std::errc::resource_unavailable_try_again;
    }
}

Task::~Task()
{
  vTaskDelete(task_handle);
}

void
Task::run(void *self)
{
  static_cast<Task *>(self)->func();
  vTaskDelete(nullptr);
}
