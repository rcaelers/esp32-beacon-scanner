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

#include "loopp/mqtt/MqttClient.hpp"

#include <algorithm>
#include <cstring>

#include "esp_log.h"
#include "esp_heap_caps.h"

#include "loopp/mqtt/MqttPacket.hpp"
#include "loopp/mqtt/MqttErrors.hpp"
#include "loopp/net/TCPStream.hpp"
#include "loopp/net/TLSStream.hpp"
#include "loopp/net/NetworkErrors.hpp"

static const char tag[] = "MQTT";

using namespace loopp;
using namespace loopp::mqtt;

MqttClient::MqttClient(std::shared_ptr<loopp::core::MainLoop> loop, std::string client_id, std::string host, int port) :
  loop(loop),
  client_id(std::move(client_id)),
  host(std::move(host)),
  port(port)
{
}

MqttClient::~MqttClient()
{
  ESP_LOGI(tag, "Mqtt closing");
}

void
MqttClient::set_client_certificate(const char *cert, const char *key)
{
  client_cert = cert;
  client_key = key;
}

void
MqttClient::set_ca_certificate(const char *cert)
{
  ca_cert = cert;
}

void
MqttClient::set_client_id(std::string client_id)
{
  this->client_id = std::move(client_id);
}

void
MqttClient::set_username(std::string username)
{
  this->username = std::move(username);
}

void
MqttClient::set_password(std::string password)
{
  this->password = std::move(password);
}

void
MqttClient::set_will(std::string topic, std::string data)
{
  will_topic = std::move(topic);
  will_data = std::move(data);
}

void
MqttClient::set_will_retain(bool retain)
{
  will_retain = retain;
}

void
MqttClient::connect()
{
  if (connected_property.get())
    {
      throw std::system_error(MqttErrc::ProtocolError, "already connected to MQTT server");
    }

  try
    {
      if (ca_cert != nullptr)
        {
          std::shared_ptr<loopp::net::TLSStream> tls_sock = std::make_shared<loopp::net::TLSStream>(loop);

          if (client_cert != nullptr && client_key != nullptr)
            {
              tls_sock->set_client_certificate(client_cert, client_key);
            }
          tls_sock->set_ca_certificate(ca_cert);

          sock = tls_sock;
        }
      else
        {
          sock = std::make_shared<loopp::net::TCPStream>(loop);
        }

      auto self = shared_from_this();
      sock->connect(host, port, [this, self] (std::error_code ec) {
          if (!ec)
            {
              ESP_LOGI(tag, "Mqtt connected");
              heap_caps_print_heap_info(0);
              send_connect();
            }
          else
            {
              handle_error("connect", ec);
            }
        });
    }
  catch (std::system_error &e)
    {
      handle_error(std::string("connect: ") + e.what(), e.code());
    }
}

void
MqttClient::disconnect()
{
  ESP_LOGI(tag, "Closing socket");
  if (!connected_property.get())
    {
      throw std::system_error(MqttErrc::NotConnected, "not connected to MQTT server");
    }

  if (ping_timer)
    {
      loop->cancel_timer(ping_timer);
      ping_timer = 0;
    }

  if (sock)
    {
      sock->close();
      sock.reset();
    }
}

void
MqttClient::publish(std::string topic, std::string payload, PublishOptions options)
{
  if (!connected_property.get())
    {
      throw std::system_error(MqttErrc::NotConnected, "not connected to MQTT server");
    }

  auto self = shared_from_this();
  loop->invoke([this, self, topic, payload, options] () {
      send_publish(topic, payload, options);
    });
}

void
MqttClient::subscribe(std::string topic)
{
  subscriptions.push_back(topic);

  if (connected_property.get())
    {
      auto self = shared_from_this();
      loop->invoke([this, self, topic] () {
          std::list<std::string> topics;
          topics.push_back(topic);
          send_subscribe(topics);
        });
    }
}

