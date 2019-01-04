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

#ifndef LEDSTRIPDRIVER_HH
#define LEDSTRIPDRIVER_HH

#if __cplusplus > 201703L
// Requires C++17

#include <string>

#include "loopp/led/WS28xxLedStrip.hpp"
#include "loopp/led/GridLayout.hpp"
#include "loopp/led/CurrentLimiter.hpp"

#include "loopp/core/MainLoop.hpp"
#include "loopp/drivers/IDriver.hpp"
#include "loopp/drivers/DriverRegistry.hpp"

#include "loopp/utils/json.hpp"

namespace loopp
{
  namespace drivers
  {
    class LedStripDriver
      : public loopp::drivers::IDriver
      , public std::enable_shared_from_this<LedStripDriver>
    {
    public:
      LedStripDriver(loopp::drivers::DriverContext context, const nlohmann::json &config);
      ~LedStripDriver();

    private:
      static std::string base64_encode(const std::string &in);

      void on_ble_scanner_scan_result(const loopp::ble::LedStrip::ScanResult &result);
      void on_scan_timer();

      virtual void start() override;
      virtual void stop() override;

    private:
      using Leds = loopp::led::LedStrip<loopp::led::WS28xxDriver, loopp::led::GridLayout, loopp::led::CurrentLimiter>;

      std::shared_ptr<loopp::core::MainLoop> loop;
      std::shared_ptr<loopp::mqtt::MqttClient> mqtt;
      std::shared_ptr<Leds> leds;

      uint16_t number_of_leds = 0;
      gpio_num_t pin_no = 0;
      uint16_t current_budget = 0;
      uint16_t width = 0;
      uint16_t height = 0;
    };

  } // namespace drivers
} // namespace loopp

#endif // C++17

#endif // LEDSTRIPDRIVER_HH
