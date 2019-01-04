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

#if __cplusplus > 201703L

#include "loopp/drivers/LedStripDriver.hpp"

#include <string>
#include <vector>
#include <algorithm>

#include "esp_log.h"
#include "driver/gpio.h"

#include "loopp/ble/AdvertisementDecoder.hpp"
#include "loopp/drivers/DriverRegistry.hpp"
#include "loopp/utils/memlog.hpp"

using namespace loopp::drivers;

// LOOPP_REGISTER_DRIVER("ledstrip", LedStripDriver);

using json = nlohmann::json;

static const char *tag = "BLE-SCANNER";

LedStripDriver::LedStripDriver(loopp::drivers::DriverContext context, const nlohmann::json &config)
  : loop(context.get_loop())
  , mqtt(context.get_mqtt())
  , ble_scanner(loopp::ble::LedStrip::instance())
{
  auto it = config.find("number_of_leds");
  if (it != config.end())
    {
      number_of_leds = static_cast<int>(*it);
    }

  it = config.find("pin");
  if (it != config.end())
    {
      pin_no = static_cast<gpio_num_t>(*it);
    }

  it = config.find("current_budget");
  if (it != config.end())
    {
      current_budget = static_cast<int>(*it);
    }

  it = config.find("width");
  if (it != config.end())
    {
      width = static_cast<gpio_num_t>(*it);
    }

  it = config.find("height");
  if (it != config.end())
    {
      width = static_cast<gpio_num_t>(*it);
    }
}

LedStripDriver::~LedStripDriver()
{
}

void
LedStripDriver::start()
{
  // TODO: make hardcoded WS2813 timing configurable.
  loopp::led::WS28xxTiming timing(300, 1090, 1090, 320, 290000);

  leds = std::make_shared<Leds>(60,
                                Leds::DriverInit { timing, RMT_CHANNEL_0, pin_no },
                                Leds::LayoutInit { width, height },
                                Leds::CurrentLimiterInit {});

  if (current_budget != 0)
    {
      leds->set_current_budget(current_budget);
    }
  leds->clear();

}

void
LedStripDriver::stop()
{
}

#endif
