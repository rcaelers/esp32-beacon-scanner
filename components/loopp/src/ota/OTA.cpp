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

#include "loopp/ota/OTA.hpp"

#include <iomanip>
#include <sstream>

#include "boost/format.hpp"

#include "esp_log.h"
#include "esp_heap_caps.h"

#include "loopp/ota/OTAErrors.hpp"

static const char *tag = "OTA";

using namespace loopp;
using namespace loopp::ota;

OTA::OTA(std::shared_ptr<loopp::core::MainLoop> loop)
  : loop(loop)
  , client(std::make_shared<loopp::http::HttpClient>(loop))
{
  check();
}

void
OTA::set_client_certificate(const char *cert, const char *key)
{
  client->set_client_certificate(cert, key);
}

void
OTA::set_ca_certificate(const char *cert)
{
  client->set_ca_certificate(cert);
}

void
OTA::check()
{
  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  if (configured != running)
    {
      ESP_LOGW(tag, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x", configured->address, running->address);
    }
  ESP_LOGI(tag, "Running partition type %d subtype %d (offset 0x%08x)", running->type, running->subtype, running->address);
}

void
OTA::upgrade_async(const std::string &url, std::chrono::seconds timeout_duration, const ota_result_callback_t &callback)
{
  try
    {
      this->callback = callback;

      if (timeout_duration != std::chrono::seconds(0))
        {
          timeout_timer = loop->add_timer(std::chrono::seconds(timeout_duration), []() { esp_restart(); });
        }

      update_partition = esp_ota_get_next_update_partition(nullptr);
      ESP_LOGI(tag,
               "Writing to partition, type %d subtype %d at offset 0x%x",
               update_partition->type,
               update_partition->subtype,
               update_partition->address);

      uint32_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
      if (err != ESP_OK)
        {
          throw std::system_error(OTAErrc::InternalError, (boost::format("Could not start OTA, error %1%") % err).str());
        }

      loopp::http::Request request("GET", url);
      auto self = shared_from_this();
      client->execute(request, std::bind(&OTA::on_http_response, this, std::placeholders::_1, std::placeholders::_2));
    }
  catch (const std::system_error &ex)
    {
      ESP_LOGD(tag, "upgrade_async exception %d %s", ex.code().value(), ex.what());
      callback(ex.code());
    }
}

void
OTA::on_http_response(std::error_code ec, const loopp::http::Response &response)
{
  if (!ec)
    {
      ESP_LOGI(tag, "Status %03d: %s", response.status_code(), response.status_message().c_str());
      if (response.status_code() != 200)
        {
          this->callback(OTAErrc::InternalError);
        }
      else
        {
          ESP_LOGI(tag, "Retrieving firmware");
          retrieve_body();
        }
    }
  else
    {
      ESP_LOGE(tag, "Failed to request firmware");
      this->callback(ec);
    }
}

void
OTA::retrieve_body()
{
  auto self = shared_from_this();
  client->read_body_async(2048, loopp::core::bind_loop(loop, [this, self](std::error_code ec, loopp::net::StreamBuffer *buffer) {
                            try
                              {
                                if (ec)
                                  {
                                    throw std::system_error(ec, "failed to read body");
                                  }

                                if (buffer->consume_size() > 0)
                                  {
                                    esp_err_t err = esp_ota_write(update_handle, buffer->consume_data(), buffer->consume_size());
                                    if (err != ESP_OK)
                                      {
                                        throw std::system_error(OTAErrc::InternalError,
                                                                (boost::format("Could not write OTA, error %1%") % err).str());
                                      }

                                    buffer->consume_commit(buffer->consume_size());

                                    std::size_t total = client->get_body_length();
                                    std::size_t left = client->get_body_length_left();
                                    int done = (100 * (total - left)) / total;
                                    ESP_LOGI(tag, "Progress: %d%%", done);

                                    retrieve_body();
                                  }
                                else
                                  {
                                    ESP_LOGD(tag, "OTA ready");
                                    callback(std::error_code());
                                  }
                              }
                            catch (const std::system_error &ex)
                              {
                                ESP_LOGE(tag, "upgrade_async exception %d %s", ex.code().value(), ex.what());
                                callback(ex.code());
                              }
                          }));
}

void
OTA::commit()
{
  if (timeout_timer != 0)
    {
      loop->cancel_timer(timeout_timer);
    }

  esp_err_t err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK)
    {
      ESP_LOGE(tag, "esp_ota_set_boot_partition failed! err=0x%x", err);
    }
  ESP_LOGI(tag, "Restarting");
  esp_restart();
}
