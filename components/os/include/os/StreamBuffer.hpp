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

#ifndef OS_STREAM_BUFFER_HPP
#define OS_STREAM_BUFFER_HPP

#include <cstring>
#include <stdexcept>
#include <streambuf>
#include <vector>

namespace os
{
  class StreamBuffer : public std::streambuf
  {
  public:
    explicit StreamBuffer(std::size_t max_buffer_size = DEFAULT_MAX_BUFFER_SIZE)
      : max_buffer_size(max_buffer_size)
    {
      buffer.resize(std::min<std::size_t>(max_buffer_size, std::size_t(BUFFER_INCREASE_SIZE)));
      setg(&buffer[0], &buffer[0], &buffer[0]);
      setp(&buffer[0], &buffer[0] + buffer.size());
    }

    std::size_t max_size() const noexcept
    {
      return max_buffer_size;
    }

    char *produce_data(std::size_t n)
    {
      // TODO: return nullptr when buffer is full?
      reserve(n);
      return pptr();
    }

    void produce_commit(std::size_t n)
    {
      n = std::min<std::size_t>(n, epptr() - pptr());
      pbump(static_cast<int>(n));
      setg(eback(), gptr(), pptr());
    }

    char *consume_data() const noexcept
    {
      return gptr();
    }

    std::size_t consume_size() const noexcept
    {
      return pptr() - gptr();
    }

    void consume_commit(std::size_t n)
    {
      n = std::min<std::size_t>(n, consume_size());
      gbump(static_cast<int>(n));
    }

  private:
    int_type underflow()
    {
      return gptr() < egptr() ?
        traits_type::to_int_type(*gptr()) :
        traits_type::eof();
    }

    int_type overflow(int_type ch)
    {
      if (!traits_type::eq_int_type(ch, traits_type::eof()))
        {
          // TODO: return eof when buffer is full?
          reserve(BUFFER_INCREASE_SIZE);
          *pptr() = traits_type::to_char_type(ch);
          pbump(1);
          return ch;
        }
      else
        {
          return traits_type::not_eof(ch);
        }
    }

    void reserve(std::size_t n)
    {
      std::size_t available = epptr() - pptr();
      if (available < n)
        {
          std::size_t pptr_offset = pptr() - &buffer[0];
          if (gptr() > &buffer[0])
            {
              std::memmove(&buffer[0], gptr(), consume_size());
              pptr_offset -= (gptr() - &buffer[0]);
              available += (gptr() - &buffer[0]);
            }

          if (available < n)
            {
              std::size_t new_size = buffer.size() + n - available;
              if (new_size > max_buffer_size)
                {
                  throw std::length_error("stream buffer full");
                }
              buffer.resize(new_size);
            }

          setg(&buffer[0], &buffer[0], &buffer[0] + pptr_offset);
          setp(&buffer[0] + pptr_offset, &buffer[0] + buffer.size());
        }
    }

private:
    std::size_t max_buffer_size;
    std::vector<char> buffer;

    static constexpr std::size_t BUFFER_INCREASE_SIZE = 100;
    static constexpr std::size_t DEFAULT_MAX_BUFFER_SIZE = 10 * 1024;
  };
}

#endif // OS_STREAM_BUFFER_HPP
