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

#ifndef BLESCANNERDRIVER_HH
#define BLESCANNERDRIVER_HH

#include <string>

#include "loopp/core/MainLoop.hpp"
#include "loopp/drivers/IDriver.hpp"
#include "loopp/drivers/DriverRegistry.hpp"
#include "loopp/mqtt/MqttClient.hpp"
#include "loopp/ble/BLEScanner.hpp"

#include "loopp/utils/json.hpp"

namespace loopp
{
  namespace drivers
  {
    class BLEScannerDriver : public loopp::drivers::IDriver, public std::enable_shared_from_this<BLEScannerDriver>
    {
    public:
      BLEScannerDriver(loopp::drivers::DriverContext context, nlohmann::json config);
      ~BLEScannerDriver();

    private:
      static std::string base64_encode(const std::string &in);

      void on_ble_scanner_scan_result(loopp::ble::BLEScanner::ScanResult result);
      void on_scan_timer();

      virtual void start() override;
      virtual void stop() override;

    private:
      std::shared_ptr<loopp::core::MainLoop> loop;
      std::shared_ptr<loopp::mqtt::MqttClient> mqtt;
      loopp::ble::BLEScanner &ble_scanner;
      loopp::core::MainLoop::timer_id scan_timer = 0;
      std::list<loopp::ble::BLEScanner::ScanResult> scan_results;
      std::string topic_scan;

      const static gpio_num_t LED_GPIO = GPIO_NUM_5;
    };

  } // namespace drivers
} // namespace loopp

#endif // BLESCANNERDRIVER_HH
