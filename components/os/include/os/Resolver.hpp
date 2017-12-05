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

#ifndef OS_RESOLVER_HPP
#define OS_RESOLVER_HPP

#include <string>
#include <system_error>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include "os/Task.hpp"
#include "os/Queue.hpp"
#include "os/Slot.hpp"

namespace os
{
  class Resolver
  {
  public:
    using resolved_slot_type = os::Slot<void (std::error_code ec, struct addrinfo *addr_list)>;

    static Resolver &instance();
    void resolve_async(std::string host, std::string port, resolved_slot_type slot);

  private:
    Resolver();
    ~Resolver() = default;

    void resolve_task();

  private:
    struct ResolveJob
    {
    public:
      ResolveJob(std::string host, std::string port, resolved_slot_type slot)
        : host(std::move(host)),
          port(std::move(port)),
          slot(std::move(slot))
      {
      }
      std::string host;
      std::string port;
      resolved_slot_type slot;
    };

    os::Queue<ResolveJob> queue;
    os::Task task;
  };
}

#endif // OS_RESOLVER_HPP
