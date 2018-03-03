// Copyright (C) 2017, 2018 Rob Caelers <rob.caelers@gmail.com>
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

#ifndef LOOPP_CORE_SIGNAL_HPP
#define LOOPP_CORE_SIGNAL_HPP

#include <list>
#include <functional>
#include <memory>

#include "loopp/core/Mutex.hpp"
#include "loopp/core/ScopedLock.hpp"

namespace loopp
{
  namespace core
  {
    template<class T>
    class Signal;

    namespace detail
    {
      using disconnector_type = std::function<void()>;
    }

    class Connection
    {
    public:
      Connection() = default;

      explicit Connection(std::weak_ptr<detail::disconnector_type> disconnector)
        : disconnector(disconnector)
      {
      }

      void disconnect()
      {
        std::shared_ptr<detail::disconnector_type> d = disconnector.lock();
        if (d != nullptr)
          {
            (*d)();
          }
      }

      bool operator==(const Connection &other) const
      {
        std::shared_ptr<detail::disconnector_type> d = disconnector.lock();
        std::shared_ptr<detail::disconnector_type> dother = other.disconnector.lock();
        return d == dother;
      }

      bool operator!=(const Connection &other) const
      {
        return !(this->operator==(other));
      }

    private:
      std::weak_ptr<detail::disconnector_type> disconnector;
    };

    class ScopedConnection
    {
    public:
      ScopedConnection() = default;

      ScopedConnection(const Connection &connection)
        : connection(connection)
      {
      }

      ~ScopedConnection()
      {
        connection.disconnect();
      }

      ScopedConnection &operator=(const Connection &connection)
      {
        if (this->connection != connection)
          {
            this->connection.disconnect();
            this->connection = connection;
          }
        return *this;
      }

      void disconnect()
      {
        connection.disconnect();
      }

    private:
      Connection connection;
    };

    template<class... Args>
    class Signal<void(Args...)>
    {
      using function_type = std::function<void(Args...)>;
      using self_type = Signal<void(Args...)>;

      struct Slot
      {
      public:
        Slot(function_type fn)
          : fn(std::move(fn))
        {
        }

        function_type fn;
        std::shared_ptr<detail::disconnector_type> disconnector;
      };

      using slot_list_type = std::list<Slot>;

    public:
      Signal() = default;
      ~Signal() = default;

      Signal(const Signal &) = delete;
      Signal &operator=(const Signal &) = delete;

      Signal(Signal &&lhs) = default;
      Signal &operator=(Signal &&lhs) = default;

      Connection connect(const function_type function)
      {
        ScopedLock l(mutex);

        typename slot_list_type::iterator slot_it = slots.emplace(slots.end(), function);
        slot_it->disconnector = std::make_shared<detail::disconnector_type>([this, slot_it]() { disconnect(slot_it); });
        return Connection{ std::weak_ptr<detail::disconnector_type>(slot_it->disconnector) };
      }

      void operator()(const Args &... args)
      {
        ScopedLock l(mutex);
        for (auto &slot : slots)
          {
            slot.fn(args...);
          }
      }

    private:
      void disconnect(typename slot_list_type::iterator it)
      {
        ScopedLock l(mutex);
        slots.erase(it);
      }

    private:
      mutable loopp::core::Mutex mutex;
      slot_list_type slots;
    };
  } // namespace core
} // namespace loopp

#endif // LOOPP_CORE_SIGNAL_HPP
