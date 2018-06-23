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

#include "loopp/ble/BLEScanner.hpp"

#include <sstream>
#include <iomanip>
#include <cstring>

#ifdef CONFIG_BT_ENABLED

#include "esp_bt.h"
#include "esp_log.h"

#include "loopp/core/Task.hpp"

static const char *tag = "BLE";

using namespace loopp;
using namespace loopp::ble;

BLEScanner &
BLEScanner::instance()
{
  static BLEScanner instance;
  return instance;
}

BLEScanner::BLEScanner()
  : ble_scan_params()
{
  init();
}

void
BLEScanner::gap_event_handler_static(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  BLEScanner::instance().gap_event_handler(event, param);
}

void
BLEScanner::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
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
          switch (param->scan_rst.search_evt)
            {
              case ESP_GAP_SEARCH_INQ_RES_EVT:
                {
                  ScanResult beacon(&param->scan_rst);
                  signal_scan_result(beacon);
                  break;
                }

              case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                {
                  ESP_LOGI(tag, "Scan completed, restarting.");
                  signal_scan_complete();
                  esp_ble_gap_set_scan_params(&ble_scan_params);
                  break;
                }

              default:
                ESP_LOGI(tag, "Unhandled scan result %d.", param->scan_rst.search_evt);
                break;
            }
          break;
        }
      default:
        break;
    }
}

void
BLEScanner::init()
{
  ble_scan_params.scan_type = BLE_SCAN_TYPE_PASSIVE;
  ble_scan_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
  ble_scan_params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
  ble_scan_params.scan_interval = 0x50;
  ble_scan_params.scan_window = 0x30;

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);
  esp_bluedroid_init();
  esp_bluedroid_enable();
}

void
BLEScanner::deinit()
{
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
}

void
BLEScanner::set_scan_type(ScanType type)
{
  ble_scan_params.scan_type = (type == ScanType::Active) ? BLE_SCAN_TYPE_ACTIVE : BLE_SCAN_TYPE_PASSIVE;
}

void
BLEScanner::set_scan_interval(uint16_t interval)
{
  ble_scan_params.scan_interval = interval;
}

void
BLEScanner::set_scan_window(uint16_t window)
{
  ble_scan_params.scan_window = window;
}

void
BLEScanner::start()
{
  esp_ble_gap_register_callback(gap_event_handler_static);
  esp_ble_gap_set_scan_params(&ble_scan_params);
}

void
BLEScanner::stop()
{
  esp_ble_gap_stop_scanning();
}

loopp::core::Signal<void()> &
BLEScanner::scan_complete_signal()
{
  return signal_scan_complete;
}

loopp::core::Signal<void(loopp::ble::BLEScanner::ScanResult)> &
BLEScanner::scan_result_signal()
{
  return signal_scan_result;
}

BLEScanner::ScanResult::ScanResult(esp_ble_gap_cb_param_t::ble_scan_result_evt_param *scan_result)
  : adv_data(reinterpret_cast<char *>(scan_result->ble_adv), scan_result->adv_data_len)
  , rssi(scan_result->rssi)
{
  memcpy(bda, scan_result->bda, 6);
}

std::string
BLEScanner::ScanResult::bda_as_string()
{
  std::stringstream stream;
  stream << std::hex << std::setfill('0');
  stream << std::setw(2);

  for (int i = 0; i < 6; i++)
    {
      stream << static_cast<int>(bda[i]);
      if (i != 5)
        {
          stream << ':';
        }
    }

  return stream.str();
}

#endif
