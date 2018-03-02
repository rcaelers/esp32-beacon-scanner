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

#ifndef LOOPP_CORE_EVENTGROUP_HPP
#define LOOPP_CORE_EVENTGROUP_HPP

#include <chrono>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

namespace loopp
{
  namespace core
  {
    class EventGroup
    {
    public:
      typedef EventGroupHandle_t native_handle_type;

      EventGroup() noexcept
      {
        event_group = xEventGroupCreate();
      }

      ~EventGroup()
      {
        if (event_group)
          {
            vEventGroupDelete(event_group);
          }
      }

      EventGroup(const EventGroup &) = delete;
      EventGroup &operator=(const EventGroup &) = delete;

      EventGroup(EventGroup &&lhs)
        : event_group(lhs.event_group)
      {
        lhs.event_group = nullptr;
      }

      EventGroup &operator=(EventGroup &&lhs)
      {
        if (this != &lhs)
          {
            event_group = lhs.event_group;
            lhs.event_group = nullptr;
          }
        return *this;
      }

      void set(int bit)
      {
        xEventGroupSetBits(event_group, bit);
      }

      void clear(int bit)
      {
        xEventGroupClearBits(event_group, bit);
      }

      void wait_and_clear(int bit, std::chrono::milliseconds timeout_duration)
      {
        xEventGroupWaitBits(event_group, bit, false, true, timeout_duration.count() / portTICK_PERIOD_MS);
      }

      void wait(int bit, std::chrono::milliseconds timeout_duration)
      {
        xEventGroupWaitBits(event_group, bit, false, true, timeout_duration.count() / portTICK_PERIOD_MS);
      }

      void wait_and_clear(int bit)
      {
        wait_and_clear(bit, std::chrono::milliseconds(portMAX_DELAY));
      }

      void wait(int bit)
      {
        wait(bit, std::chrono::milliseconds(portMAX_DELAY));
      }

      native_handle_type native_handle()
      {
        return event_group;
      }

    private:
      EventGroupHandle_t event_group;
    };
  } // namespace core
} // namespace loopp

#endif // LOOPP_CORE_EVENTGROUP_HPP
