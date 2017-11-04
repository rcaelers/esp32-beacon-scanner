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

#ifndef OS_SLOT_HPP
#define OS_SLOT_HPP

#include "os/MainLoop.hpp"
#include "os/Closure.hpp"
#include "os/lambda.hpp"

namespace os
{
  template <typename T>
  class Slot;

  template<typename... Args>
  class Slot<void(Args...)>
  {
  private:
    using callback_type = std::function<void(Args...)>;
    using closure_type = os::Closure<callback_type, Args...>;

  public:
    Slot(std::shared_ptr<os::MainLoop> loop, callback_type callback)
      : loop(loop), callback(callback)
    {
    }

    Slot(const Slot&) = default;
    Slot &operator=(const Slot&) = default;

    Slot(Slot &&lhs)
      : loop(lhs.loop),
        callback(std::move(lhs.callback))
    {
    }

    Slot &operator=(Slot &&lhs)
    {
      if (this != &lhs)
        {
          loop = lhs.loop;
          callback = std::move(lhs.callback);
        }
      return *this;
    }

    void call(Args... args) const
    {
      if (loop)
        {
          loop->post(std::make_shared<closure_type>(callback, std::move(args)...));
        }
      else
        {
          callback(std::move(args)...);
        }
    }

  private:
    std::shared_ptr<os::MainLoop> loop;
    std::function<void(Args...)> callback;
  };

  template<typename F>
  auto make_slot(std::shared_ptr<os::MainLoop> loop, F &&func)
  {
    using fpointer_type = get_signature<F>;

    return Slot<fpointer_type>(loop, std::forward<std::function<fpointer_type>>(func));
  }

}

#endif // OS_SLOT_HPP
