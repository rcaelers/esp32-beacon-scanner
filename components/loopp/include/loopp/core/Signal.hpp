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

#ifndef LOOPP_CORE_SIGNAL_HPP
#define LOOPP_CORE_SIGNAL_HPP

#include <list>
#include <functional>

#include "loopp/core/Mutex.hpp"
#include "loopp/core/ScopedLock.hpp"

namespace loopp
{
  namespace core
  {
    template<class T>
    class Signal;

    template<class... Args>
    class Signal<void(Args...)>
    {
    public:
      using callback_type = std::function<void(Args...)>;
      using slot_list_type = std::list<callback_type>;

      Signal() = default;
      ~Signal() = default;

      Signal(const Signal &) = delete;
      Signal &operator=(const Signal &) = delete;

      Signal(Signal &&lhs)
        : mutex(std::move(lhs.mutex))
        , slots(std::move(lhs.slots))
      {
      }

      Signal &operator=(Signal &&lhs)
      {
        if (this != &lhs)
          {
            mutex = std::move(lhs.mutex);
            slots = std::move(lhs.slots);
          }
        return *this;
      }

      void connect(const callback_type callback)
      {
        ScopedLock l(mutex);
        slots.push_back(callback);
      }

      void operator()(const Args &... args)
      {
        ScopedLock l(mutex);
        for (auto &slot : slots)
          {
            slot(args...);
          }
      }

    private:
      mutable loopp::core::Mutex mutex;
      slot_list_type slots;
    };
  } // namespace core
} // namespace loopp

#endif // LOOPP_CORE_SIGNAL_HPP
