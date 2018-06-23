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

#ifndef LOOPP_BLE_BLE__SCANNER_HPP
#define LOOPP_BLE_BLE__SCANNER_HPP

#include <string>

#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "loopp/core/Signal.hpp"

namespace loopp
{
  namespace ble
  {
    class BLEScanner
    {
    public:
      static BLEScanner &instance();

      struct ScanResult
      {
        ScanResult() = default;
        ScanResult(esp_ble_gap_cb_param_t::ble_scan_result_evt_param *scan_result);

        std::string bda_as_string();

      public:
        uint8_t bda[6];
        std::string adv_data;
        int rssi;
      };

      BLEScanner(const BLEScanner &) = delete;
      BLEScanner &operator=(const BLEScanner &) = delete;

      enum class ScanType
      {
        Active,
        Passive
      };

      void set_scan_type(ScanType type);
      void set_scan_interval(uint16_t interval);
      void set_scan_window(uint16_t window);
      void start();
      void stop();

      loopp::core::Signal<void()> &scan_complete_signal();
      loopp::core::Signal<void(ScanResult)> &scan_result_signal();

    private:
      BLEScanner();
      ~BLEScanner() = default;

      static void gap_event_handler_static(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
      void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

      void init();
      void deinit();
      void bt_task();

    private:
      loopp::core::Signal<void(void)> signal_scan_complete;
      loopp::core::Signal<void(ScanResult)> signal_scan_result;

      mutable loopp::core::Mutex mutex;
      esp_ble_scan_params_t ble_scan_params;

      const static int scan_duration = 30;
    };
  } // namespace ble
} // namespace loopp

#endif // LOOPP_BLE_BLE_SCANNER_HPP
