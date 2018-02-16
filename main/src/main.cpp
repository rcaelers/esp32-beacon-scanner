// Copyright (C) 2017, 2018 Rob Caelers <rob.caelers@gmail.com>
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

#include "loopp/ble/BLEScanner.hpp"
#include "loopp/core/MainLoop.hpp"
#include "loopp/core/Slot.hpp"
#include "loopp/core/Task.hpp"
#include "loopp/mqtt/MqttClient.hpp"
#include "loopp/net/Wifi.hpp"
#include "loopp/ota/OTA.hpp"
#include "loopp/utils/hexdump.hpp"
#include "loopp/utils/json.hpp"

#include "string.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "user_config.h"

#include "DriverFactory.hpp"

static const char current_version[] = VERSION;

using json = nlohmann::json;

static const char tag[] = "BEACON-SCANNER";

extern const uint8_t ca_start[] asm("_binary_CA_crt_start");
extern const uint8_t ca_end[] asm("_binary_CA_crt_end");
extern const uint8_t certificate_start[] asm("_binary_esp32_crt_start");
extern const uint8_t certificate_end[] asm("_binary_esp32_crt_end");
extern const uint8_t private_key_start[] asm("_binary_esp32_key_start");
extern const uint8_t private_key_end[] asm("_binary_esp32_key_end");

void
memlog(const char *msg)
{
  static int last = -1;

  int heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);

  if (last != -1)
    {
      ESP_LOGI(tag, "%s: %d (delta %d)", msg, heap, last - heap);
    }
  else
    {
      ESP_LOGI(tag, "%s: %d", msg, heap);
    }
  last = heap;
}

class Main
{
public:
  Main()
    : ble_scanner(loopp::ble::BLEScanner::instance())
    , wifi(loopp::net::Wifi::instance())
    , loop(std::make_shared<loopp::core::MainLoop>())
    , mqtt(std::make_shared<loopp::mqtt::MqttClient>(loop, "BLEScanner", MQTT_HOST, MQTT_PORT))
    , driver_factory(std::make_shared<DriverFactory>(loop, mqtt))
    , task(std::make_shared<loopp::core::Task>("main_task", std::bind(&Main::main_task, this)))
  {

    std::string mac = wifi.get_mac();
    topic_root = std::string(MQTT_TOPIC_PREFIX) + "/" + mac + "/";
    topic_command = topic_root + "command";
    topic_configuration = topic_root + "configuration";
  }

private:
  void
  on_wifi_system_event(system_event_t event)
  {
    ESP_LOGI(tag, "-> System event %d", event.event_id);
  }

  void
  on_wifi_timeout()
  {
    ESP_LOGI(tag, "-> Wifi timer");
    wifi_timeout_timer = 0;
    if (!wifi.connected().get())
      {
        ESP_LOGI(tag, "-> Wifi failed to connect in time. Reset");
        wifi.reconnect();
        wifi_timeout_timer = loop->add_timer(std::chrono::milliseconds(5000), std::bind(&Main::on_wifi_timeout, this));
      }
  }

  void
  on_wifi_connected(bool connected)
  {
    if (connected)
      {
        ESP_LOGI(tag, "-> Wifi connected");
        loop->cancel_timer(wifi_timeout_timer);
#ifdef MQTT_USER
        mqtt->set_username(MQTT_USER);
#endif
#ifdef MQTT_PASS
        mqtt->set_password(MQTT_PASS);
#endif
#ifdef MQTT_USE_TLS
        mqtt->set_client_certificate(reinterpret_cast<const char *>(certificate_start), reinterpret_cast<const char *>(private_key_start));
        mqtt->set_ca_certificate(reinterpret_cast<const char *>(ca_start));
#endif
        mqtt->set_callback(loopp::core::make_slot(loop, [this](std::string topic, std::string payload) { on_mqtt_data(topic, payload); }));
        mqtt->connected().connect(loop, std::bind(&Main::on_mqtt_connected, this, std::placeholders::_1));
        mqtt->connect();
      }
    else
      {
        ESP_LOGI(tag, "-> Wifi disconnected");
      }
  }

  void
  on_mqtt_connected(bool connected)
  {
    if (connected)
      {
        ESP_LOGI(tag, "-> MQTT connected");
        ESP_LOGI(tag, "-> Subscribing to configuration at %s", topic_configuration.c_str());
        mqtt->subscribe(topic_configuration);
        mqtt->add_filter(topic_configuration,
                         loopp::core::make_slot(loop, [this](std::string topic, std::string payload) { on_provisioning(payload); }));
        ESP_LOGI(tag, "-> Subscribing to remote commands at %s", topic_command.c_str());
        mqtt->subscribe(topic_command);
        mqtt->add_filter(topic_command,
                         loopp::core::make_slot(loop, [this](std::string topic, std::string payload) { on_remote_command(payload); }));

#ifdef ENABLE_DEFAULT_BLE_SCANNER
        std::string name = "ble-scanner";
        std::shared_ptr<IDriver> driver = driver_factory->create(name, topic_root);
        if (driver)
          {
            drivers[name] = driver;
            driver->start();
          }
#endif
      }
    else
      {
        ESP_LOGI(tag, "-> MQTT disconnected");
      }
  }

