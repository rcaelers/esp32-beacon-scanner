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

#include <string.h>

#include "os/Mqtt.hpp"

using namespace os;

static const char tag[] = "MQTT";

Mqtt::Mqtt()
  : wifi(os::Wifi::instance()),
    mqtt_loop_task("mqtt_loop_task", std::bind(&Mqtt::loop_task, this)),
    mqtt_yield_task("mqtt_yield_task", std::bind(&Mqtt::yield_task, this))
{
  ESP_LOGI(tag, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
}

Mqtt::~Mqtt()
{
}

void
Mqtt::on_wifi_connected(bool connected)
{
  if (connected)
    {
      ESP_LOGI(tag, "-> Wifi connected");
      event_group.set(NETWORK_READY_BIT);
    }
  else
    {
      ESP_LOGI(tag, "-> Wifi disconnected");
      event_group.clear(NETWORK_READY_BIT);
    }
}

void
Mqtt::disconnect_callback_static(AWS_IoT_Client *client, void *user_data)
{
  static_cast<Mqtt *>(user_data)->disconnect_callback(client);
}

void
Mqtt::disconnect_callback(AWS_IoT_Client *client)
{
  ESP_LOGW(tag, "MQTT Disconnected");
  connected_property.set(false);

  if (client == nullptr)
    {
      return;
    }

  if (!aws_iot_is_autoreconnect_enabled(client))
    {
      IoT_Error_t rc = aws_iot_mqtt_attempt_reconnect(client);
      if (rc != NETWORK_RECONNECTED)
        {
          ESP_LOGW(tag, "MQTT reconnect Failed (%d)", rc);
        }
    }
}

void
Mqtt::subscribe_callback(AWS_IoT_Client *client, char *topic, uint16_t topic_len, IoT_Publish_Message_Params *params)
{
  ESP_LOGI(tag, "Subscribe callback");
  ESP_LOGI(tag, "%.*s\t%.*s", topic_len, topic, (int) params->payloadLen, (char *)params->payload);
}

void
Mqtt::subscribe_callback_static(AWS_IoT_Client *client, char *topic, uint16_t topic_len, IoT_Publish_Message_Params *params, void *user_data)
{
  static_cast<Mqtt *>(user_data)->subscribe_callback(client, topic, topic_len, params);
}

void
Mqtt::init(const char *host, const char *ca, const char *cert, const char *key)
{
  IoT_Client_Init_Params init_params = iotClientInitParamsDefault;
  init_params.enableAutoReconnect = false;
  init_params.pHostURL = const_cast<char *>(host);
  init_params.port = 8883;
  init_params.mqttCommandTimeout_ms = 20000;
  init_params.tlsHandshakeTimeout_ms = 5000;
  init_params.isSSLHostnameVerify = true;
  init_params.disconnectHandler = disconnect_callback_static;
  init_params.disconnectHandlerData = this;

  init_params.pRootCALocation = ca;
  init_params.pDeviceCertLocation = cert;
  init_params.pDevicePrivateKeyLocation = key;

  IoT_Error_t rc = aws_iot_mqtt_init(&client, &init_params);
  if (rc != SUCCESS)
    {
      ESP_LOGE(tag, "MQTT failed to initialize (%d) ", rc);
      // TODO: throw exception
    }
}

void
Mqtt::publish(std::string topic, std::string data)
{
  ScopedLock l(mutex);
  queue.push(std::make_pair(topic, data));
}

void
Mqtt::loop_task()
{
  wifi.connected().connect(os::Slot<void(bool)>(loop, std::bind(&Mqtt::on_wifi_connected, this, std::placeholders::_1)));
  loop.run();
}

void
Mqtt::yield_task()
{
  event_group.wait(NETWORK_READY_BIT);
  IoT_Error_t rc = FAILURE;

  IoT_Client_Connect_Params connect_params = iotClientConnectParamsDefault;
  connect_params.keepAliveIntervalInSec = 10;
  connect_params.isCleanSession = true;
  connect_params.MQTTVersion = MQTT_3_1_1;
  connect_params.pClientID = "ble-scan";
  connect_params.clientIDLen = (uint16_t) strlen(connect_params.pClientID);
  connect_params.isWillMsgPresent = false;

  ESP_LOGI(tag, "Connecting to MQTT server.");
  do
    {
      rc = aws_iot_mqtt_connect(&client, &connect_params);
      if (rc != SUCCESS)
        {
          ESP_LOGE(tag, "Failed to connect to MQTT server (%d)", rc);
          vTaskDelay(500 / portTICK_RATE_MS);
        }
    } while (rc != SUCCESS);

  connected_property.set(true);

  rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
  if (rc != SUCCESS)
    {
      ESP_LOGE(tag, "Failed to set MQTT auto reconnect (%d)", rc);
    }

  while ((rc == NETWORK_ATTEMPTING_RECONNECT || rc == NETWORK_RECONNECTED || rc == SUCCESS))
    {
      // TODO: replace AWS IoT MQTT to get rid of this polling loop....
      rc = aws_iot_mqtt_yield(&client, 100);
      if (rc == NETWORK_ATTEMPTING_RECONNECT)
        {
          continue;
        }
      connected_property.set(true);

      // TODO: queue may be locked too long
      mutex.lock();
      while (!queue.empty())
        {
          auto msg = queue.front();
          IoT_Publish_Message_Params msg_param;
          msg_param.qos = QOS0;
          msg_param.payload = static_cast<void *>(const_cast<char *>(msg.second.c_str()));
          msg_param.payloadLen = msg.second.length();
          msg_param.isRetained = 0;

          IoT_Error_t rc = aws_iot_mqtt_publish(&client, msg.first.c_str(), msg.first.length(), &msg_param);
          if (rc != SUCCESS)
            {
              ESP_LOGE(tag, "MQTT failed to publish (%d) %d", rc, aws_iot_mqtt_get_client_state(&client));
            }
          queue.pop();
        }

      mutex.unlock();
    }
}


os::Property<bool> &
Mqtt::connected()
{
  return connected_property;
}
