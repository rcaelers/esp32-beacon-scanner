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

#include <string>
#include <vector>
#include <algorithm>

#include "esp_log.h"

#include "DriverFactory.hpp"
#include "BLEScannerDriver.hpp"
#include "GPIODriver.hpp"

using json = nlohmann::json;

DriverFactory::DriverFactory(std::shared_ptr<loopp::core::MainLoop> loop, std::shared_ptr<loopp::mqtt::MqttClient> mqtt)
  : loop(loop)
  , mqtt(mqtt)
{
}

std::shared_ptr<IDriver>
DriverFactory::create(nlohmann::json config, std::string topic_root)
{
  std::shared_ptr<IDriver> driver;

  try
    {
      std::string driver_id = config["driver"].get<std::string>();

      if (driver_id == "ble-scanner")
        {
          driver = std::make_shared<BLEScannerDriver>(config, topic_root, loop, mqtt);
        }
      else if (driver_id == "gpio")
        {
          driver = std::make_shared<GPIODriver>(config, topic_root, loop, mqtt);
        }
    }
  catch (json::out_of_range &e)
    {
      ESP_LOGI("APP", "-> Invalid driver specification: %s", e.what());
    }

  return driver;
}
