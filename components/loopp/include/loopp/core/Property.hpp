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

#ifndef LOOPP_CORE_PROPERTY_HPP
#define LOOPP_CORE_PROPERTY_HPP

#include "loopp/core/Signal.hpp"

namespace loopp
{
  namespace core
  {
    template<class T>
    class Property : public Signal<void(T)>
    {
    public:
      Property(T initial) : value(initial) {}
      ~Property() = default;

      Property(const Property&) = delete;
      Property& operator=(const Property&) = delete;

      Property(Property &&lhs) :
        value(std::move(lhs.value))
      {
      }

      Property &operator=(Property &&lhs)
      {
        if (this != &lhs)
          {
            value = std::move(lhs.value);
          }
        return *this;
      }

      const T &get() const
      {
        return value;
      }

      void set(const T &new_value)
      {
        if (value != new_value)
          {
            value = new_value;
            this->operator()(value);
          }
      }

      void set(T &&new_value)
      {
        if (value != new_value)
          {
            value = std::move(new_value);
            this->operator()(value);
          }
      }

    private:
      T value;
    };
  }
}

#endif // LOOPP_CORE_PROPERTY_HPP