void
MqttClient::unsubscribe(std::string topic)
{
  subscriptions.remove(topic);

  if (connected_property.get())
    {
      auto self = shared_from_this();
      loop->invoke([this, self, topic] () {
          std::list<std::string> topics;
          topics.push_back(topic);
          send_unsubscribe(topics);
        });
    }
}

void
MqttClient::set_callback(subscribe_callback_t callback)
{
  subscribe_callback = std::move(callback);
}

void
MqttClient::send_connect()
{
  try
    {
      std::shared_ptr<MqttPacket> pkt = std::make_shared<MqttPacket>();
      BitMask<ConnectFlags> flags(ConnectFlags::None);
      std::size_t len = 10;

      len += client_id.size() + 2;

      if (username != "")
        {
          flags |= ConnectFlags::UserName;
          len += username.size() + 2;
        }

      if (password != "")
        {
          flags |= ConnectFlags::Password;
          len += password.size() + 2;
        }

      if (will_topic != "" && will_data != "")
        {
          flags |= ConnectFlags::Will;
          if (will_retain)
            {
              flags |= ConnectFlags::WillRetain;
            }
          len += will_topic.size() + 2;
          len += will_data.size() + 2;
        }

      flags |= ConnectFlags::CleanSession;

      pkt->add_fixed_header(loopp::mqtt::PacketType::Connect, 0);
      pkt->add_length(len);
      pkt->add("MQTT");
      pkt->add(0x04);
      pkt->add(static_cast<uint8_t>(flags.value()));
      pkt->add(static_cast<std::uint8_t>(keep_alive_sec >> 8));
      pkt->add(static_cast<std::uint8_t>(keep_alive_sec & 0xff));
      pkt->add(client_id);

      if (flags & loopp::mqtt::ConnectFlags::Will)
        {
          pkt->add(will_topic);
          pkt->add(will_data);
        }

      if (flags & loopp::mqtt::ConnectFlags::UserName)
        {
          pkt->add(username);
        }

      if (flags & loopp::mqtt::ConnectFlags::Password)
        {
          pkt->add(password);
        }

      auto self = shared_from_this();
      sock->write_async(pkt->get_buffer(), [this, self, pkt] (std::error_code ec, std::size_t bytes_transferred) {
          ec = verify("send connect", bytes_transferred, pkt->size(), ec);
          if (!ec)
            {
              async_read_control_packet();
            }
        });
    }
  catch (std::system_error &e)
    {
      handle_error(std::string("send connect: ") + e.what(), e.code());
    }
}

void
MqttClient::send_ping()
{
  try
    {
      std::shared_ptr<MqttPacket> pkt = std::make_shared<MqttPacket>();

      pkt->add_fixed_header(loopp::mqtt::PacketType::PingReq, 0);
      pkt->add(0);
      auto self = shared_from_this();
      sock->write_async(pkt->get_buffer(), [this, self, pkt] (std::error_code ec, std::size_t bytes_transferred) {
          verify("send ping", bytes_transferred, pkt->size(), ec);
          pending_ping_count++;

          if (pending_ping_count > pending_ping_count_limit)
            {
              handle_error("ping response timeout ", MqttErrc::Timeout);
            }
        });
    }
  catch (std::system_error &e)
    {
      handle_error(std::string("send ping: ") + e.what(), e.code());
    }
}

void
MqttClient::send_publish(std::string topic, std::string payload, PublishOptions options)
{
  try
    {
      std::shared_ptr<MqttPacket> pkt = std::make_shared<MqttPacket>();

      std::size_t len = 0;
      BitMask<PublishFlags> flags = PublishFlags::None;

      if (options & PublishOptions::Retain)
        {
          flags |= PublishFlags::Retain;
        }

      len += topic.size() + 2;
      len += payload.size();

      pkt->add_fixed_header(loopp::mqtt::PacketType::Publish, static_cast<uint8_t>(flags.value()));
      pkt->add_length(len);
      pkt->add(topic);
      pkt->append(payload);

      auto self = shared_from_this();
      sock->write_async(pkt->get_buffer(), [this, self, pkt] (std::error_code ec, std::size_t bytes_transferred) {
          verify("send publish", bytes_transferred, pkt->size(), ec);
        });
    }
  catch (std::system_error &e)
    {
      handle_error(std::string("send publish: ") + e.what(), e.code());
    }
}

