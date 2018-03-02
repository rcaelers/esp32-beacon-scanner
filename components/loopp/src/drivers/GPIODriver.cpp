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

#include "loopp/drivers/GPIODriver.hpp"

#include <string>
#include <algorithm>

#include "esp_log.h"
#include "driver/gpio.h"

#include "loopp/core/ScopedLock.hpp"

using namespace loopp::drivers;
using namespace loopp::drivers::details;

using json = nlohmann::json;

static const char tag[] = "GPIO";

// LOOPP_REGISTER_DRIVER("gpio", GPIODriver);

GPIODriver::GPIODriver(loopp::drivers::DriverContext context, nlohmann::json config)
  : queue(std::make_shared<loopp::core::QueueISR<gpio_num_t>>())
  , task(std::make_shared<loopp::core::Task>("gpio_task", std::bind(&GPIODriver::gpio_task, this), loopp::core::Task::CoreId::NoAffinity, 2048))
{
  auto it = config.find("pins");
  if (it != config.end())
    {
      for (auto pin_config : config.at("pins"))
        {
          std::shared_ptr<GPIOPin> pin = std::make_shared<GPIOPin>(context, queue, pin_config);
          gpios[pin->get_pin_no()] = pin;
        }
    }
}

GPIODriver::~GPIODriver()
{
  stop();
}

void
GPIODriver::start()
{
  gpio_install_isr_service(0);
  for (auto kv : gpios)
    {
      kv.second->start();
    }
}

void
GPIODriver::stop()
{
  for (auto kv : gpios)
    {
      kv.second->stop();
    }
  gpio_uninstall_isr_service();
}

void
GPIODriver::gpio_task()
{
  while (true)
    {
      gpio_num_t pin_no;
      bool ok = queue->pop(pin_no);
      if (ok)
        {
          loopp::core::ScopedLock l(mutex);
          std::shared_ptr<GPIOPin> pin = gpios[pin_no];
          if (pin)
            {
              pin->trigger();
            }
        }
    }
}

GPIOPin::GPIOPin(loopp::drivers::DriverContext context, std::shared_ptr<loopp::core::QueueISR<gpio_num_t>> queue, nlohmann::json config)
  : loop(context.get_loop())
  , mqtt(context.get_mqtt())
  , queue(queue)
{
  pin.pin_bit_mask = 0;
  pin.mode = GPIO_MODE_DISABLE;
  pin.pull_down_en = GPIO_PULLDOWN_DISABLE;
  pin.pull_up_en = GPIO_PULLUP_DISABLE;
  pin.intr_type = GPIO_INTR_ANYEDGE;

  std::string path = config["path"].get<std::string>();
  topic = context.get_topic_root() + path;

  pin_no = static_cast<gpio_num_t>(config["pin"].get<int>());
  pin.pin_bit_mask = (1ULL << pin_no);

  ESP_LOGI(tag, "-> Path      : %s", path.c_str());
  ESP_LOGI(tag, "-> Pin       : %d", pin_no);

  set_pin_direction(config);
  set_pin_mode(config);
  set_pin_trigger(config);

  auto it = config.find("invert");
  if (it != config.end())
    {
      invert = *it;
    }

  if (is_in())
    {
      it = config.find("debounce");
      if (it != config.end())
        {
          debounce = pdMS_TO_TICKS(*it);
          ESP_LOGI(tag, "-> Debounce  : %d", debounce);
        }
      it = config.find("retain");
      if (it != config.end())
        {
          retain = *it;
          ESP_LOGI(tag, "-> Retain    : %d", retain);
        }
    }

  if (is_out())
    {
      it = config.find("initial");
      if (it != config.end())
        {
          initial = *it;
          ESP_LOGI(tag, "-> Initial   : %d", retain);
        }
    }
}

GPIOPin::~GPIOPin()
{
  stop();
}

void
GPIOPin::start()
{
  loopp::core::ScopedLock l(mutex);
  if (started)
    {
      return;
    }

  gpio_config(&pin);

  if (is_in())
    {
      gpio_isr_handler_add(pin_no, gpio_isr_handler, static_cast<GPIOPin *>(this));
      gpio_intr_enable(pin_no);
    }

  if (is_out())
    {
      gpio_set_level(pin_no, (initial != invert) ? 1 : 0);

      mqtt->subscribe(topic);
      auto self = shared_from_this();
      mqtt->add_filter(topic, loopp::core::bind_loop(loop, [this, self](std::string topic, std::string payload) {
                         std::transform(payload.begin(), payload.end(), payload.begin(), ::tolower);
                         bool on = (payload == "true" || payload == "1" || payload == "on" || payload == "yes");
                         gpio_set_level(pin_no, (on != invert) ? 1 : 0);
                       }));
    }

  started = true;
}

