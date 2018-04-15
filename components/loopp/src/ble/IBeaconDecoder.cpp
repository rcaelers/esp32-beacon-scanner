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

#include "IBeaconDecoder.hpp"

#include <boost/endian/conversion.hpp>

using namespace loopp;
using namespace loopp::ble;

IBeaconDecoder::IBeaconDecoder()
{
}

std::string
IBeaconDecoder::uuid_as_string(const uint8_t uuid[16]) const
{
  std::stringstream ss;
  ss << std::hex << std::right;
  ss.fill('0');
  ss.width(2);

  for (int i = 0; i < 16; i++)
    {
      ss << static_cast<unsigned int>(uuid[i]);
      if (i == 3 || i == 5 || i == 7 || i == 9)
        {
          ss << '-';
        }
    }

  return ss.str();
}

void
IBeaconDecoder::decode(const std::string &adv_data, nlohmann::json &info) const
{
  BOOST_STATIC_ASSERT(sizeof(ibeacon_data_t) == 30u);

  if (matches(adv_data))
    {
      const ibeacon_data_t *data = reinterpret_cast<const ibeacon_data_t *>(adv_data.data());

      nlohmann::json j;
      j["uuid"] = uuid_as_string(data->uuid);
      j["major"] = data->major.value();
      j["minor"] = data->minor.value();
      j["power"] = data->power;

      info["ibeacon"] = j;
    }
}

bool
IBeaconDecoder::matches(const std::string &adv_data) const
{
  static uint8_t ibeacon_prefix[] =
    {
      0x02, 0x01, 0x00, 0x1A, 0xFF, 0x4C, 0x00, 0x02, 0x15
    };

  for (int i = 0; i < sizeof(ibeacon_prefix); i++)
    {
      if (i != 2 && adv_data[i] != ibeacon_prefix[i])
        {
          return false;
        }
    }

  return true;
}