void
MqttClient::send_subscribe(std::list<std::string> topics)
{
  try
    {
      std::shared_ptr<MqttPacket> pkt = std::make_shared<MqttPacket>();
      packet_id++;

      std::size_t len = 2 + std::accumulate(topics.begin(), topics.end(), 0, [](int sum, const std::string &s) {
          return sum + s.size() + 2 + 1;
        });

      pkt->add_fixed_header(loopp::mqtt::PacketType::Subscribe, 0b0010u);
      pkt->add_length(len);
      pkt->add(static_cast<std::uint8_t>(packet_id >> 8));
      pkt->add(static_cast<std::uint8_t>(packet_id & 0xff));
      for (auto topic : topics)
        {
          pkt->add(topic);
          pkt->add(0);
        }

      auto self = shared_from_this();
      sock->write_async(pkt->get_buffer(), [this, self, pkt] (std::error_code ec, std::size_t bytes_transferred) {
          verify("send subscribe", bytes_transferred, pkt->size(), ec);
        });
    }
  catch (std::system_error &e)
    {
      handle_error(std::string("send subscribe: ") + e.what(), e.code());
    }
}

void
MqttClient::send_unsubscribe(std::list<std::string> topics)
{
  try
    {
      std::shared_ptr<MqttPacket> pkt = std::make_shared<MqttPacket>();
      packet_id++;

      std::size_t len = 2 + std::accumulate(topics.begin(), topics.end(), 0, [](int sum, const std::string &s) {
          return sum + s.size() + 2;
        });

      pkt->add_fixed_header(loopp::mqtt::PacketType::Unsubscribe, 0b0010u);
      pkt->add_length(len);
      pkt->add(static_cast<std::uint8_t>(packet_id >> 8));
      pkt->add(static_cast<std::uint8_t>(packet_id & 0xff));
      for (auto topic : topics)
        {
          pkt->add(topic);
        }

      auto self = shared_from_this();
      sock->write_async(pkt->get_buffer(), [this, self, pkt] (std::error_code ec, std::size_t bytes_transferred) {
          verify("send unsubscribe", bytes_transferred, pkt->size(), ec);
        });
    }
  catch (std::system_error &e)
    {
      handle_error(std::string("send unsubscribe: ") + e.what(), e.code());
    }
}

void
MqttClient::async_read_control_packet()
{
  auto self = shared_from_this();
  sock->read_async(buffer, 1, [this, self] (std::error_code ec, std::size_t bytes_transferred) {
      ec = verify("fixed header", bytes_transferred, 1, ec);
      if (!ec)
        {
          handle_control_packet();
        }
    });
}

void
MqttClient::handle_control_packet()
{
  remaining_length = 0;
  remaining_length_multiplier = 1;
  fixed_header = *buffer.consume_data();
  buffer.consume_commit(1);

  async_read_remaining_length();
}

void
MqttClient::async_read_remaining_length()
{
  auto self = shared_from_this();
  sock->read_async(buffer, 1, [this, self] (std::error_code ec, std::size_t bytes_transferred) {
      ec = verify("remaining length", bytes_transferred, 1, ec);
      if (!ec)
        {
          handle_remaining_length();
        }
    });
}

void
MqttClient::handle_remaining_length()
{
  uint8_t header = *reinterpret_cast<uint8_t *>(buffer.consume_data());
  buffer.consume_commit(1);

  remaining_length += (header & 0b01111111) * remaining_length_multiplier;
  remaining_length_multiplier *= 128;

  if (header & 0b10000000u)
    {
      async_read_remaining_length();
    }
  else
    {
      if (remaining_length == 0)
        {
          handle_payload();
        }
      else
        {
          async_read_payload();
        }
    }
}

