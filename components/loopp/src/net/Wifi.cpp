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

#include "loopp/net/Wifi.hpp"

#include <string.h>

#include "esp_log.h"
#include "esp_event_loop.h"

static const char tag[] = "WIFI";

using namespace loopp;
using namespace loopp::net;

Wifi::Wifi()
{
  tcpip_adapter_init();
}

Wifi::~Wifi()
{
  esp_wifi_disconnect();
  esp_wifi_stop();
}

void
Wifi::set_host_name(const std::string &host_name)
{
  this->host_name = host_name;
}

void
Wifi::set_ssid(const std::string &ssid)
{
  this->ssid = ssid;
}

void
Wifi::set_passphase(const std::string &passphrase)
{
  this->passphrase = passphrase;
}

void Wifi::set_auto_connect(bool auto_connect)
{
  this->auto_connect = auto_connect;
}

void
Wifi::connect()
{
  ESP_LOGI(tag, "Connecting");

  esp_event_loop_init(&Wifi::on_sysem_event, this);

  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&init);

  esp_wifi_set_mode(WIFI_MODE_STA);

  wifi_config_t wifi_config;
  memset(&wifi_config, 0, sizeof(wifi_config));
  strlcpy(reinterpret_cast<char *>(wifi_config.sta.ssid), ssid.c_str(), sizeof(wifi_config.sta.ssid));
  strlcpy(reinterpret_cast<char *>(wifi_config.sta.password), passphrase.c_str(), sizeof(wifi_config.sta.password));
  wifi_config.sta.bssid_set = false;

  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

  esp_wifi_start();
}

void
Wifi::reconnect()
{
  esp_wifi_disconnect();
  esp_wifi_stop();
  esp_wifi_start();
}

std::string
Wifi::get_mac() const
{
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  char mac_str[18] = { 0 };
  sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return std::string(mac_str);
}

esp_err_t
Wifi::on_sysem_event(void *ctx, system_event_t *event)
{
  Wifi *wifi = static_cast<Wifi *>(ctx);
  return wifi->on_sysem_event(event);
}

esp_err_t
Wifi::on_sysem_event(system_event_t *event)
{
  ESP_LOGI(tag, "System event: %d", event->event_id);
  signal_system_event(*event);

  switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
      tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, host_name.c_str());
      esp_wifi_connect();
      break;

    case SYSTEM_EVENT_STA_STOP:
      connected_property.set(false);
      break;

    case SYSTEM_EVENT_STA_GOT_IP:
      connected_property.set(true);
      break;

    case SYSTEM_EVENT_STA_CONNECTED:
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      connected_property.set(false);

      if (auto_connect)
        {
          esp_wifi_stop();
          esp_wifi_start();
          esp_wifi_connect();
        }
      break;

    default:
      break;
    }

  return ESP_OK;
}

loopp::core::Signal<void(system_event_t)> &
Wifi::system_event_signal()
{
  return signal_system_event;
}

loopp::core::Property<bool> &
Wifi::connected()
{
  return connected_property;
}
