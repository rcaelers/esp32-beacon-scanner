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

#include "loopp/utils/hexdump.hpp"

#include <iostream>
#include <iomanip>
#include <algorithm>

#include "esp_log.h"

namespace loopp
{
  namespace utils
  {
    void hexdump(const char *tag, const uint8_t *data, std::size_t size)
    {
      hexdump(tag, "", data, size);
    }

    void hexdump(const char *tag, const char *prefix, const uint8_t *data, std::size_t size)
    {

      unsigned int num_lines = (size + 15) / 16;
      unsigned int index = 0;

      for (unsigned int line = 0; line < num_lines; line++)
        {
          std::stringstream ss;
          ss << std::hex << std::setfill('0');
          ss << std::setw(8) << index;

          for (unsigned int column = 0; column < 16; column++)
            {
              if (column % 8 == 0)
                {
                  ss << ' ';
                }

              if (index + column < size)
                {
                  ss << ' ' << std::setw(2) << static_cast<unsigned>(data[index + column]);
                }
              else
                {
                  ss << "   ";
                }
            }

          ss << "  ";
          for (unsigned int column = 0; column < std::min(std::size_t(16), size - index); column++)
            {
              if (data[index + column] < 32)
                {
                  ss << '.';
                }
              else
                {
                  ss << data[index + column];
                }
            }

          ESP_LOGD(tag, "%s%s", prefix, ss.str().c_str());
          index += 16;
        }
    }
  } // namespace utils
} // namespace loopp
