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

#include "esp_log.h"

namespace os
{
  template <class T>
  class Slot;

  template<class... Args>
  class Slot<void(Args...)>
  {
    using callback_type = std::function<void(Args...)>;

  private:
    template<class F>
    class Closure
    {
    public:
      Closure() = default;

      Closure(const Closure&) = default;
      Closure &operator=(const Closure&) = default;

      Closure(Closure &&) = default;
      Closure &operator=(Closure &&) = default;

      // TODO: move semantics
      Closure(F &f, Args... args)
      {
        fn = f;
        tuple = std::tuple<Args...>(args...);
      }

      void operator()()
      {
        call_func_with_tuple(std::index_sequence_for<Args...>());
      }

    private:
      template<std::size_t... Is>
      void call_func_with_tuple(std::index_sequence<Is...>)
      {
        fn(std::get<Is>(tuple)...);
      }

      callback_type fn;
      std::tuple<Args...> tuple;
    };

    using closure_type = Closure<callback_type>;

  public:
    Slot(os::MainLoop &loop, std::function<void(Args...)> callback, int size = 10)
      : loop(loop), queue(size), callback(callback)
    {
    }

    Slot(const Slot&) = delete;
    Slot &operator=(const Slot&) = delete;

    Slot(Slot &&lhs)
      : loop(lhs.loop),
        queue(std::move(lhs.queue)),
        callback(std::move(lhs.callback))
    {
    }

    Slot &operator=(Slot &&lhs)
    {
      if (this != &lhs)
        {
          loop = lhs.loop;
          queue = std::move(lhs.queue);
          callback = std::move(lhs.callback);
        }
      return *this;
    }

    void operator()(Args const&... args)
    {
      // TODO: move semantics
      queue.push(closure_type(callback, args...));
    }

    void activate()
    {
      loop.register_queue(queue, std::bind(&Slot<void(Args...)>::execute, this));
    }

    void execute()
    {
      // TODO: move semantics
      closure_type p;
      if (queue.pop(p))
        {
          p();
        }
    }

  private:
    os::MainLoop &loop;
    os::Queue<closure_type> queue;
    std::function<void(Args...)> callback;
  };
}

#endif // OS_QUEUE_HPP
