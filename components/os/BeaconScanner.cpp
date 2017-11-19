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

#include "os/BeaconScanner.hpp"

#ifdef CONFIG_BT_ENABLED

#include "bt.h"
#include "esp_bt_main.h"
#include "esp_log.h"

#include "os/Task.hpp"

static const char tag[] = "BLE";

using namespace os;

static esp_ble_scan_params_t ble_scan_params = {
  .scan_type              = BLE_SCAN_TYPE_PASSIVE,
  .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
  .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
  .scan_interval          = 0x50,
  .scan_window            = 0x30,
};

BeaconScanner::BeaconScanner()
{
  init();
}

BeaconScanner::~BeaconScanner()
{
}

void
BeaconScanner::gap_event_handler_static(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  BeaconScanner::instance().gap_event_handler(event, param);
}

void
BeaconScanner::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  switch (event)
    {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
      {
        ESP_LOGI(tag, "Scan param set complete, start scanning.");
        esp_ble_gap_start_scanning(scan_duration);
        break;
      }

    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
      if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
          ESP_LOGE(tag, "Scan start failed.");
        }
      else
        {
          ESP_LOGI(tag, "Scan start successfully.");
        }
      break;

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
      if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
          ESP_LOGE(tag, "Scan stop failed.");
        }
      else
        {
          ESP_LOGI(tag, "Scan stop successfully.");
        }
      break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT:
      {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt)
          {
          case ESP_GAP_SEARCH_INQ_RES_EVT:
            {
              static uint8_t ibeacon_prefix[] =
                {
                  0x02, 0x01, 0x00, 0x1A, 0xFF, 0x4C, 0x00, 0x02, 0x15
                };

              uint8_t *addr = scan_result->scan_rst.bda;

              scan_result->scan_rst.ble_adv[2]  = 0x00;
              for (int i = 0; i < sizeof(ibeacon_prefix); i++) {
                if (scan_result->scan_rst.ble_adv[i] != ibeacon_prefix[i]) {
                  return;
                }
              }

              char mac[18];
              sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

              signal_scan_result(ScanResult(scan_result->scan_rst.rssi, mac));
              break;
            }

          case ESP_GAP_SEARCH_INQ_CMPL_EVT: {
            ESP_LOGI(tag, "Scan completed, restarting.");
            signal_scan_complete();
            esp_ble_gap_set_scan_params(&ble_scan_params);
            break;
          }

          default:
            ESP_LOGI(tag, "Unhandled scan result %d.",  scan_result->scan_rst.search_evt);
            break;
          }
        break;
      }
    default:
      break;
    }
}

void
BeaconScanner::init()
{
  ESP_LOGI(tag, "BeaconScanner::init");
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);
  esp_bluedroid_init();
  esp_bluedroid_enable();
}

void
BeaconScanner::deinit()
{
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
}

void
BeaconScanner::start()
{
  esp_ble_gap_register_callback(gap_event_handler_static);
  esp_ble_gap_set_scan_params(&ble_scan_params);
}

void
BeaconScanner::stop()
{
  esp_ble_gap_stop_scanning();
}

os::Signal<void()> &
BeaconScanner::scan_complete_signal()
{
  return signal_scan_complete;
}

os::Signal<void(os::BeaconScanner::ScanResult)> &
BeaconScanner::scan_result_signal()
{
  return signal_scan_result;
}

#endif
