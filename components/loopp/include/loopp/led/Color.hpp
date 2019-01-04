// Copyright (C) 2018 Rob Caelers <rob.caelers@gmail.com>
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

#ifndef LOOPP_LED_COLOR_HPP
#define LOOPP_LED_COLOR_HPP

#include <cstdint>
#include <cmath>

#include "boost/operators.hpp"

namespace loopp
{
  namespace led
  {
    class Color : boost::equality_comparable<Color>
    {
    public:
      Color()
        : red_(0)
        , green_(0)
        , blue_(0)
      {
      }

      Color(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
        : red_(red)
        , green_(green)
        , blue_(blue)
      {
      }

      Color(const Color &other) = default;
      Color(Color &&other) = default;
      Color &operator=(const Color &other) = default;
      Color &operator=(Color &&other) = default;
      ~Color() = default;

      bool operator==(Color const &rhs) const
      {
        return (red_ == rhs.red()) && (green_ == rhs.green()) && (blue_ == rhs.blue());
      }

      std::uint8_t red() const
      {
        return red_;
      }

      std::uint8_t green() const
      {
        return green_;
      }

      std::uint8_t blue() const
      {
        return blue_;
      }

      void set_red(std::uint8_t red)
      {
        red_ = red;
      }

      void set_green(std::uint8_t green)
      {
        green_ = green;
      }

      void set_blue(std::uint8_t blue)
      {
        blue_ = blue;
      }

      unsigned rgb() const
      {
        return static_cast<unsigned>((blue_ << 16) | (green_ << 8) | (red_));
      }

      Color operator*(double value) const
      {
        value = std::max(0.0, std::min(value, 1.0));
        return Color(std::round(red_ * value), std::round(green_ * value), std::round(blue_ * value));
      }

      Color gamma_correct(const double &gamma)
      {
        return Color(pow(red_, 1.0 / gamma), pow(green_, 1.0 / gamma), pow(blue_, 1.0 / gamma));
      }

    private:
      std::uint8_t red_;
      std::uint8_t green_;
      std::uint8_t blue_;
    };

  } // namespace led
} // namespace loopp

#endif // LOOPP_LED_COLOR_HPP
