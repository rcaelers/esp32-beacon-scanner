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
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "esp_log.h"

#include "loopp/net/Resolver.hpp"
#include "loopp/net/NetworkErrors.hpp"

static const char *tag = "NET";

using namespace loopp;
using namespace loopp::net;

Resolver &
Resolver::instance()
{
  static Resolver instance;
  return instance;
}

Resolver::Resolver()
  : task("resolve_task", std::bind(&Resolver::resolve_task, this), loopp::core::Task::CoreId::NoAffinity, 2048)
{
}

void
Resolver::resolve_async(std::string host, std::string port, resolved_callback_type callback)
{
  ESP_LOGD(tag, "Resolving %s:%s", host.c_str(), port.c_str());
  queue.emplace(std::move(host), std::move(port), std::move(callback));
}

void
Resolver::resolve_task()
{
  while (true)
    {
      nonstd::optional<ResolveJob> resolve_job(queue.pop());
      if (resolve_job)
        {
          int ret;
          std::error_code ec;
          struct addrinfo hints {};
          struct addrinfo *addr_list = nullptr;

          memset(&hints, 0, sizeof(hints));
          hints.ai_family = AF_UNSPEC;

          ret = getaddrinfo(resolve_job->host.c_str(), resolve_job->port.c_str(), &hints, &addr_list);
          if (ret != 0)
            {
              ESP_LOGE(tag, "Failed to resolve %s, error %d", resolve_job->host.c_str(), ret);
              ec = NetworkErrc::NameResolutionFailed;
            }

          resolve_job->callback(ec, addr_list);
        }
    }
}
