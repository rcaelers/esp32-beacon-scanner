// Copyright (C) 2018, 2019 Rob Caelers <rob.caelers@gmail.com>
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

#ifndef LOOPP_LED_CURRENTLIMITER_HPP
#define LOOPP_LED_CURRENTLIMITER_HPP

#include <cstdint>
#include <stdexcept>
#include <utility>

#include "esp_log.h"

#include "loopp/led/Color.hpp"

namespace loopp
{
  namespace led
  {
    template<typename Derived>
    class CurrentLimiter
    {
    public:
      using CurrentLimiterInit = CurrentLimiter<Derived>;

      CurrentLimiter() = default;
      ~CurrentLimiter() = default;

      CurrentLimiter(const CurrentLimiter &) = delete;
      CurrentLimiter &operator=(const CurrentLimiter &) = delete;

      CurrentLimiter(CurrentLimiter &&lhs) = default;
      CurrentLimiter &operator=(CurrentLimiter &&lhs) = default;

    public:
      void set_current_usage(double base, double red, double green, double blue)
      {
        current_base = base;
        current_red = red;
        current_green = green;
        current_blue = blue;
      }

      void set_current_budget(double current_in_mA)
      {
        current_budget = current_in_mA;
      }

      void apply_hook()
      {
        double scale = 1;
        if (current_budget > 0)
          {
            double current = calculate_current();
            if (current > current_budget)
              {
                scale = current_budget / current;
                ESP_LOGD("leds", "Current demand exceeds budget (%f > %f). Scaling factor = %f", current, current_budget, scale);
              }
          }

        auto &derived = static_cast<Derived &>(*this);
        derived.set_brighness_scale(scale);
      }

    private:
      double calculate_current() const
      {
        std::uint32_t red_sum = 0;
        std::uint32_t green_sum = 0;
        std::uint32_t blue_sum = 0;

        auto &derived = static_cast<const Derived &>(*this);

        for (const Color &c : derived.led_colors)
          {
            red_sum += c.red();
            green_sum += c.green();
            blue_sum += c.blue();
          }

        return red_sum * current_red + green_sum * current_green + blue_sum * current_blue + derived.number_of_leds * current_base;
      }

    private:
      double current_budget = 0;
      double current_base = 1;
      double current_red = 20.0 / 255;
      double current_green = 20.0 / 255;
      double current_blue = 20.0 / 255;
    };
  } // namespace led
} // namespace loopp

#endif // LOOPP_LED_CURRENTLIMITER_HPP