  void
  on_mqtt_data(std::string topic, std::string payload)
  {
    ESP_LOGI(tag, "-> MQTT %s -> %s (free %d)", topic.c_str(), payload.c_str(), heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
  }

  void
  on_firmware_provisioning(json top)
  {
    try
      {
        auto firmware = top.at("firmware");
        auto version = firmware.at("version").get<std::string>();
        auto url = firmware.at("url").get<std::string>();

        int timeout = 0;

        auto it = top.find("timeout");
        if (it != top.end())
          {
            timeout = *it;
          }

        ESP_LOGI(tag, "-> Version: %s (current %s)", version.c_str(), current_version);
        ESP_LOGI(tag, "-> URI    : %s", url.c_str());

        if (version != std::string(current_version))
          {
            firmware_update(url, timeout);
          }
      }
    catch (json::out_of_range &e)
      {
        ESP_LOGI(tag, "-> Invalid firmware specification: %s", e.what());
      }
  }

  void
  on_provisioning(std::string payload)
  {
    ESP_LOGI(tag, "-> MQTT provisioning: %s", payload.c_str());

    // TODO: Allow re-configuration.

    auto top = json::parse(payload);

    ESP_LOGI(tag, "-> Name: %s", top["name"].get<std::string>().c_str());

    auto it = top.find("firmware");
    if (it != top.end())
      {
        on_firmware_provisioning(top);
      }

    it = top.find("devices");
    if (it != top.end())
      {
        for (auto driver : drivers)
          {
            ESP_LOGI(tag, "Stopping : %s", driver.first.c_str());
            driver.second->stop();
          }
        drivers.clear();

        for (auto device : top.at("devices"))
          {
            std::string name = device["name"].get<std::string>();
            ESP_LOGI(tag, "-> Name  : %s", name.c_str());
            ESP_LOGI(tag, "-> Driver: %s", device["driver"].get<std::string>().c_str());

            std::shared_ptr<IDriver> driver = driver_factory->create(device, topic_root);
            if (driver)
              {
                drivers[name] = driver;
                driver->start();
              }
            else
              {
                ESP_LOGE(tag, "No driver for %s", device["driver"].get<std::string>().c_str());
              }
          }
      }
  }

  void
  on_remote_command(std::string payload)
  {
    ESP_LOGI(tag, "-> MQTT remote command: %s", payload.c_str());

    auto top = json::parse(payload);

    try
      {
        auto cmd = top.at("cmd");
        ESP_LOGI(tag, "-> Command: %s", cmd.get<std::string>().c_str());

        if (cmd == "reboot")
          {
            ESP_LOGI(tag, "Restarting");
            esp_restart();
          }
        else if (cmd == "firmware-upgrade")
          {
            auto url = top.at("url").get<std::string>();
            int timeout = 0;

            auto it = top.find("timeout");
            if (it != top.end())
              {
                timeout = *it;
              }

            firmware_update(url, timeout);
          }
      }
    catch (json::out_of_range &e)
      {
        ESP_LOGI(tag, "-> Invalid command: %s", e.what());
      }
  }

  void
  firmware_update(std::string url, int timeout)
  {
    // 520K is insufficient to run two TLS connections, so close MQTT before retrieving new firmware.
    mqtt->disconnect();

    // Memory may be freed asynchronously, so delay firmware update until mainloop had a change to terminate the MQTT connection...
    loop->add_timer(std::chrono::milliseconds(1000), [this, url]() {
      std::shared_ptr<loopp::ota::OTA> ota = std::make_shared<loopp::ota::OTA>(loop);

      ota->set_client_certificate(reinterpret_cast<const char *>(certificate_start), reinterpret_cast<const char *>(private_key_start));
      ota->set_ca_certificate(reinterpret_cast<const char *>(ca_start));

      ota->upgrade_async(url, std::chrono::seconds(60), loopp::core::make_slot(loop, [this, ota](std::error_code ec) {
                           ESP_LOGI(tag, "-> OTA ready");
                           if (!ec)
                             {
                               ESP_LOGI(tag, "-> OTA commit");
                               ota->commit();
                             }
                           else
                             {
                               esp_restart();
                             }
                         }));
    });
  }

  void
  main_task()
  {
    wifi.set_ssid(WIFI_SSID);
    wifi.set_passphase(WIFI_PASS);
    wifi.set_host_name("scan");
    wifi.set_auto_connect(true);
    wifi.system_event_signal().connect(loop, std::bind(&Main::on_wifi_system_event, this, std::placeholders::_1));
    wifi.connected().connect(loop, std::bind(&Main::on_wifi_connected, this, std::placeholders::_1));

    wifi_timeout_timer = loop->add_timer(std::chrono::milliseconds(5000), std::bind(&Main::on_wifi_timeout, this));
    wifi.connect();

    ESP_LOGI(tag, "Main::main_task memory free %d", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);

    loop->run();
  }

  loopp::ble::BLEScanner &ble_scanner;
  loopp::net::Wifi &wifi;
  std::shared_ptr<loopp::core::MainLoop> loop;
  std::shared_ptr<loopp::mqtt::MqttClient> mqtt;
  std::shared_ptr<DriverFactory> driver_factory;
  std::shared_ptr<loopp::core::Task> task;
  std::map<std::string, std::shared_ptr<IDriver>> drivers;
  loopp::core::MainLoop::timer_id wifi_timeout_timer = 0;
  std::string topic_root;
  std::string topic_command;
  std::string topic_configuration;
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
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(tag, "Version: %s", current_version);
  ESP_LOGI(tag, "HEAP: startup");
  heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);

  new Main();

  while (1)
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
