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

#ifndef GPIODRIVER_HH
#define GPIODRIVER_HH

#include <string>
#include <memory>

#include "loopp/core/MainLoop.hpp"
#include "loopp/core/Mutex.hpp"
#include "loopp/mqtt/MqttClient.hpp"
#include "loopp/core/QueueISR.hpp"

#include "loopp/utils/json.hpp"

#include "loopp/drivers/DriverRegistry.hpp"

namespace loopp
{
  namespace drivers
  {
    class GPIODriver;

    namespace details
    {
      class GPIOPin : public std::enable_shared_from_this<GPIOPin>
      {
      public:
        GPIOPin(loopp::drivers::DriverContext context, std::shared_ptr<loopp::core::QueueISR<gpio_num_t>> queue, nlohmann::json config);
        ~GPIOPin();

        void start();
        void stop();

        void trigger();

        gpio_num_t get_pin_no() const;
        bool is_in() const;
        bool is_out() const;

      private:
        void set_pin_direction(nlohmann::json config);
        void set_pin_mode(nlohmann::json config);
        void set_pin_trigger(nlohmann::json config);

        static void IRAM_ATTR gpio_isr_handler(void *arg);

      private:
        mutable loopp::core::Mutex mutex;
        std::shared_ptr<loopp::core::MainLoop> loop;
        std::shared_ptr<loopp::mqtt::MqttClient> mqtt;
        std::shared_ptr<loopp::core::QueueISR<gpio_num_t>> queue;

        gpio_config_t pin;
        gpio_num_t pin_no;
        std::string topic;
        bool initial = false;
        bool invert = false;
        bool retain = false;
        int debounce = 0;
        TickType_t last_tick = 0;
        bool started = false;
      };
    } // namespace details

    class GPIODriver : public loopp::drivers::IDriver, public std::enable_shared_from_this<GPIODriver>
    {
    public:
      GPIODriver(loopp::drivers::DriverContext context, nlohmann::json config);
      ~GPIODriver();

    private:
      virtual void start() override;
      virtual void stop() override;

      void gpio_task();

    private:
      mutable loopp::core::Mutex mutex;
      std::shared_ptr<loopp::core::MainLoop> loop;
      std::shared_ptr<loopp::mqtt::MqttClient> mqtt;
      std::map<gpio_num_t, std::shared_ptr<details::GPIOPin>> gpios;
      std::shared_ptr<loopp::core::QueueISR<gpio_num_t>> queue;
      std::shared_ptr<loopp::core::Task> task;
    };

  } // namespace drivers
} // namespace loopp

#endif // GPIODRIVER_HH
