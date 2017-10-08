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

#ifndef OS_CLOSURE_HPP
#define OS_CLOSURE_HPP

#include "os/MainLoop.hpp"

namespace os
{
  class ClosureBase
  {
  public:
    virtual void operator()() = 0;
  };

  template<typename F, typename... Args>
  class Closure : public ClosureBase
  {
  public:
    using function_type = std::function<void(Args...)>;
    using tuple_type = std::tuple<Args...>;

    Closure() = default;
    ~Closure() = default;

    Closure(Closure &&lhs)
      : fn(std::move(lhs.fn)),
        args(std::move(lhs.args))
    {
    }

    Closure &operator=(Closure &&lhs)
    {
      if (this != &lhs)
        {
          fn = std::move(lhs.fn);
          args = std::move(lhs.args);
        }
      return *this;
    }

    Closure(const Closure &lhs)
      : fn(lhs.fn),
        args(lhs.args)
    {
    }

    Closure &operator=(const Closure &lhs)
    {
      if (this != &lhs)
        {
          fn = lhs.fn;
          args = lhs.args;
        }
      return *this;
    }

    Closure(F fn, Args... args) :
      fn(fn),
      args(std::make_tuple(std::move(args)...))
    {
    }

    void operator()()
    {
      invoke_helper(std::index_sequence_for<Args...>());
    }

  private:
    template<std::size_t... Is>
    void invoke_helper(std::index_sequence<Is...>)
    {
      fn(std::get<Is>(args)...);
    }

  private:
    function_type fn;
    std::tuple<Args...> args;
  };

  template<typename F, typename... Args>
  auto make_closure(F &&fn, Args&&... args)
  {
    return Closure<F, Args...>(std::forward<F>(fn), std::forward<Args>(args)...);
  }
}

#endif // OS_CLOSURE_HPP
