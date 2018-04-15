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

#include "loopp/drivers/BLEScannerDriver.hpp"

#include <string>
#include <vector>
#include <algorithm>

#include "esp_log.h"
#include "driver/gpio.h"

#include "loopp/ble/AdvertisementDecoder.hpp"
#include "loopp/drivers/DriverRegistry.hpp"
#include "loopp/utils/memlog.hpp"

using namespace loopp::drivers;

// LOOPP_REGISTER_DRIVER("ble-scanner", BLEScannerDriver);

using json = nlohmann::json;

static const char *tag = "BLE-SCANNER";

BLEScannerDriver::BLEScannerDriver(loopp::drivers::DriverContext context, const nlohmann::json &config)
  : loop(context.get_loop())
  , mqtt(context.get_mqtt())
  , ble_scanner(loopp::ble::BLEScanner::instance())
{
  topic_scan = context.get_topic_root() + "scan";
  ESP_LOGD(tag, "BLEScannerDriver");

  auto it = config.find("feedback_pin");
  if (it != config.end())
    {
      pin_no = static_cast<gpio_num_t>(*it);
      feedback = true;

      gpio_pad_select_gpio(pin_no);
      gpio_set_direction(pin_no, GPIO_MODE_OUTPUT);
    }

  it = config.find("scan_type");
  if (it != config.end())
    {
      std::string type = *it;

      if (type == "active")
        {
          ble_scanner.set_scan_type(loopp::ble::BLEScanner::ScanType::Active);
        }
      else if (type == "passive")
        {
          ble_scanner.set_scan_type(loopp::ble::BLEScanner::ScanType::Passive);
        }
      else
        {
          throw std::runtime_error("invalid scan_type value: " + type);
        }
    }

  it = config.find("scan_interval");
  if (it != config.end())
    {
      uint16_t interval = *it;
      ble_scanner.set_scan_interval(interval);
    }

  it = config.find("scan_window");
  if (it != config.end())
    {
      uint16_t window = *it;
      ble_scanner.set_scan_window(window);
    }
}

BLEScannerDriver::~BLEScannerDriver()
{
  ESP_LOGD(tag, "BLEScannerDriver~");
}

// https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
std::string
BLEScannerDriver::base64_encode(const std::string &in)
{
  std::string out;

  int val = 0, valb = -6;
  for (char c : in)
    {
      val = (val << 8) + c;
      valb += 8;
      while (valb >= 0)
        {
          out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
          valb -= 6;
        }
    }
  if (valb > -6)
    {
      out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
    }
  while ((out.size() % 4) != 0u)
    {
      out.push_back('=');
    }
  return out;
}

void
BLEScannerDriver::on_ble_scanner_scan_result(const loopp::ble::BLEScanner::ScanResult &result)
{
  if (feedback)
    {
      static int led_state = 0;
      led_state ^= 1;
      gpio_set_level(pin_no, led_state);
    }
  scan_results.push_back(result);
}

void
BLEScannerDriver::on_scan_timer()
{
  loopp::utils::memlog("BLEScannerDriver::on_scan_timer entry");
  try
    {
      json j;
      if (mqtt && mqtt->connected().get() && scan_results.size() > 0)
        {
          for (auto r : scan_results)
            {
              ESP_LOGI(tag, "on_scan_timer %s", r.bda_as_string().c_str());
              json jb;
              jb["mac"] = r.bda_as_string();
              jb["bda"] = base64_encode(std::string(reinterpret_cast<char *>(r.bda), sizeof(r.bda)));
              jb["rssi"] = r.rssi;
              jb["adv_data"] = base64_encode(r.adv_data);

              decoder.decode(r.adv_data, jb);
              j.push_back(jb);
            }

          mqtt->publish(topic_scan, j.dump());
        }
    }
  catch (std::exception &e)
    {
      ESP_LOGE(tag, "on_scan_timer %s", e.what());
    }
  scan_results.clear();
}

void
BLEScannerDriver::start()
{
  auto self = shared_from_this();
  scan_result_signal_connection = ble_scanner.scan_result_signal().connect(
    loopp::core::bind_loop(loop, [this, self](loopp::ble::BLEScanner::ScanResult scan_result) { on_ble_scanner_scan_result(scan_result); }));
  scan_timer = loop->add_periodic_timer(std::chrono::milliseconds(1000), [this, self]() { on_scan_timer(); });
  ble_scanner.start();
}

void
BLEScannerDriver::stop()
{
  loop->cancel_timer(scan_timer);
  scan_timer = 0;
  ble_scanner.stop();
  scan_result_signal_connection.disconnect();
}
