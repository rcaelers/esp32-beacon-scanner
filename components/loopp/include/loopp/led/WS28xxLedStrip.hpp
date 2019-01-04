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

#ifndef LOOPP_LED_WS28XXLEDSTRIP_HPP
#define LOOPP_LED_WS28XXLEDSTRIP_HPP

#include <cstdint>
#include <vector>

#include "soc/rmt_struct.h"
#include "driver/rmt.h"
#include "driver/gpio.h"

#include "loopp/led/LedStrip.hpp"

namespace loopp
{
  namespace led
  {
    struct WS28xxTiming
    {
      WS28xxTiming(uint32_t T0H, uint32_t T0L, uint32_t T1H, uint32_t T1L, uint32_t RES)
        : T0H(T0H)
        , T0L(T0L)
        , T1H(T1H)
        , T1L(T1L)
        , RES(RES)
      {
      }

      uint32_t T0H;
      uint32_t T0L;
      uint32_t T1H;
      uint32_t T1L;
      uint32_t RES;
    };

    template<typename Derived>
    class WS28xxDriver
    {
    private:
      using RmtDataType = std::vector<rmt_item32_t>;

    public:
      using DriverInit = WS28xxDriver<Derived>;

    public:
      WS28xxDriver() = default;

      WS28xxDriver(const WS28xxTiming &timing, rmt_channel_t channel, gpio_num_t pin)
        : channel(channel)
      {
        low.level0 = 1;
        low.level1 = 0;
        low.duration0 = timing.T0H / clock_duration;
        low.duration1 = timing.T0L / clock_duration;

        high.level0 = 1;
        high.level1 = 0;
        high.duration0 = timing.T1H / clock_duration;
        high.duration1 = timing.T1L / clock_duration;

        reset.level0 = 0;
        reset.level1 = 0;
        reset.duration0 = timing.RES / clock_duration - 1;
        reset.duration1 = 1;

        rmt_config_t config;

        config.channel = channel;
        config.rmt_mode = RMT_MODE_TX;
        config.gpio_num = pin;
        config.mem_block_num = 1;
        config.clk_div = clock_divider;

        config.tx_config.loop_en = false;
        config.tx_config.carrier_en = false;
        config.tx_config.carrier_freq_hz = 0;
        config.tx_config.carrier_duty_percent = 0;
        config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
        config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
        config.tx_config.idle_output_en = true;

        ESP_LOGI("ws28xx", "Init");

        esp_err_t err = rmt_config(&config);
        if (err != ESP_OK)
          {
            throw std::runtime_error("Cannot configure RMT");
          }

        err = rmt_driver_install(channel, 0, 0);
        if (err != ESP_OK)
          {
            throw std::runtime_error("Cannot configure RMT");
          }

        installed = true;
      }

      ~WS28xxDriver()
      {
        if (installed)
          {
            rmt_driver_uninstall(channel);
          }
      }

      WS28xxDriver(const WS28xxDriver &) = delete;
      WS28xxDriver &operator=(const WS28xxDriver &) = delete;

      WS28xxDriver(WS28xxDriver &&lhs)
        : channel(lhs.channel)
        , installed(lhs.installed)
        , high(lhs.high)
        , low(lhs.low)
        , reset(lhs.reset)
      {
        lhs.installed = false;
      }

      WS28xxDriver &operator=(WS28xxDriver &&lhs)
      {
        if (this != &lhs)
          {
            channel = lhs.channel;
            high = lhs.high;
            low = lhs.low;
            reset = lhs.reset;
            installed = lhs.installed;

            lhs.installed = false;
          }
        return *this;
      }

      void execute(const loopp::led::LedColorsType &led_colors, double scale)
      {
        rmt_data.assign(led_colors.size() * 24 + 1, reset);
        RmtDataType::iterator it = rmt_data.begin();
        for (const Color &c : led_colors)
          {
            Color scaled_color = c * scale;
            add_byte(it, scaled_color.green());
            add_byte(it, scaled_color.red());
            add_byte(it, scaled_color.blue());
          }
        rmt_data[rmt_data.size() - 1] = reset;

        ESP_ERROR_CHECK(rmt_write_items(channel, rmt_data.data(), rmt_data.size(), true));
      }

    private:
      void add_byte(RmtDataType::iterator &it, uint8_t value)
      {
        uint8_t mask = 0x80;
        while (mask > 0)
          {
            *it = (value & mask) != 0 ? high : low;
            mask >>= 1;
            ++it;
          }
      }

      static constexpr uint8_t clock_divider = 2;
      static constexpr uint16_t clock_duration = 25; // TODO: compute 12.5 * clock_divider

      rmt_channel_t channel;
      bool installed = false;
      rmt_item32_t high;
      rmt_item32_t low;
      rmt_item32_t reset;
      RmtDataType rmt_data;
    };

  } // namespace led
} // namespace loopp

#endif // LOOPP_LED_WS28XXLEDSTRIP_HPP
