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

#include "os/Wifi.hpp"
#include "os/BeaconScanner.hpp"
#include "os/Slot.hpp"
#include "os/Task.hpp"
#include "os/MainLoop.hpp"

#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "user_config.h"

static const char tag[] = "BEACON-SCANNER";

class Main
{
public:
  Main()
    : beacon_scanner(os::BeaconScanner::instance()),
      task("main_task", std::bind(&Main::main_task, this))
  {
    gpio_pad_select_gpio(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  }

private:
  void on_wifi_system_event(system_event_t event)
  {
    ESP_LOGI(tag, "-> System event %d", event.event_id);
  }

  void on_wifi_connected(bool connected)
  {
    if (connected)
      {
        ESP_LOGI(tag, "-> Wifi connected");
        beacon_scanner.start();
      }
    else
      {
        ESP_LOGI(tag, "-> Wifi disconnected");
        beacon_scanner.stop();
      }
  }

  void on_beacon_scanner_scan_result(os::BeaconScanner::ScanResult result)
  {
    static int led_state = 0;

    ESP_LOGI(tag, "-> BT result %s %d", result.mac.c_str(), result.rssi);
    led_state ^= 1;
    gpio_set_level(LED_GPIO, led_state);
  }

  void main_task()
  {
    wifi.set_ssid(WIFI_SSID);
    wifi.set_passphase(WIFI_PASS);
    wifi.set_host_name("scan");
    wifi.set_auto_connect(true);
    wifi.system_event_signal().connect(os::Slot<void(system_event_t)>(loop, std::bind(&Main::on_wifi_system_event, this, std::placeholders::_1)));
    wifi.connected().connect(os::Slot<void(bool)>(loop, std::bind(&Main::on_wifi_connected, this, std::placeholders::_1)));

    beacon_scanner.scan_result_signal().connect(os::Slot<void(os::BeaconScanner::ScanResult)>(loop, std::bind(&Main::on_beacon_scanner_scan_result, this, std::placeholders::_1)));

    wifi.connect();

    loop.run();
  }

  os::BeaconScanner &beacon_scanner;
  os::Wifi wifi;
  os::MainLoop loop;
  os::Task task;

  const static gpio_num_t LED_GPIO = GPIO_NUM_5;
};

extern "C" void
app_main()
{
  nvs_flash_init();

  Main main;

  while(1)
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
