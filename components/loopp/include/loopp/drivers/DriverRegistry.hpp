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

#ifndef DRIVERREGISTRY_HH
#define DRIVERREGISTRY_HH

#include <string>
#include <map>

#include "loopp/core/MainLoop.hpp"
#include "loopp/mqtt/MqttClient.hpp"

#include "IDriver.hpp"

#include "esp_log.h"

namespace loopp
{
  namespace drivers
  {
    class DriverContext
    {
    public:
      DriverContext() = default;
      DriverContext(std::shared_ptr<loopp::core::MainLoop> loop, std::shared_ptr<loopp::mqtt::MqttClient> mqtt, std::string topic_root)
        : loop(loop)
        , mqtt(mqtt)
        , topic_root(topic_root)
      {
      }

      std::shared_ptr<loopp::core::MainLoop> get_loop() const
      {
        return loop;
      }
      std::shared_ptr<loopp::mqtt::MqttClient> const get_mqtt()
      {
        return mqtt;
      }
      std::string get_topic_root() const
      {
        return topic_root;
      }

    private:
      std::shared_ptr<loopp::core::MainLoop> loop;
      std::shared_ptr<loopp::mqtt::MqttClient> mqtt;
      std::string topic_root;
    };

    class IDriverFactory
    {
    public:
      ~IDriverFactory() = default;
      virtual std::shared_ptr<IDriver> create(DriverContext context, nlohmann::json config) = 0;
    };

    class DriverRegistry
    {
    public:
      static DriverRegistry &instance()
      {
        static DriverRegistry instance;
        return instance;
      }

      void register_driver(const std::string &name, IDriverFactory *factory)
      {
        ESP_LOGI("REGISTRY", "Register %s", name.c_str());
        if (factories.find(name) == factories.end())
          {
            factories[name] = factory;
          }
      }

      std::shared_ptr<IDriver> create(const std::string &name, DriverContext context, nlohmann::json config)
      {
        ESP_LOGI("REGISTRY", "Create %s", name.c_str());
        if (factories.find(name) != factories.end())
          {
            return factories[name]->create(context, config);
          }
        return std::shared_ptr<IDriver>();
      }

    private:
      std::map<std::string, IDriverFactory *> factories;
      ;
    };

    template<typename T>
    class DriverFactory : public IDriverFactory
    {
    public:
      DriverFactory() = default;

      DriverFactory(std::string name)
      {
        ESP_LOGI("REGISTRY", "DriverFactory %s", name.c_str());
        DriverRegistry::instance().register_driver(std::move(name), this);
      }

      std::shared_ptr<IDriver> create(DriverContext context, nlohmann::json config)
      {
        ESP_LOGI("REGISTRY", "DriverFactory create");
        return std::make_shared<T>(context, config);
      }
    };

  } // namespace drivers
} // namespace loopp

#define LOOPP_CONCAT(a, b) a##b
#define LOOPP_LABEL(a, b) LOOPP_CONCAT(a, b)
#define LOOPP_UNIQUE_NAME(name) LOOPP_LABEL(name, __LINE__)

#define LOOPP_REGISTER_DRIVER(name, klass)                                                                                                 \
  namespace                                                                                                                                \
  {                                                                                                                                        \
    loopp::drivers::DriverFactory<klass> __attribute__((used)) LOOPP_UNIQUE_NAME(global_driver)(name);                                     \
  }

#endif // DRIVERREGISTRY_HH
