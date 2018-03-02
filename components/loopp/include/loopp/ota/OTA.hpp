// Copyright (C) 2018 Rob Caelers <rob.caelers@gmail.com>
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

#ifndef LOOPP_OTA_OTA_HPP
#define LOOPP_OTA_OTA_HPP

#include <string>
#include <memory>

#include "esp_ota_ops.h"

#include "loopp/core/MainLoop.hpp"
#include "loopp/http/HttpClient.hpp"

namespace loopp
{
  namespace ota
  {
    class OTA : public std::enable_shared_from_this<OTA>
    {
    public:
      using ota_result_function_t = void(std::error_code);
      using ota_result_callback_t = std::function<ota_result_function_t>;

      OTA(std::shared_ptr<loopp::core::MainLoop> loop);
      ~OTA() = default;
      OTA(const OTA&) = delete;
      OTA& operator=(const OTA&) = delete;

      void set_client_certificate(const char *cert, const char *key);
      void set_ca_certificate(const char *cert);

      void upgrade_async(std::string url, std::chrono::seconds timeout_duration, ota_result_callback_t callback);
      void commit();

    private:
      void check();
      void on_http_response(std::error_code ec, loopp::http::Response response);
      void retrieve_body();

    private:
      std::shared_ptr<loopp::core::MainLoop> loop;
      std::shared_ptr<loopp::http::HttpClient> client;

      ota_result_callback_t callback;

      esp_ota_handle_t update_handle = 0;
      const esp_partition_t *update_partition = NULL;
      loopp::core::MainLoop::timer_id timeout_timer = 0;
    };
  }
}

#endif // LOOPP_OTA_OT_HPP
