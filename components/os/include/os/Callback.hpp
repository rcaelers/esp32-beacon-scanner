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

#ifndef OS_CALLBACK_HPP
#define OS_CALLBACK_HPP

#include "os/Slot.hpp"
#include "os/Mutex.hpp"
#include "os/ScopedLock.hpp"

#include "os/optional.hpp"

namespace os
{
  template <class T>
  class Callback;

  template<class... Args>
  class Callback<void(Args...)>
  {
  public:
    using slot_type = os::Slot<void(Args...)>;

    Callback() = default;
    ~Callback() = default;

    Callback(const Callback&) = delete;
    Callback& operator=(const Callback&) = delete;

    Callback(Callback &&lhs)
      : mutex(std::move(lhs.mutex)),
        callback(std::move(lhs.callback))
    {
    }

    Callback &operator=(Callback &&lhs)
    {
      if (this != &lhs)
        {
          mutex = std::move(lhs.mutex);
          callback = std::move(lhs.callback);
        }
      return *this;
    }

    void set(const slot_type &slot)
    {
      ScopedLock l(mutex);
      callback = slot;
      callback->activate();
    }

    void set(slot_type &&slot)
    {
      ScopedLock l(mutex);
      callback.emplace(std::move(slot));
      callback->activate();
    }

    void unset()
    {
      ScopedLock l(mutex);
      callback.reset();
    }

    void operator()(const Args &... args)
    {
      ScopedLock l(mutex);
      if (callback)
        {
          callback->operator()(args...);
        }
    }

  private:
    mutable os::Mutex mutex;
    nonstd::optional<slot_type> callback;
  };
}

#endif // OS_CALLBACK_HPP
