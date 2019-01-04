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

#ifndef LOOPP_LED_LEDMATRIX_HPP
#define LOOPP_LED_LEDMATRIX_HPP

#include <cstdint>
#include <vector>

#include "loopp/led/LedErrors.hpp"

namespace loopp
{
  namespace led
  {
    template<typename Derived>
    class GridLayout
    {
    public:
      using LayoutInit = GridLayout<Derived>;

    public:
      enum class Origin
      {
        TopLeft,
        BottomLeft,
        TopRight,
        BottomRight
      };

      enum class Direction
      {
        Horizontal,
        Vertical
      };

      enum class Sequence
      {
        Progressive,
        ZigZag
      };

      GridLayout(uint16_t width, uint16_t height)
        : GridLayout(width, height, Origin::TopLeft, Direction::Horizontal, Sequence::Progressive)
      {
      }

      GridLayout(uint16_t width, uint16_t height, Origin origin, Direction direction, Sequence sequence)
        : GridLayout(width, height, origin, direction, sequence, direction == Direction::Horizontal ? width : height)
      {
      }

      GridLayout(uint16_t width, uint16_t height, Origin origin, Direction direction, Sequence sequence, uint16_t stride)
        : width(width)
        , height(height)
        , origin(origin)
        , direction(direction)
        , sequence(sequence)
        , stride(stride)
      {
      }

      ~GridLayout() = default;
      GridLayout(const GridLayout &) = delete;
      GridLayout &operator=(const GridLayout &) = delete;
      GridLayout(GridLayout &&lhs) = default;
      GridLayout &operator=(GridLayout &&lhs) = default;

      uint16_t convert_xy(uint16_t x, uint16_t y)
      {
        if (x > width)
          {
            throw std::system_error(LedErrc::InternalError, "X coordinate out of range");
          }
        if (y > height)
          {
            throw std::system_error(LedErrc::InternalError, "Y coordinate out of range");
          }

        if (origin == Origin::TopRight || origin == Origin::BottomRight)
          {
            x = width - x - 1;
          }

        if (origin == Origin::BottomLeft || origin == Origin::BottomRight)
          {
            y = height - y - 1;
          }

        if (sequence == Sequence::ZigZag)
          {
            if (direction == Direction::Horizontal && (y % 2 == 1))
              {
                x = width - x - 1;
              }
            else if (direction == Direction::Vertical && (x % 2 == 1))
              {
                y = height - y - 1;
              }
          }

        if (direction == Direction::Vertical)
          {
            std::swap(x, y);
          }

        return y * stride + x;
      }

      void set_color_xy(uint16_t x, uint16_t y, const loopp::led::Color &color)
      {
        auto &derived = static_cast<Derived &>(*this);
        derived.set_color(convert_xy(x, y), color);
      }

    private:
      uint16_t width = 0;
      uint16_t height = 0;
      Origin origin = Origin::TopLeft;
      Direction direction = Direction::Horizontal;
      Sequence sequence = Sequence::Progressive;
      uint16_t stride = 0;
    };

  } // namespace led
} // namespace loopp

#endif // LOOPP_LED_LEDMATRIX_HPP
