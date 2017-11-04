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

#include <iostream>
#include <string>

#include "os/Wifi.hpp"
#include "os/BeaconScanner.hpp"
#include "os/Slot.hpp"
#include "os/Task.hpp"
#include "os/MainLoop.hpp"
#include "os/Mqtt.hpp"
#include "os/TLSStream.hpp"

extern "C"
{
#include "esp_heap_caps.h"
}
#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "user_config.h"

static const char tag[] = "BEACON-SCANNER";

extern const uint8_t ca_start[] asm("_binary_CA_crt_start");
extern const uint8_t ca_end[] asm("_binary_CA_crt_end");
extern const uint8_t certificate_start[] asm("_binary_esp32_crt_start");
extern const uint8_t certificate_end[] asm("_binary_esp32_crt_end");
extern const uint8_t private_key_start[] asm("_binary_esp32_key_start");
extern const uint8_t private_key_end[] asm("_binary_esp32_key_end");

class Main
{
public:
  Main():
#ifdef  CONFIG_BT_ENABLED
    beacon_scanner(os::BeaconScanner::instance()),
#endif
    wifi(os::Wifi::instance()),
    loop(std::make_shared<os::MainLoop>()),
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

  void on_socket_io()
  {
    try
      {
        ESP_LOGI(tag, "on_socket_io");

        try
          {
            unsigned char buf[200];
            size_t bytes_read = 0;
            sock.read(buf, sizeof(buf), bytes_read);
            ESP_LOGI(tag, "on_socket_io: read %d", bytes_read);

            if (bytes_read == 0)
              {
                sock.close();
              }
          }
        catch(std::exception &e)
          {
            ESP_LOGI(tag, "on_socket_io: exception");
          }
      }
    catch(...)
      {
        ESP_LOGI(tag, "on_socket_io: exception");
        sock.close();
      }
  }

  void on_wifi_connected(bool connected)
  {
    if (connected)
      {
        ESP_LOGI(tag, "-> Wifi connected");
        try
          {
            sock.io_callback().set(os::Slot<void()>(loop, std::bind(&Main::on_socket_io, this)));
            sock.connect(MQTT_HOST, 8883);
          }
        catch(std::exception &e)
          {
            ESP_LOGE(tag, "Socket connect exception: %s", e.what());
          }
      }
    else
      {
        ESP_LOGI(tag, "-> Wifi disconnected");
      }
  }

  void on_mqtt_connected(bool connected)
  {
    if (connected)
      {
        ESP_LOGI(tag, "-> MQTT connected");
#ifdef  CONFIG_BT_ENABLED
        beacon_scanner.start();
#endif
      }
    else
      {
        ESP_LOGI(tag, "-> MQTT disconnected");
#ifdef  CONFIG_BT_ENABLED
        beacon_scanner.stop();
#endif
      }
  }

#ifdef  CONFIG_BT_ENABLED
  void on_beacon_scanner_scan_result(os::BeaconScanner::ScanResult result)
  {
    static int led_state = 0;

    ESP_LOGI(tag, "-> BT result %s %d", result.mac.c_str(), result.rssi);
    led_state ^= 1;
    gpio_set_level(LED_GPIO, led_state);

#ifdef CONFIG_AWS_IOT_SDK
    mqtt.publish("beacon", result.mac);
#endif
  }
#endif

  void main_task()
  {
    wifi.set_ssid(WIFI_SSID);
    wifi.set_passphase(WIFI_PASS);
    wifi.set_host_name("scan");
    wifi.set_auto_connect(true);
    wifi.system_event_signal().connect(os::Slot<void(system_event_t)>(loop, std::bind(&Main::on_wifi_system_event, this, std::placeholders::_1)));
    wifi.connected().connect(os::Slot<void(bool)>(loop, std::bind(&Main::on_wifi_connected, this, std::placeholders::_1)));

#ifdef  CONFIG_BT_ENABLED
    beacon_scanner.scan_result_signal().connect(os::Slot<void(os::BeaconScanner::ScanResult)>(loop, std::bind(&Main::on_beacon_scanner_scan_result, this, std::placeholders::_1)));
#endif

    wifi.connect();

#ifdef CONFIG_AWS_IOT_SDK
    mqtt.connected().connect(os::Slot<void(bool)>(loop, std::bind(&Main::on_mqtt_connected, this, std::placeholders::_1)));
    mqtt.init(MQTT_HOST, reinterpret_cast<const char *>(ca_start), reinterpret_cast<const char *>(certificate_start), reinterpret_cast<const char *>(private_key_start));
#endif

    try
      {
        sock.set_client_certificate(reinterpret_cast<const char *>(certificate_start), reinterpret_cast<const char *>(private_key_start));
        sock.set_ca_certificate(reinterpret_cast<const char *>(ca_start));
      }
    catch(std::exception &e)
      {
        ESP_LOGE(tag, "Socket exception: %s", e.what());
      }
    heap_caps_print_heap_info(0);
    loop->run();
  }

#ifdef  CONFIG_BT_ENABLED
  os::BeaconScanner &beacon_scanner;
#endif
  os::Wifi &wifi;
  std::shared_ptr<os::MainLoop> loop;
  os::Task task;
#ifdef CONFIG_AWS_IOT_SDK
  os::Mqtt mqtt;
#endif
  os::TLSStream sock;

  const static gpio_num_t LED_GPIO = GPIO_NUM_5;
};


extern "C" void
app_main()
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
  ESP_ERROR_CHECK (ret);

  heap_caps_print_heap_info(0);
  new Main();

  while(1)
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
