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

#ifndef OS_BEACON_SCANNER_HPP
#define OS_BEACON_SCANNER_HPP

#include <string>

#include "esp_gap_ble_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "os/Task.hpp"
#include "os/Signal.hpp"

namespace os
{
  class BeaconScanner
  {
  public:
    static BeaconScanner &instance()
    {
      static BeaconScanner instance;
      return instance;
    }

    struct ScanResult
    {
      ScanResult() = default;
      ScanResult(int rssi, std::string mac)
        : rssi(rssi), mac(mac)
      {
      }


      ScanResult(ScanResult &&lhs)
        : rssi(std::move(lhs.rssi)),
          mac(std::move(lhs.mac))
      {
      }

      ScanResult &operator=(ScanResult &&lhs)
      {
        if (this != &lhs)
          {
            rssi = std::move(lhs.rssi);
            mac = std::move(lhs.mac);
          }
        return *this;
      }

      ScanResult(const ScanResult &lhs)
        : rssi(lhs.rssi),
          mac(lhs.mac)
      {
      }

      ScanResult &operator=(const ScanResult &lhs)
      {
        if (this != &lhs)
          {
            rssi = lhs.rssi;
            mac = lhs.mac;
          }
        return *this;
      }

      int rssi;
      // TODO: use custom Mac address class.
      std::string mac;
    };

    BeaconScanner(const BeaconScanner&) = delete;
    BeaconScanner& operator=(const BeaconScanner&) = delete;

    void start();
    void stop();

    os::Signal<void()> &scan_complete_signal();
    os::Signal<void(ScanResult)> &scan_result_signal();

  private:
    BeaconScanner();
    ~BeaconScanner();

    static void gap_event_handler_static(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

    void init();
    void deinit();
    void bt_task();

  private:
    os::Signal<void(void)> signal_scan_complete;
    os::Signal<void(ScanResult)> signal_scan_result;

    mutable os::Mutex mutex;
    EventGroupHandle_t event_group;
    os::Task task;

    const static int scan_duration = 10;
    const static int RESTART_BIT = BIT0;
    const static int READY_BIT = BIT1;
  };
}

#endif // OS_BEACON_SCANNER_HPP
