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

#ifndef LOOPP_WIFI_HPP
#define LOOPP_WIFI_HPP

#include <string>

#include "esp_system.h"
#include "esp_wifi.h"

#include "loopp/core/Signal.hpp"
#include "loopp/core/Property.hpp"

namespace loopp
{
  namespace net
  {
    class Wifi
    {
    public:
      static Wifi &instance();

      Wifi(const Wifi &) = delete;
      Wifi &operator=(const Wifi &) = delete;

      void set_host_name(const std::string &host_name);
      void set_ssid(const std::string &ssid);
      void set_passphase(const std::string &passphrase);
      void set_auto_connect(bool auto_connect);

      void connect();
      void reconnect();

      loopp::core::Signal<void(system_event_t)> &system_event_signal();
      loopp::core::Property<bool> &connected();

      std::string get_mac() const;

    private:
      Wifi();
      ~Wifi();

      static esp_err_t on_sysem_event(void *ctx, system_event_t *event);
      esp_err_t on_sysem_event(system_event_t *event);

    private:
      bool auto_connect = false;
      std::string host_name;
      std::string ssid;
      std::string passphrase;

      loopp::core::Signal<void(system_event_t)> signal_system_event;
      loopp::core::Property<bool> connected_property{ false };
    };
  } // namespace net
} // namespace loopp
#endif // LOOPP_WIFI_HPP
