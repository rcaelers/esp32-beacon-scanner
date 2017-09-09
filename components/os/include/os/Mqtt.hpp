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

#ifndef OS_MQTT_HPP
#define OS_MQTT_HPP

#include <chrono>
#include <string>
#include <queue>

#include "os/Task.hpp"
#include "os/Wifi.hpp"
#include "os/EventGroup.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"

namespace os
{
  class Mqtt
  {
  public:
    Mqtt();
    ~Mqtt();

    void init(const char *host, const char *ca, const char *cert, const char *key);

    void publish(std::string topic, std::string data);

    os::Property<bool> &connected();

  private:
    static void subscribe_callback_static(AWS_IoT_Client *client, char *topic, uint16_t topic_len, IoT_Publish_Message_Params *params, void *user_data);
    static void disconnect_callback_static(AWS_IoT_Client *client, void *user_data);

    void yield_task();
    void loop_task();

    void on_wifi_connected(bool connected);
    void subscribe_callback(AWS_IoT_Client *client, char *topic, uint16_t topic_len, IoT_Publish_Message_Params *params);
    void disconnect_callback(AWS_IoT_Client *client);

  private:
    os::Wifi &wifi;
    os::MainLoop loop;
    os::EventGroup event_group;

    AWS_IoT_Client client;

    const static int NETWORK_READY_BIT = BIT0;

    os::Task mqtt_loop_task;
    os::Task mqtt_yield_task;
    os::Property<bool> connected_property { false };


    mutable os::Mutex mutex;

    using msg_type = std::pair<std::string, std::string>;
    std::queue<msg_type> queue;
  };
}

#endif // OS_MQTT_HPP
