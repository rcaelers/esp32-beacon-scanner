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

#ifndef OS_SIGNAL_HPP
#define OS_SIGNAL_HPP

#include <list>

#include "os/Slot.hpp"
#include "os/Mutex.hpp"
#include "os/ScopedLock.hpp"

namespace os
{
  template <class T>
  class Signal;

  template<class... Args>
  class Signal<void(Args...)>
  {
  public:
    using slot_type = os::Slot<void(Args...)>;

    Signal() = default;
    ~Signal() = default;

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
    Signal(Signal &&slot) = default;
    Signal &operator=(Signal &&lhs) = default;

    // TODO: hide slot from API
    void connect(const slot_type &slot)
    {
      ScopedLock l(mutex);
      slots.push_back(slot);
      slots.back().activate();
    }

    void connect(slot_type &&slot)
    {
      ScopedLock l(mutex);
      slots.push_back(std::move(slot));
      slots.back().activate();
    }

    void operator()(Args const&... args)
    {
      ScopedLock l(mutex);
      for (auto &slot : slots)
        {
          slot.operator()(args...);
        }
    }

  private:
    mutable os::Mutex mutex;
    std::vector<slot_type> slots;
  };
}

#endif // OS_SIGNAL_HPP
