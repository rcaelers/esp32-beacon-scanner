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

#include "loopp/drivers/DriverRegistry.hpp"

using namespace loopp;
using namespace loopp::drivers;

DriverContext::DriverContext(std::shared_ptr<loopp::core::MainLoop> loop, std::shared_ptr<loopp::mqtt::MqttClient> mqtt, std::string topic_root)
  : loop(loop)
  , mqtt(mqtt)
  , topic_root(topic_root)
{
}

std::shared_ptr<loopp::core::MainLoop>
DriverContext::get_loop() const
{
  return loop;
}

std::shared_ptr<loopp::mqtt::MqttClient> const
DriverContext::get_mqtt()
{
  return mqtt;
}

std::string
DriverContext::get_topic_root() const
{
  return topic_root;
}

 DriverRegistry &
DriverRegistry::instance()
{
  static DriverRegistry instance;
  return instance;
}

void
DriverRegistry::register_driver(const std::string &name, IDriverFactory *factory)
{
  if (factories.find(name) == factories.end())
    {
      factories[name] = factory;
    }
}

std::shared_ptr<IDriver>
DriverRegistry::create(const std::string &name, DriverContext context, const nlohmann::json &config)
{
  if (factories.find(name) != factories.end())
    {
      return factories[name]->create(context, config);
    }
  return std::shared_ptr<IDriver>();
}