void
GPIOPin::stop()
{
  loopp::core::ScopedLock l(mutex);
  if (!started)
    {
      return;
    }

  gpio_set_direction(pin_no, GPIO_MODE_DISABLE);
  gpio_set_pull_mode(pin_no, GPIO_FLOATING);

  if (is_in())
    {
      gpio_intr_disable(pin_no);
      gpio_isr_handler_remove(pin_no);
    }

  if (is_out())
    {
      mqtt->unsubscribe(topic);
      mqtt->remove_filter(topic);
    }

  started = false;
}

void
GPIOPin::set_pin_direction(nlohmann::json config)
{
  auto it = config.find("direction");
  if (it != config.end())
    {
      std::string direction = *it;
      ESP_LOGI(tag, "-> Direction : %s", direction.c_str());

      if (direction == "in")
        {
          pin.mode = GPIO_MODE_INPUT;
        }
      else if (direction == "out")
        {
          pin.mode = GPIO_MODE_OUTPUT;
        }
      else if (direction == "inout")
        {
          pin.mode = GPIO_MODE_INPUT_OUTPUT;
        }
      else if (direction == "inout-od")
        {
          pin.mode = GPIO_MODE_OUTPUT_OD;
        }
      else if (direction == "out-od")
        {
          pin.mode = GPIO_MODE_INPUT_OUTPUT_OD;
        }
      else
        {
          throw std::runtime_error("invalid direction value: " + direction);
        }
    }
}

void
GPIOPin::set_pin_mode(nlohmann::json config)
{
  std::string mode = "pulldown";

  auto it = config.find("pull-down");
  if (it != config.end())
    {
      pin.pull_down_en = *it ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
      ESP_LOGI(tag, "-> Pull Down : %s", *it ? "Yes" : "No");
    }
  it = config.find("pull-up");
  if (it != config.end())
    {
      pin.pull_up_en = *it ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
      ESP_LOGI(tag, "-> Pull Up   : %s", *it ? "Yes" : "No");
    }
}

void
GPIOPin::set_pin_trigger(nlohmann::json config)
{
  auto it = config.find("trigger");
  if (it != config.end())
    {
      std::string trigger = *it;
      ESP_LOGI(tag, "-> Trigger   : %s", trigger.c_str());
      if (trigger == "rising")
        {
          pin.intr_type = GPIO_INTR_POSEDGE;
        }
      else if (trigger == "falling")
        {
          pin.intr_type = GPIO_INTR_NEGEDGE;
        }
      else if (trigger == "any")
        {
          pin.intr_type = GPIO_INTR_ANYEDGE;
        }
      else if (trigger == "low")
        {
          pin.intr_type = GPIO_INTR_LOW_LEVEL;
        }
      else if (trigger == "high")
        {
          pin.intr_type = GPIO_INTR_HIGH_LEVEL;
        }
      else
        {
          throw std::runtime_error("invalid trigger value: " + trigger);
        }
    }
}

void IRAM_ATTR
GPIOPin::gpio_isr_handler(void *arg)
{
  GPIOPin *self = static_cast<GPIOPin *>(arg);
  self->queue->push(self->pin_no);
}

void
GPIOPin::trigger()
{
  loopp::core::ScopedLock l(mutex);
  TickType_t current_tick = xTaskGetTickCount();
  if (current_tick - last_tick > debounce)
    {
      ESP_LOGD(tag, "Pin: %d (debounced)", pin_no);

      bool on = (gpio_get_level(pin_no) != 0) != invert;
      std::string payload = on ? "1" : "0";

      auto self = shared_from_this();
      loop->invoke([this, self, payload]() {
        mqtt->publish(topic, payload, retain ? loopp::mqtt::PublishOptions::Retain : loopp::mqtt::PublishOptions::None);
      });
    }
  last_tick = current_tick;
}

gpio_num_t
GPIOPin::get_pin_no() const
{
  return pin_no;
}

bool
GPIOPin::is_in() const
{
  return pin.mode == GPIO_MODE_INPUT || pin.mode == GPIO_MODE_INPUT_OUTPUT;
}

bool
GPIOPin::is_out() const
{
  return pin.mode == GPIO_MODE_OUTPUT || pin.mode == GPIO_MODE_INPUT_OUTPUT || pin.mode == GPIO_MODE_OUTPUT_OD ||
         pin.mode == GPIO_MODE_INPUT_OUTPUT_OD;
}