void
MqttClient::async_read_payload()
{
  auto self = shared_from_this();
  sock->read_async(buffer, remaining_length, [this, self] (std::error_code ec, std::size_t bytes_transferred) {
      ec = verify("payload", bytes_transferred, remaining_length, ec);
      if (!ec)
        {
          handle_payload();
          buffer.consume_commit(remaining_length);
        }
    });
}

std::error_code
MqttClient::handle_payload()
{
  PacketType packet_type = static_cast<PacketType>(fixed_header >> 4);
  std::error_code ec;

  switch (packet_type)
    {
    case PacketType::ConnAck:
      ec = handle_connect_ack();
      break;

    case PacketType::Publish:
      ec = handle_publish();
      break;

    case PacketType::PubAck:
      ec = handle_publish_ack();
      break;

    case PacketType::PubRec:
      ec = handle_publish_received();
      break;

    case PacketType::PubRel:
      ec = handle_publish_release();
      break;

    case PacketType::PubComp:
      ec = handle_publish_complete();
      break;

    case PacketType::SubAck:
      ec = handle_subscribe_ack();
      break;

    case PacketType::UnsubAck:
      ec = handle_unsubscribe_ack();
      break;

    case PacketType::PingResp:
      ec = handle_ping_response();
      break;

    default:
      verify("invalid payload type", 0, 0, MqttErrc::ProtocolError);
    }

  if (!ec)
    {
      async_read_control_packet();
    }

  return ec;
}

std::error_code
MqttClient::handle_connect_ack()
{
  std::error_code ec = verify("handle ConnAck", remaining_length, 2, std::error_code());
  if (!ec)
    {
      uint8_t *payload_buffer = reinterpret_cast<uint8_t*>(buffer.consume_data());
      uint8_t connect_return_code = static_cast<std::uint8_t>(payload_buffer[1]);
      if (connect_return_code != 0)
        {
          ESP_LOGE(tag, "Error: Connect return code = %d", connect_return_code);
          handle_error("ConnAck: failed to connect", MqttErrc::ProtocolError);
        }
      else
        {
          ESP_LOGI(tag, "Info: Connect OK");
          ping_timer = loop->add_periodic_timer(std::chrono::milliseconds(ping_interval_sec * 1000), std::bind(&MqttClient::send_ping, this));

          if (subscriptions.size() > 0)
            {
              ESP_LOGI(tag, "Info: Connect OK - Sending subscriptions");
              send_subscribe(subscriptions);
            }
          else
            {
              connected_property.set(true);
            }
        }
    }

  return ec;
}

std::error_code
MqttClient::handle_publish()
{
  std::error_code ec;

  try
    {
      uint8_t *payload_buffer = reinterpret_cast<uint8_t *>(buffer.consume_data());
      BitMask<PublishFlags> flags;

      flags.set(fixed_header & 0x0f);

      if ((flags & PublishFlags::QosMask) != PublishFlags::Qos0)
        {
          throw std::system_error(MqttErrc::ProtocolError, "QoS1/2 not supported");
        }

      std::size_t index = 0;
      std::uint16_t topic_len = (payload_buffer[index] << 8) + payload_buffer[index + 1];
      index += 2;

      if (remaining_length - index < topic_len)
        {
          throw std::system_error(MqttErrc::ProtocolError, "short packet");
        }

      std::string topic(reinterpret_cast<char *>(payload_buffer + index), topic_len);
      index += topic_len;

      std::string payload(reinterpret_cast<char *>(payload_buffer + index), remaining_length - index);

      ESP_LOGI(tag, "Info: Received %s -> %s", topic.c_str(), payload.c_str());
      bool matched = false;
      for (auto kv : filters)
        {
          if (match_topic(topic, kv.first))
            {
              kv.second(topic, payload);
              matched = true;
            }
        }
      if (!matched)
        {
          subscribe_callback(topic, payload);
        }
    }
  catch (std::system_error &e)
    {
      handle_error(std::string("handle publish: ") + e.what(), e.code());
      ec = e.code();
    }
  return ec;
}

std::error_code
MqttClient::handle_publish_ack()
{
  std::error_code ec;
  return ec;
}

