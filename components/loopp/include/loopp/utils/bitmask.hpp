// Copyright (c) 2014 Dmitry Arkhipov
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
//
// Based on https://github.com/grisumbras/enum-flags

#ifndef LOOPP_BITMASK_HPP
#define LOOPP_BITMASK_HPP

#include <type_traits>

namespace loopp
{
  template <class Enum, class Enabler = void>
  struct enable_bitmask : public std::false_type
  {
  };

  template <class Enum>
  struct BitMask
  {
  public:
    static_assert(enable_bitmask<Enum>::value, "use DEFINE_BITMASK to enable bitmask for enum class");

    using enum_type = typename std::decay<Enum>::type;
    using underlying_type = typename std::underlying_type<enum_type>::type;
    using impl_type = typename std::make_unsigned<underlying_type>::type;

  public:
    BitMask() noexcept = default;

    BitMask(const BitMask &) noexcept = default;
    BitMask &operator=(const BitMask &) noexcept = default;

    BitMask(BitMask &&) noexcept = default;
    BitMask &operator=(BitMask &&) noexcept = default;

    constexpr BitMask(enum_type e) noexcept
      : val(static_cast<impl_type>(e))
    {
    }

    BitMask &operator=(enum_type e) noexcept
    {
      val = static_cast<impl_type>(e);
      return *this;
    }

    constexpr explicit operator bool() const noexcept
    {
      return val;
    }

    constexpr bool operator!() const noexcept
    {
      return !val;
    }

    constexpr BitMask operator~() const noexcept
    {
      return BitMask(~val);
    }

    BitMask &operator&=(const BitMask &bitmask) noexcept
    {
      val &= bitmask.val;
      return *this;
    }

    BitMask &operator|=(const BitMask &bitmask) noexcept
    {
      val |= bitmask.val;
      return *this;
    }

    BitMask &operator^=(const BitMask &bitmask) noexcept
    {
      val ^= bitmask.val;
      return *this;
    }

    // BitMask &operator&=(enum_type e) noexcept
    // {
    //   val &= static_cast<impl_type>(e);
    //   return *this;
    // }

    // BitMask &operator|=(enum_type e) noexcept
    // {
    //   val |= static_cast<impl_type>(e);
    //   return *this;
    // }

    // BitMask &operator^=(enum_type e) noexcept
    // {
    //   val ^= static_cast<impl_type>(e);
    //   return *this;
    // }

    friend constexpr bool operator==(BitMask lhs, BitMask rhs)
    {
      return lhs.val == rhs.val;
    }

    friend constexpr bool operator!=(BitMask lhs, BitMask rhs)
    {
      return lhs.val != rhs.val;
    }

    friend constexpr BitMask operator&(BitMask lhs, BitMask rhs) noexcept
    {
      return BitMask{static_cast<impl_type>(lhs.val & rhs.val)};
    }

    friend constexpr BitMask operator|(BitMask lhs, BitMask rhs) noexcept
    {
      return BitMask{static_cast<impl_type>(lhs.val | rhs.val)};
    }

    friend constexpr BitMask operator^(BitMask lhs, BitMask rhs) noexcept
    {
      return BitMask{static_cast<impl_type>(lhs.val ^ rhs.val)};
    }

    constexpr underlying_type value() const noexcept
    {
      return static_cast<underlying_type>(val);
    }

    void set(underlying_type v) noexcept
    {
      val = static_cast<impl_type>(v);
    }

private:
    constexpr explicit BitMask(impl_type val) noexcept : val(val)
    {
    }

    impl_type val;
};

template <class Enum>
constexpr auto operator&(Enum e1, Enum e2) noexcept
    -> typename std::enable_if<enable_bitmask<Enum>::value, loopp::BitMask<Enum>>::type
{
  return loopp::BitMask<Enum>(e1) & e2;
}

template <class Enum>
constexpr auto operator|(Enum e1, Enum e2) noexcept
    -> typename std::enable_if<enable_bitmask<Enum>::value, loopp::BitMask<Enum>>::type
{
  return loopp::BitMask<Enum>(e1) | e2;
}

template <class Enum>
constexpr auto operator^(Enum e1, Enum e2) noexcept
    -> typename std::enable_if<enable_bitmask<Enum>::value, loopp::BitMask<Enum>>::type
{
  return loopp::BitMask<Enum>(e1) ^ e2;
}
}

#define DEFINE_BITMASK(name)                    \
  template <>                                   \
  struct enable_bitmask<name>                   \
    : std::true_type                            \
  {                                             \
  };

#endif // LOOPP_BITMASK_HPP
