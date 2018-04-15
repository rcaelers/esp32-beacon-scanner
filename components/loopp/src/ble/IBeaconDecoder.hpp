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

#ifndef LOOPP_BLE_IBEACON_DECODER_HPP
#define LOOPP_BLE_IBEACON_DECODER_HPP

#include <string>

#include "loopp/ble/AdvertisementDecoder.hpp"

#include "loopp/utils/json.hpp"
#include "boost/endian/arithmetic.hpp"

namespace loopp
{
  namespace ble
  {
    class IBeaconDecoder : public Decoder
    {
    public:
      IBeaconDecoder();
      void decode(const std::string &adv_data, nlohmann::json &info) const;

    private:
      bool matches(const std::string &adv_data) const;
      std::string uuid_as_string(const uint8_t uuid[16]) const;

      struct ibeacon_data_t
      {
        uint8_t  prefix[9];
        uint8_t  uuid[16];
        boost::endian::big_uint16_t major;
        boost::endian::big_uint16_t minor;
        int8_t   power;
      };
    };
  }
}

#endif // LOOPP_BLE_IBEACON_DECODER_HPP
