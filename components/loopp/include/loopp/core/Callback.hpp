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

#ifndef LOOPP_CORE_CALLBACK_HPP
#define LOOPP_CORE_CALLBACK_HPP

#include "loopp/Mutex.hpp"
#include "loopp/ScopedLock.hpp"

#include "loopp/optional.hpp"

namespace loopp
{
  namespace core
  {
    template<class T>
    class Callback;

    template<class... Args>
    class Callback<void(Args...)>
    {
    public:
      using callback_type = std::function<void(Args...)>;

      Callback() = default;
      ~Callback() = default;

      Callback(const Callback &) = delete;
      Callback &operator=(const Callback &) = delete;

      Callback(Callback &&lhs) = default;
      Callback &operator=(Callback &&lhs) = default;

      void set(const callback_type &callback)
      {
        ScopedLock l(mutex);
        this->callback = callback;
      }

      void set(callback_type &&callback)
      {
        ScopedLock l(mutex);
        callback.emplace(std::move(callback));
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
            callback->call(args...);
          }
      }

    private:
      mutable loopp::core::Mutex mutex;
      nonstd::optional<callback_type> callback;
    };
  } // namespace core
} // namespace loopp

#endif // LOOPP_CORE_CALLBACK_HPP