std::error_code
MqttClient::handle_publish_received()
{
  std::error_code ec;
  return ec;
}

std::error_code
MqttClient::handle_publish_release()
{
  std::error_code ec;
  return ec;
}

std::error_code
MqttClient::handle_publish_complete()
{
  std::error_code ec;
  return ec;
}

std::error_code
MqttClient::handle_subscribe_ack()
{
  std::error_code ec;

  try
    {
      if (remaining_length < 2)
        {
          throw std::system_error(MqttErrc::ProtocolError, "no packet id in suback");
        }

#ifdef NOT_YEY_USED
      // TODO: return response to client
      uint8_t *payload_buffer = reinterpret_cast<uint8_t*>(buffer.consume_data());
      int id = (payload_buffer[0] << 8) + payload_buffer[1] ;

      for (int i = 2; i < remaining_length; i++)
        {
          uint8_t status = payload_buffer[i];
        }
#endif

      connected_property.set(true);
    }
  catch (std::system_error &e)
    {
      handle_error(std::string("handle subscribe ack: ") + e.what(), e.code());
      ec = e.code();
    }
  return ec;
}

std::error_code
MqttClient::handle_unsubscribe_ack()
{
  std::error_code ec;

  try
    {
      if (remaining_length < 2)
        {
          throw std::system_error(MqttErrc::ProtocolError, "no packet id in suback");
        }

#ifdef NOT_USED
      uint8_t *payload_buffer = reinterpret_cast<uint8_t*>(buffer.data());
      int id = (payload_buffer[0] << 8) + payload_buffer[1] ;
#endif
    }
  catch (std::system_error &e)
    {
      handle_error(std::string("handle unsubscribe ack: ") + e.what(), e.code());
      ec = e.code();
    }
  return ec;
}

std::error_code
MqttClient::handle_ping_response()
{
  std::error_code ec;
  pending_ping_count--;
  return ec;
}

void
MqttClient::handle_error(std::string what, std::error_code ec)
{
  if (ec)
    {
      ESP_LOGE(tag, "Error: %s %s", what.c_str(), ec.message().c_str());
      connected_property.set(false);

      if (ping_timer)
        {
          loop->cancel_timer(ping_timer);
          ping_timer = 0;
        }

      heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
      if (sock)
        {
          sock->close();
          sock.reset();
          heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
        }

      if (ec != loopp::net::NetworkErrc::Cancelled)
        {
          connect();
        }
    }
}

std::error_code
MqttClient::verify(std::string what, std::size_t actual_size, std::size_t expect_size, std::error_code ec)
{
  if (!ec && actual_size != expect_size)
    {
      ESP_LOGE(tag, "Error: %s short packet, actual %d expected %d", what.c_str(), actual_size, expect_size);
      ec = MqttErrc::ProtocolError;
    }

  handle_error(what, ec);

  return ec;
}

loopp::core::Property<bool> &
MqttClient::connected()
{
  return connected_property;
}

bool
MqttClient::match_topic(std::string topic, std::string filter)
{
  auto topic_it = topic.begin();
  auto filter_it = filter.begin();

  while (true)
    {
      if (filter_it == filter.end())
        {
          return topic_it == topic.end();
        }
      else if (topic_it == topic.end()) // && filter_it != filter.end()
        {
          if (*filter_it == '/')
            {
              filter_it++;
              if ((filter_it != filter.end()) && (*filter_it == '#'))
                {
                  return true;
                }
            }
          return false;
        }

      if (*filter_it == '+')
        {
          while (*topic_it != '/'  && topic_it < topic.end())
            {
              ++topic_it;
            }
          ++filter_it;
        }

      else if (*filter_it == '#')
        {
          return true;
        }

      else
        {
          if (*topic_it != *filter_it)
            {
              return false;
            }
          ++topic_it;
          ++filter_it;
        }
    }
}

void
MqttClient::add_filter(std::string filter, subscribe_callback_t callback)
{
  filters[filter] = std::move(callback);
}

void
MqttClient::remove_filter(std::string filter)
{
  filters.erase(filter);
}
