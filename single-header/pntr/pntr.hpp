////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//  MIT License                                                                                   //
//                                                                                                //
//  Copyright (c) 2023 John Plate (john.plate@gmx.com)                                            //
//                                                                                                //
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this          //
//  software and associated documentation files (the "Software"), to deal in the Software         //
//  without restriction, including without limitation the rights to use, copy, modify, merge,     //
//  publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons    //
//  to whom the Software is furnished to do so, subject to the following conditions:              //
//                                                                                                //
//  The above copyright notice and this permission notice shall be included in all copies or      //
//  substantial portions of the Software.                                                         //
//                                                                                                //
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,           //
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR      //
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE     //
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR          //
//  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
//  DEALINGS IN THE SOFTWARE.                                                                     //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                        pntr/common.hpp                                         //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <iostream>
#include <type_traits>
#include <utility>

#ifndef PNTR_NAMESPACE
  #define PNTR_NAMESPACE pntr
#endif

// clang-format off
#define PNTR_NAMESPACE_BEGIN namespace PNTR_NAMESPACE {
#define PNTR_NAMESPACE_END   }
// clang-format on

#ifndef PNTR_ASSERT
  #include <cassert>
  #define PNTR_ASSERT(x_condition) assert(x_condition)
#endif


#ifndef PNTR_ENABLE_LOGGING
  #ifdef NDEBUG
    #define PNTR_ENABLE_LOGGING 0
  #else
    #define PNTR_ENABLE_LOGGING 1
  #endif
#endif

#ifndef PNTR_LOG_MESSAGE
  #if PNTR_ENABLE_LOGGING
    #define PNTR_LOG_MESSAGE(x_severity, x_message) \
      std::cerr << #x_severity << " in " << __FILE__ << ", line " << __LINE__ << ": " << (x_message) << std::endl
  #else
    #define PNTR_LOG_MESSAGE(x_severity, x_message) static_cast<void>(0)
  #endif
#endif

#ifndef PNTR_LOG_WARNING
  #if PNTR_ENABLE_LOGGING
    #define PNTR_LOG_WARNING(x_message) PNTR_LOG_MESSAGE(Warning, x_message)
  #else
    #define PNTR_LOG_WARNING(x_message) static_cast<void>(0)
  #endif
#endif

#ifndef PNTR_LOG_ERROR
  #if PNTR_ENABLE_LOGGING
    #define PNTR_LOG_ERROR(x_message) PNTR_LOG_MESSAGE(Error, x_message)
  #else
    #define PNTR_LOG_ERROR(x_message) static_cast<void>(0)
  #endif
#endif

#ifndef PNTR_TRY_LOG_WARNING
  #if PNTR_ENABLE_LOGGING
    #define PNTR_TRY_LOG_WARNING(x_condition, x_message) \
      do \
      { \
        if (x_condition) \
        { \
          PNTR_LOG_WARNING(x_message); \
        } \
      } \
      while (false)
  #else
    #define PNTR_TRY_LOG_WARNING(x_condition, x_message) static_cast<void>(0)
  #endif
#endif

#ifndef PNTR_TRY_LOG_ERROR
  #if PNTR_ENABLE_LOGGING
    #define PNTR_TRY_LOG_ERROR(x_condition, x_message) \
      do \
      { \
        if (x_condition) \
        { \
          PNTR_LOG_ERROR(x_message); \
        } \
      } \
      while (false)
  #else
    #define PNTR_TRY_LOG_ERROR(x_condition, x_message) static_cast<void>(0)
  #endif
#endif


PNTR_NAMESPACE_BEGIN


using StaticSupport = std::true_type;
using NoStaticSupport = std::false_type;

enum class ControlStatus
{
  e_invalid,
  e_acquired,
  e_shared
};

template<class t_shared>
class SharedPtr;

template<class t_shared>
class WeakPtr;


namespace detail
{
  template<class t_shared, class t_nothrow, typename... t_args>
  SharedPtr<t_shared> make_shared_impl(t_args &&... p_args) noexcept(t_nothrow::value);

  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>, SharedPtr<t_shared>>
  make_shared_with_deleter_impl(t_forward && p_deleter, t_args &&... p_args) noexcept(t_nothrow::value);

  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  std::enable_if_t<!std::is_void_v<typename t_shared::PntrAllocator>, SharedPtr<t_shared>>
  allocate_shared_impl(t_forward && p_allocator, t_args &&... p_args) noexcept(t_nothrow::value);
}


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                   pntr/CounterThreadSafe.hpp                                   //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>

PNTR_NAMESPACE_BEGIN


template<typename t_value>
class CounterThreadSafe
{
public:
  using ValueType = t_value;

  explicit CounterThreadSafe(t_value const p_init_value) noexcept
  : m_value(p_init_value)
  {}

  // Return the current count.
  t_value
  get_count() const noexcept
  {
    return m_value.load(std::memory_order_relaxed);
  }

  // Increment the counter by the given value and return the previous value.
  t_value
  increment(t_value const p_value) noexcept
  {
    return m_value.fetch_add(p_value, std::memory_order_relaxed);
  }

  // Decrement the counter by the given value and return the previous value.
  t_value
  decrement(t_value const p_value) noexcept
  {
    return m_value.fetch_sub(p_value, std::memory_order_acq_rel);
  }

  // If the counter equals 'p_expected' replace it with the desired value and return true.
  // Otherwise save the counter value in 'p_expected' and return false.
  // Might fail sporadically even if the counter equals expected.
  bool
  compare_exchange_weak(t_value & p_expected, t_value const p_desired) noexcept
  {
    return m_value.compare_exchange_weak(p_expected, p_desired, std::memory_order_relaxed);
  }

private:
  std::atomic<t_value> m_value;
};


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                  pntr/CounterThreadUnsafe.hpp                                  //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


template<typename t_value>
class CounterThreadUnsafe
{
public:
  using ValueType = t_value;

  explicit CounterThreadUnsafe(t_value const p_init_value) noexcept
  : m_value(p_init_value)
  {}

  // Return the current count.
  t_value
  get_count() const noexcept
  {
    return m_value;
  }

  // Increment the counter by the given value and return the previous value.
  t_value
  increment(t_value const p_value) noexcept
  {
    t_value const previous = m_value;
    m_value += p_value;
    return previous;
  }

  // Decrement the counter by the given value and return the previous value.
  t_value
  decrement(t_value const p_value) noexcept
  {
    t_value const previous = m_value;
    m_value -= p_value;
    return previous;
  }

  // If the counter equals 'p_expected' replace it with the desired value and return true.
  // Otherwise save the counter value in 'p_expected' and return false.
  bool
  compare_exchange_weak(t_value & p_expected, t_value const p_desired) noexcept
  {
    if (m_value == p_expected)
    {
      m_value = p_desired;
      return true;
    }
    p_expected = m_value;
    return false;
  }

private:
  t_value m_value;
};


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                 pntr/detail/PntrTypeTraits.hpp                                 //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


namespace detail
{
  constexpr bool
  is_power_of_two(std::uint64_t const p_value) noexcept
  {
    return (p_value != 0u && (p_value & (p_value - 1u)) == 0u);
  }

  constexpr unsigned
  log2(std::uint64_t const p_power_of_two) noexcept
  {
    return (p_power_of_two == 1u ? 0u : 1u + log2(p_power_of_two >> 1u));
  }

  constexpr unsigned
  bit_width(std::uint64_t const p_value) noexcept
  {
    return (p_value == 0u ? 0u : 1u + bit_width(p_value >> 1u));
  }

  template<typename t_type>
  inline constexpr bool is_empty_base = std::is_class_v<t_type> && std::is_empty_v<t_type> && !std::is_final_v<t_type>;

  template<class t_shared, typename = void>
  struct BaseImpl
  {
    using Type = t_shared;
  };

  template<class t_shared>
  struct BaseImpl<t_shared, std::void_t<typename t_shared::PntrSharedBase>>
  {
    using Type = std::conditional_t<std::is_const_v<t_shared>, std::add_const_t<typename t_shared::PntrSharedBase>,
                                    typename t_shared::PntrSharedBase>;
  };

  template<class t_shared>
  using BaseType = typename BaseImpl<t_shared>::Type;


  template<class t_shared, typename = void>
  struct StaticCastable: std::false_type
  {};

  template<class t_shared>
  struct StaticCastable<t_shared, std::void_t<decltype(static_cast<t_shared *>(std::declval<BaseType<t_shared> *>()))>>
  : std::true_type
  {};

  template<class t_shared>
  inline constexpr bool is_static_castable = StaticCastable<t_shared>::value;


  template<class t_shared>
  inline t_shared *
  static_or_dynamic_cast(BaseType<t_shared> * p_base) noexcept
  {
    t_shared * shared = nullptr;
    if constexpr (is_static_castable<t_shared>)
    {
      shared = static_cast<t_shared *>(p_base);
    }
    else
    {
      shared = dynamic_cast<t_shared *>(p_base);
    }
    return shared;
  }


  template<class t_allocator, typename = void>
  struct SupportsStatic: std::true_type
  {};

  template<class t_allocator>
  struct SupportsStatic<t_allocator, std::void_t<typename t_allocator::SupportsStatic>>: t_allocator::SupportsStatic
  {};

  template<class t_allocator>
  inline constexpr bool supports_static = SupportsStatic<t_allocator>::value;


  template<class t_shared>
  inline std::size_t
  calc_base_offset(t_shared * const p_shared) noexcept
  {
    BaseType<t_shared> * const shared_base = p_shared;
    std::ptrdiff_t const diff = reinterpret_cast<char *>(shared_base) - reinterpret_cast<char *>(p_shared);
    if (diff >= 0)
    {
      constexpr std::size_t s_alignof_control = alignof(typename t_shared::PntrControlType);
      std::size_t const byte_offset = static_cast<std::size_t>(diff);
      if ((byte_offset % s_alignof_control) == 0u)
      {
        return (byte_offset / s_alignof_control);
      }
    }
    PNTR_LOG_ERROR("Failed to calculate pointer offset");
    return std::numeric_limits<std::size_t>::max();
  }

  template<class t_shared_base>
  inline void *
  calc_shared_pointer(t_shared_base * const p_shared_base, std::size_t const p_offset) noexcept
  {
    constexpr std::size_t s_alignof_control = alignof(typename t_shared_base::PntrControlType);
    std::size_t const byte_offset = p_offset * s_alignof_control;
    return (reinterpret_cast<char *>(p_shared_base) - byte_offset);
  }

  template<class t_shared>
  inline constexpr std::size_t
  calc_base_size_offset() noexcept
  {
    static_assert((sizeof(t_shared) - sizeof(BaseType<t_shared>)) % alignof(typename t_shared::PntrControlType) == 0u);
    return ((sizeof(t_shared) - sizeof(BaseType<t_shared>)) / alignof(typename t_shared::PntrControlType));
  }

  template<class t_shared_base>
  inline std::size_t
  calc_shared_size(std::uint64_t const p_size_offset) noexcept
  {
    return static_cast<std::size_t>(sizeof(t_shared_base)
                                    + p_size_offset * alignof(typename t_shared_base::PntrControlType));
  }

  template<class t_shared>
  inline constexpr std::size_t
  calc_base_align_offset() noexcept
  {
    return (log2(alignof(t_shared)) - log2(alignof(BaseType<t_shared>)));
  }

  template<class t_shared_base>
  inline std::size_t
  calc_shared_align(std::uint64_t const p_align_offset) noexcept
  {
    return (static_cast<std::size_t>(1u) << (log2(alignof(t_shared_base)) + p_align_offset));
  }
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                               pntr/detail/ControlDataStorage.hpp                               //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


namespace detail
{
  // Only support architectures where one byte has eight bits.
  static_assert(static_cast<unsigned>(CHAR_BIT) == 8u);

  template<typename t_type>
  inline constexpr unsigned
  type_bits() noexcept
  {
    return static_cast<unsigned>(sizeof(t_type) * CHAR_BIT);
  }

  template<unsigned t_bits>
  using Bits = std::integral_constant<unsigned, t_bits>;

  using NoBits = Bits<0u>;
  using SharedBits = Bits<std::numeric_limits<unsigned>::max()>;

  template<class t_bits>
  struct TypeFromBitsImpl;
  template<>
  struct TypeFromBitsImpl<Bits<8u>>
  {
    using Type = std::uint8_t;
  };
  template<>
  struct TypeFromBitsImpl<Bits<16u>>
  {
    using Type = std::uint16_t;
  };
  template<>
  struct TypeFromBitsImpl<Bits<32u>>
  {
    using Type = std::uint32_t;
  };
  template<>
  struct TypeFromBitsImpl<Bits<64u>>
  {
    using Type = std::uint64_t;
  };
  template<class t_bits>
  using TypeFromBits = typename TypeFromBitsImpl<t_bits>::Type;

  template<class t_bits>
  inline constexpr bool
  is_power_of_two() noexcept
  {
    return is_power_of_two(t_bits::value);
  }


  // ControlDataStorage is a data structure that is able to combine usage and weak count,
  // pointer offset, type size and alignment, and a user value for the remaining bits.
  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_size_bits, class t_align_bits, typename = void>
  class ControlDataStorage
  {
    static constexpr unsigned s_storage_bits = type_bits<t_storage>();

  public:
    using StorageType = t_storage;
    using UsageValueType = t_storage;
    using WeakValueType = t_storage;
    using DataValueType = t_storage;

    static constexpr bool s_shared_offset = std::is_same_v<t_offset_bits, SharedBits>;
    using OffsetValueType = t_storage;

    static constexpr unsigned s_usage_bits = t_usage_bits::value;
    static constexpr unsigned s_weak_bits = t_weak_bits::value;
    static constexpr unsigned s_offset_bits = (s_shared_offset ? 0u : t_offset_bits::value);
    static constexpr unsigned s_size_bits = t_size_bits::value;
    static constexpr unsigned s_align_bits = t_align_bits::value;
    static constexpr unsigned s_user_bits =
      s_storage_bits - s_usage_bits - s_weak_bits - s_offset_bits - s_size_bits - s_align_bits;

    static constexpr unsigned s_weak_shift = s_usage_bits;
    static constexpr unsigned s_offset_shift = s_weak_shift + s_weak_bits;
    static constexpr unsigned s_size_shift = s_offset_shift + s_offset_bits;
    static constexpr unsigned s_align_shift = s_size_shift + s_size_bits;
    static constexpr unsigned s_user_shift = s_align_shift + s_align_bits;

    ControlDataStorage(UsageValueType const p_usage_init, WeakValueType const p_weak_init,
                       DataValueType const p_data_init) noexcept
    : m_counter(p_usage_init | p_weak_init | p_data_init)
    {}

    template<unsigned t_bits = s_usage_bits, typename = std::enable_if_t<t_bits != 0u>>
    t_counter<UsageValueType> &
    usage_counter() noexcept
    {
      return m_counter;
    }

    template<unsigned t_bits = s_usage_bits, typename = std::enable_if_t<t_bits != 0u>>
    t_counter<UsageValueType> const &
    usage_counter() const noexcept
    {
      return m_counter;
    }

    template<unsigned t_bits = s_weak_bits, typename = std::enable_if_t<t_bits != 0u>>
    t_counter<WeakValueType> &
    weak_counter() noexcept
    {
      return m_counter;
    }

    template<unsigned t_bits = s_weak_bits, typename = std::enable_if_t<t_bits != 0u>>
    t_counter<WeakValueType> const &
    weak_counter() const noexcept
    {
      return m_counter;
    }

    template<unsigned t_bits = s_offset_bits + s_size_bits + s_align_bits + s_user_bits,
             typename = std::enable_if_t<t_bits != 0u>>
    t_counter<DataValueType> &
    data() noexcept
    {
      return m_counter;
    }

    template<unsigned t_bits = s_offset_bits + s_size_bits + s_align_bits + s_user_bits,
             typename = std::enable_if_t<t_bits != 0u>>
    t_counter<DataValueType> const &
    data() const noexcept
    {
      return m_counter;
    }

  private:
    t_counter<t_storage> m_counter;

    static_assert(std::is_unsigned_v<t_storage>);
    static_assert(s_usage_bits + s_weak_bits + s_offset_bits + s_size_bits + s_align_bits <= s_storage_bits);

    // The below configuration should be picked up by a specialization.
    static_assert(s_usage_bits != s_storage_bits / 2u || s_storage_bits < 16u);
  };


  // Specialization for a separate usage counter if it is half of the data size.
  template<template<typename> class t_counter, typename t_storage, class t_weak_bits, class t_offset_bits,
           class t_size_bits, class t_align_bits>
  class ControlDataStorage<
    t_counter, t_storage, Bits<type_bits<t_storage>() / 2u>, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits,
    std::enable_if_t<type_bits<t_storage>() >= 16u
                     && (t_weak_bits::value < 8u || t_weak_bits::value != type_bits<t_storage>() / 4u)>>
  {
    static constexpr unsigned s_storage_bits = type_bits<t_storage>();

  public:
    using StorageType = t_storage;
    using UsageValueType = TypeFromBits<Bits<type_bits<t_storage>() / 2u>>;
    using WeakValueType = UsageValueType;
    using DataValueType = UsageValueType;

    static constexpr bool s_shared_offset = std::is_same_v<t_offset_bits, SharedBits>;
    using OffsetValueType = UsageValueType;

    static constexpr unsigned s_usage_bits = s_storage_bits / 2u;
    static constexpr unsigned s_weak_bits = t_weak_bits::value;
    static constexpr unsigned s_offset_bits = (s_shared_offset ? 0u : t_offset_bits::value);
    static constexpr unsigned s_size_bits = t_size_bits::value;
    static constexpr unsigned s_align_bits = t_align_bits::value;
    static constexpr unsigned s_user_bits =
      s_storage_bits / 2u - s_weak_bits - s_offset_bits - s_size_bits - s_align_bits;

    static constexpr unsigned s_weak_shift = 0u;
    static constexpr unsigned s_offset_shift = s_weak_bits;
    static constexpr unsigned s_size_shift = s_offset_shift + s_offset_bits;
    static constexpr unsigned s_align_shift = s_size_shift + s_size_bits;
    static constexpr unsigned s_user_shift = s_align_shift + s_align_bits;

    ControlDataStorage(UsageValueType const p_usage_init, WeakValueType const p_weak_init,
                       DataValueType const p_data_init) noexcept
    : m_usage_counter(p_usage_init)
    , m_counter(p_weak_init | p_data_init)
    {}

    t_counter<UsageValueType> &
    usage_counter() noexcept
    {
      return m_usage_counter;
    }

    t_counter<UsageValueType> const &
    usage_counter() const noexcept
    {
      return m_usage_counter;
    }

    template<unsigned t_bits = s_weak_bits, typename = std::enable_if_t<t_bits != 0u>>
    t_counter<WeakValueType> &
    weak_counter() noexcept
    {
      return m_counter;
    }

    template<unsigned t_bits = s_weak_bits, typename = std::enable_if_t<t_bits != 0u>>
    t_counter<WeakValueType> const &
    weak_counter() const noexcept
    {
      return m_counter;
    }

    template<unsigned t_bits = s_offset_bits + s_size_bits + s_align_bits + s_user_bits,
             typename = std::enable_if_t<t_bits != 0u>>
    t_counter<DataValueType> &
    data() noexcept
    {
      return m_counter;
    }

    template<unsigned t_bits = s_offset_bits + s_size_bits + s_align_bits + s_user_bits,
             typename = std::enable_if_t<t_bits != 0u>>
    t_counter<DataValueType> const &
    data() const noexcept
    {
      return m_counter;
    }

  private:
    t_counter<UsageValueType> m_usage_counter;
    t_counter<WeakValueType> m_counter;

    static_assert(std::is_unsigned_v<t_storage>);
    static_assert(s_weak_bits + s_offset_bits + s_size_bits + s_align_bits <= s_storage_bits / 2u);
  };


  // Specialization for separate usage and weak counters if possible.
  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_size_bits, class t_align_bits>
  class ControlDataStorage<
    t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits,
    std::enable_if_t<is_power_of_two<t_usage_bits>() && is_power_of_two<t_weak_bits>() && (t_weak_bits::value >= 8u)
                     && (t_usage_bits::value >= t_weak_bits::value)
                     && (t_usage_bits::value + t_weak_bits::value < type_bits<t_storage>())
                     && is_power_of_two(type_bits<t_storage>() - t_usage_bits::value - t_weak_bits::value)>>
  {
  public:
    using StorageType = t_storage;
    using UsageValueType = TypeFromBits<t_usage_bits>;
    using WeakValueType = TypeFromBits<t_weak_bits>;
    using DataValueType = TypeFromBits<Bits<type_bits<t_storage>() - t_usage_bits::value - t_weak_bits::value>>;

    static constexpr bool s_shared_offset = std::is_same_v<t_offset_bits, SharedBits>;
    using OffsetValueType = std::conditional_t<s_shared_offset, UsageValueType, DataValueType>;

    static constexpr unsigned s_usage_bits = t_usage_bits::value;
    static constexpr unsigned s_weak_bits = t_weak_bits::value;
    static constexpr unsigned s_offset_bits = (s_shared_offset ? 0u : t_offset_bits::value);
    static constexpr unsigned s_size_bits = t_size_bits::value;
    static constexpr unsigned s_align_bits = t_align_bits::value;
    static constexpr unsigned s_user_bits = type_bits<DataValueType>() - s_offset_bits - s_size_bits - s_align_bits;

    static constexpr unsigned s_weak_shift = 0u;
    static constexpr unsigned s_offset_shift = 0u;
    static constexpr unsigned s_size_shift = s_offset_shift + s_offset_bits;
    static constexpr unsigned s_align_shift = s_size_shift + s_size_bits;
    static constexpr unsigned s_user_shift = s_align_shift + s_align_bits;

    ControlDataStorage(UsageValueType const p_usage_init, WeakValueType const p_weak_init,
                       DataValueType const p_data_init) noexcept
    : m_usage_counter(p_usage_init)
    , m_weak_counter(p_weak_init)
    , m_data(p_data_init)
    {}

    t_counter<UsageValueType> &
    usage_counter() noexcept
    {
      return m_usage_counter;
    }

    t_counter<UsageValueType> const &
    usage_counter() const noexcept
    {
      return m_usage_counter;
    }

    t_counter<WeakValueType> &
    weak_counter() noexcept
    {
      return m_weak_counter;
    }

    t_counter<WeakValueType> const &
    weak_counter() const noexcept
    {
      return m_weak_counter;
    }

    CounterThreadUnsafe<DataValueType> &
    data() noexcept
    {
      return m_data;
    }

    CounterThreadUnsafe<DataValueType> const &
    data() const noexcept
    {
      return m_data;
    }

  private:
    t_counter<UsageValueType> m_usage_counter;
    t_counter<WeakValueType> m_weak_counter;
    CounterThreadUnsafe<DataValueType> m_data;

    static_assert(std::is_unsigned_v<t_storage>);
    static_assert(s_offset_bits + s_size_bits + s_align_bits <= type_bits<DataValueType>());
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                pntr/detail/ControlDataUsage.hpp                                //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


namespace detail
{
  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_size_bits, class t_align_bits>
  class ControlDataOffset;


  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_size_bits, class t_align_bits>
  class ControlDataUsage
  : public ControlDataStorage<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>
  {
    using Base =
      ControlDataStorage<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>;

  public:
    using UsageValueType = typename Base::UsageValueType;
    using WeakValueType = typename Base::WeakValueType;
    using DataValueType = typename Base::DataValueType;

    ControlDataUsage(WeakValueType const p_weak_init, DataValueType const p_data_init) noexcept
    : Base(s_uncontrolled, p_weak_init, p_data_init)
    {}

    // Return true if this is not controlled (yet).
    bool
    is_uncontrolled() const noexcept
    {
      return ((this->usage_counter().get_count() & s_usage_mask) == s_uncontrolled);
    }

    // Return true if this is controlled and not expired (yet).
    bool
    is_alive() const noexcept
    {
      UsageValueType const count = (this->usage_counter().get_count() & s_usage_mask);
      return (count >= s_usage_one && count <= s_usage_max);
    }

    // Return maximum usage count value.
    static constexpr UsageValueType
    get_max_usage_count() noexcept
    {
      return s_usage_max;
    }

    // Return the usage count.
    UsageValueType
    use_count() const noexcept
    {
      return (this->usage_counter().get_count() & s_usage_mask);
    }

    // Increment the usage counter.
    void
    add_ref() noexcept
    {
      [[maybe_unused]] UsageValueType const previous = (this->usage_counter().increment(s_usage_one) & s_usage_mask);
      PNTR_ASSERT(previous > s_usage_zero && previous < s_usage_max);
    }

    // Decrement the usage counter and return true if it reaches zero or was invalid.
    bool
    release() noexcept
    {
      UsageValueType const previous = (this->usage_counter().decrement(s_usage_one) & s_usage_mask);
      if (previous > s_usage_one && previous <= s_usage_max)
      {
        return false;
      }
      if (previous != s_usage_one)
      {
        PNTR_ASSERT(previous == s_uncontrolled);
        // Undo decrement on invalid counter.
        this->usage_counter().increment(s_usage_one);
      }
      return true;
    }

    // If the usage counter is
    // - uncontrolled: Initialize it with its first reference and return ControlStatus::e_acquired
    // - zero or max:  Return ControlStatus::e_invalid
    // - otherwise:    Increment it and return ControlStatus::e_shared
    ControlStatus
    try_control() noexcept
    {
      UsageValueType count = this->usage_counter().get_count();
      while ((count & s_usage_mask) == s_uncontrolled
             && !this->usage_counter().compare_exchange_weak(count, (count & ~s_usage_mask) | s_usage_one))
      {}
      if ((count & s_usage_mask) == s_uncontrolled)
      {
        return ControlStatus::e_acquired;
      }
      while ((count & s_usage_mask) > s_usage_zero && (count & s_usage_mask) < s_usage_max
             && !this->usage_counter().compare_exchange_weak(count, count + s_usage_one))
      {}
      PNTR_ASSERT((count & s_usage_mask) < s_usage_max);
      return ((count & s_usage_mask) > s_usage_zero && (count & s_usage_mask) < s_usage_max ? ControlStatus::e_shared
                                                                                            : ControlStatus::e_invalid);
    }

    // Increment the usage counter if it is not zero or max, and return true if it was incremented.
    bool
    try_add_ref() noexcept
    {
      UsageValueType count = this->usage_counter().get_count();
      while ((count & s_usage_mask) > s_usage_zero && (count & s_usage_mask) < s_usage_max
             && !this->usage_counter().compare_exchange_weak(count, count + s_usage_one))
      {}
      PNTR_ASSERT((count & s_usage_mask) < s_usage_max);
      return ((count & s_usage_mask) > s_usage_zero && (count & s_usage_mask) < s_usage_max);
    }

    // Re-initialize the usage counter of an expired object. Return true if it is or was uncontrolled.
    bool
    try_revive() noexcept
    {
      UsageValueType count = this->usage_counter().get_count();
      while ((count & s_usage_mask) == s_usage_zero
             && !this->usage_counter().compare_exchange_weak(count, (count & ~s_usage_mask) | s_uncontrolled))
      {}
      return ((count & s_usage_mask) == s_usage_zero || (count & s_usage_mask) == s_uncontrolled);
    }

  private:
    static constexpr UsageValueType s_usage_zero = static_cast<UsageValueType>(0u);
    static constexpr UsageValueType s_usage_one = static_cast<UsageValueType>(1u);
    static constexpr UsageValueType s_usage_mask =
      (std::numeric_limits<UsageValueType>::max() >> (type_bits<UsageValueType>() - Base::s_usage_bits));
    static constexpr UsageValueType s_uncontrolled = s_usage_mask;
    static constexpr UsageValueType s_usage_max =
      (Base::s_shared_offset ? (s_uncontrolled >> 1u) : s_uncontrolled - s_usage_one);

    // At least two bits are required for the usage count.
    static_assert(Base::s_usage_bits >= 2u);

    friend ControlDataOffset<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>;
  };


  template<template<typename> class t_counter, typename t_storage, class t_weak_bits, class t_offset_bits,
           class t_size_bits, class t_align_bits>
  class ControlDataUsage<t_counter, t_storage, NoBits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>
  : public ControlDataStorage<t_counter, t_storage, NoBits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>
  {
    using Base =
      ControlDataStorage<t_counter, t_storage, NoBits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>;

  public:
    using WeakValueType = typename Base::WeakValueType;
    using DataValueType = typename Base::DataValueType;

    ControlDataUsage(WeakValueType const p_weak_init, DataValueType const p_data_init) noexcept
    : Base(0u, p_weak_init, p_data_init)
    {}
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                pntr/detail/ControlDataWeak.hpp                                 //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


namespace detail
{
  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_size_bits, class t_align_bits>
  class ControlDataWeak
  : public ControlDataUsage<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>
  {
    using Base =
      ControlDataUsage<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>;

  public:
    using WeakValueType = typename Base::WeakValueType;
    using DataValueType = typename Base::DataValueType;
    using SupportsWeak = std::true_type;

    explicit ControlDataWeak(DataValueType const p_data_init) noexcept
    : Base(s_weak_one_shifted, p_data_init)
    {}

    // Return maximum weak count value.
    static constexpr WeakValueType
    get_max_weak_count() noexcept
    {
      return s_weak_max;
    }

    // Return the weak count.
    WeakValueType
    weak_count() const noexcept
    {
      return ((this->weak_counter().get_count() & s_weak_mask) >> Base::s_weak_shift);
    }

    // Increment the weak counter.
    void
    weak_add_ref() noexcept
    {
      [[maybe_unused]] WeakValueType const previous =
        ((this->weak_counter().increment(s_weak_one_shifted) & s_weak_mask) >> Base::s_weak_shift);
      PNTR_ASSERT(previous < s_weak_max);
    }

    // Decrement the weak counter and return true if it reaches or was zero.
    bool
    weak_release() noexcept
    {
      WeakValueType const previous =
        ((this->weak_counter().decrement(s_weak_one_shifted) & s_weak_mask) >> Base::s_weak_shift);
      if (previous > s_weak_one)
      {
        return false;
      }
      PNTR_ASSERT(previous != s_weak_zero);
      if (previous == s_weak_zero)
      {
        // Undo decrement on zero counter.
        this->weak_counter().increment(s_weak_one_shifted);
      }
      return true;
    }

  private:
    static constexpr WeakValueType s_weak_zero = static_cast<WeakValueType>(0u);
    static constexpr WeakValueType s_weak_one = static_cast<WeakValueType>(1u);
    static constexpr WeakValueType s_weak_one_shifted = (s_weak_one << Base::s_weak_shift);
    static constexpr WeakValueType s_weak_max =
      (std::numeric_limits<WeakValueType>::max() >> (type_bits<WeakValueType>() - Base::s_weak_bits));
    static constexpr WeakValueType s_weak_mask = (s_weak_max << Base::s_weak_shift);

    // At least two bits are required for the weak count.
    static_assert(Base::s_weak_bits >= 2u);
  };


  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_offset_bits,
           class t_size_bits, class t_align_bits>
  class ControlDataWeak<t_counter, t_storage, t_usage_bits, NoBits, t_offset_bits, t_size_bits, t_align_bits>
  : public ControlDataUsage<t_counter, t_storage, t_usage_bits, NoBits, t_offset_bits, t_size_bits, t_align_bits>
  {
    using Base = ControlDataUsage<t_counter, t_storage, t_usage_bits, NoBits, t_offset_bits, t_size_bits, t_align_bits>;

  public:
    using DataValueType = typename Base::DataValueType;
    using SupportsWeak = std::false_type;

    explicit ControlDataWeak(DataValueType const p_data_init) noexcept
    : Base(0u, p_data_init)
    {}
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                               pntr/detail/ControlDataOffset.hpp                                //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


namespace detail
{
  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_size_bits, class t_align_bits>
  class ControlDataOffset
  : public ControlDataWeak<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>
  {
    using Base =
      ControlDataWeak<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>;

  public:
    using DataValueType = typename Base::DataValueType;
    using OffsetValueType = typename Base::OffsetValueType;

    explicit ControlDataOffset(DataValueType const p_data_init) noexcept
    : Base(p_data_init)
    {}

    // Return maximum offset value.
    static constexpr std::size_t
    get_max_offset() noexcept
    {
      return s_offset_max;
    }

    // Return the offset value.
    std::size_t
    get_offset() const noexcept
    {
      return static_cast<std::size_t>((this->data().get_count() & s_offset_mask) >> Base::s_offset_shift);
    }

    // Try to set the offset value and return true on success.
    bool
    try_set_offset(std::size_t const p_offset) noexcept
    {
      if (p_offset > s_offset_max)
      {
        return false;
      }
      OffsetValueType const offset =
        static_cast<OffsetValueType>(static_cast<OffsetValueType>(p_offset) << Base::s_offset_shift);
      OffsetValueType count = this->data().get_count();
      while (!this->data().compare_exchange_weak(count, ((count & ~s_offset_mask) | offset)))
      {}
      return true;
    }

  private:
    static constexpr std::size_t s_offset_max =
      static_cast<std::size_t>(std::numeric_limits<OffsetValueType>::max()
                               >> (type_bits<OffsetValueType>() - Base::s_offset_bits));
    static constexpr OffsetValueType s_offset_mask =
      static_cast<OffsetValueType>(static_cast<OffsetValueType>(s_offset_max) << Base::s_offset_shift);
  };


  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_size_bits, class t_align_bits>
  class ControlDataOffset<t_counter, t_storage, t_usage_bits, t_weak_bits, NoBits, t_size_bits, t_align_bits>
  : public ControlDataWeak<t_counter, t_storage, t_usage_bits, t_weak_bits, NoBits, t_size_bits, t_align_bits>
  {
    using Base = ControlDataWeak<t_counter, t_storage, t_usage_bits, t_weak_bits, NoBits, t_size_bits, t_align_bits>;

  public:
    using DataValueType = typename Base::DataValueType;

    explicit ControlDataOffset(DataValueType const p_data_init) noexcept
    : Base(p_data_init)
    {}

    static constexpr std::size_t
    get_max_offset() noexcept
    {
      return s_offset_zero;
    }

    std::size_t
    get_offset() const noexcept
    {
      return s_offset_zero;
    }

    bool
    try_set_offset(std::size_t const p_offset) noexcept
    {
      return (p_offset == s_offset_zero);
    }

  private:
    static constexpr std::size_t s_offset_zero = static_cast<std::size_t>(0u);
  };


  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_size_bits, class t_align_bits>
  class ControlDataOffset<t_counter, t_storage, t_usage_bits, t_weak_bits, SharedBits, t_size_bits, t_align_bits>
  : public ControlDataWeak<t_counter, t_storage, t_usage_bits, t_weak_bits, SharedBits, t_size_bits, t_align_bits>
  {
    using Base =
      ControlDataWeak<t_counter, t_storage, t_usage_bits, t_weak_bits, SharedBits, t_size_bits, t_align_bits>;

  public:
    using UsageValueType = typename Base::UsageValueType;
    using DataValueType = typename Base::DataValueType;
    using OffsetValueType = typename Base::OffsetValueType;

    static constexpr std::size_t s_invalid_offset = std::numeric_limits<std::size_t>::max();

    explicit ControlDataOffset(DataValueType const p_data_init) noexcept
    : Base(p_data_init)
    {}

    // Return maximum offset value.
    static constexpr std::size_t
    get_max_offset() noexcept
    {
      return s_offset_max;
    }

    // Return the offset value.
    std::size_t
    get_offset() const noexcept
    {
      OffsetValueType const count = (this->usage_counter().get_count() & Base::s_usage_mask);
      return (count == Base::s_uncontrolled || count <= Base::s_usage_max
                ? s_invalid_offset
                : static_cast<std::size_t>(count & s_offset_mask));
    }

    // Try to set the offset value and return true on success.
    bool
    try_set_offset(std::size_t const p_offset) noexcept
    {
      OffsetValueType count = this->usage_counter().get_count();
      if (p_offset > s_offset_max || (count & Base::s_usage_mask) == Base::s_uncontrolled
          || ((count & Base::s_usage_mask) >= Base::s_usage_one && (count & Base::s_usage_mask) <= Base::s_usage_max))
      {
        return false;
      }
      OffsetValueType const offset = (s_offset_bit | static_cast<OffsetValueType>(p_offset));
      while (!this->usage_counter().compare_exchange_weak(count, ((count & ~s_offset_mask) | offset)))
      {}
      return true;
    }

  private:
    static constexpr OffsetValueType s_offset_bit = (Base::s_usage_one << (Base::s_usage_bits - 1u));
    static constexpr OffsetValueType s_offset_mask = s_offset_bit - Base::s_usage_one;
    static constexpr std::size_t s_offset_max = static_cast<std::size_t>(s_offset_mask - Base::s_usage_one);

    static_assert(std::is_same_v<OffsetValueType, UsageValueType>);
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                pntr/detail/ControlDataSize.hpp                                 //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


namespace detail
{
  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_size_bits, class t_align_bits>
  class ControlDataSize
  : public ControlDataOffset<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>
  {
    using Base =
      ControlDataOffset<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>;

  public:
    using DataValueType = typename Base::DataValueType;

    explicit ControlDataSize(DataValueType const p_data_init) noexcept
    : Base(p_data_init)
    {}

    // Return maximum size value.
    static constexpr std::size_t
    get_max_size() noexcept
    {
      return s_size_max;
    }

    // Return the size value.
    std::size_t
    get_size() const noexcept
    {
      return static_cast<std::size_t>((this->data().get_count() & s_size_mask) >> Base::s_size_shift);
    }

    // Try to set the size value and return true on success.
    bool
    try_set_size(std::size_t const p_size) noexcept
    {
      if (p_size > s_size_max)
      {
        return false;
      }
      DataValueType const size = static_cast<DataValueType>(static_cast<DataValueType>(p_size) << Base::s_size_shift);
      DataValueType count = this->data().get_count();
      while (!this->data().compare_exchange_weak(count, ((count & ~s_size_mask) | size)))
      {}
      return true;
    }

  private:
    static constexpr std::size_t s_size_max =
      static_cast<std::size_t>(std::numeric_limits<DataValueType>::max()
                               >> (type_bits<DataValueType>() - Base::s_size_bits));
    static constexpr DataValueType s_size_mask =
      static_cast<DataValueType>(static_cast<DataValueType>(s_size_max) << Base::s_size_shift);
  };


  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_align_bits>
  class ControlDataSize<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, NoBits, t_align_bits>
  : public ControlDataOffset<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, NoBits, t_align_bits>
  {
    using Base =
      ControlDataOffset<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, NoBits, t_align_bits>;

  public:
    using DataValueType = typename Base::DataValueType;

    explicit ControlDataSize(DataValueType const p_data_init) noexcept
    : Base(p_data_init)
    {}

    static constexpr std::size_t
    get_max_size() noexcept
    {
      return s_size_zero;
    }

    std::size_t
    get_size() const noexcept
    {
      return s_size_zero;
    }

    bool
    try_set_size(std::size_t const p_size) noexcept
    {
      return (p_size == s_size_zero);
    }

  private:
    static constexpr std::size_t s_size_zero = static_cast<std::size_t>(0u);
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                pntr/detail/ControlDataAlign.hpp                                //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


namespace detail
{
  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_size_bits, class t_align_bits>
  class ControlDataAlign
  : public ControlDataSize<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>
  {
    using Base =
      ControlDataSize<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>;

  public:
    using DataValueType = typename Base::DataValueType;

    explicit ControlDataAlign(DataValueType const p_data_init) noexcept
    : Base(p_data_init)
    {}

    // Return maximum align value.
    static constexpr std::size_t
    get_max_align() noexcept
    {
      return s_align_max;
    }

    // Return the align value.
    std::size_t
    get_align() const noexcept
    {
      return static_cast<std::size_t>((this->data().get_count() & s_align_mask) >> Base::s_align_shift);
    }

    // Try to set the align value and return true on success.
    bool
    try_set_align(std::size_t const p_align) noexcept
    {
      if (p_align > s_align_max)
      {
        return false;
      }
      DataValueType const align =
        static_cast<DataValueType>(static_cast<DataValueType>(p_align) << Base::s_align_shift);
      DataValueType count = this->data().get_count();
      while (!this->data().compare_exchange_weak(count, ((count & ~s_align_mask) | align)))
      {}
      return true;
    }

  private:
    static constexpr ::size_t s_align_max = static_cast<::size_t>(std::numeric_limits<DataValueType>::max()
                                                                  >> (type_bits<DataValueType>() - Base::s_align_bits));
    static constexpr DataValueType s_align_mask =
      static_cast<DataValueType>(static_cast<DataValueType>(s_align_max) << Base::s_align_shift);
  };


  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_size_bits>
  class ControlDataAlign<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, NoBits>
  : public ControlDataSize<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, NoBits>
  {
    using Base = ControlDataSize<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, NoBits>;

  public:
    using DataValueType = typename Base::DataValueType;

    explicit ControlDataAlign(DataValueType const p_data_init) noexcept
    : Base(p_data_init)
    {}

    static constexpr std::size_t
    get_max_align() noexcept
    {
      return s_align_zero;
    }

    std::size_t
    get_align() const noexcept
    {
      return s_align_zero;
    }

    bool
    try_set_align(std::size_t const p_align) noexcept
    {
      return (p_align == s_align_zero);
    }

  private:
    static constexpr std::size_t s_align_zero = static_cast<std::size_t>(0u);
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                pntr/detail/ControlDataUser.hpp                                 //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


namespace detail
{
  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_size_bits, class t_align_bits, typename = void>
  class ControlDataUser
  : public ControlDataAlign<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>
  {
    using Base =
      ControlDataAlign<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>;

  public:
    using DataValueType = typename Base::DataValueType;

    explicit ControlDataUser(DataValueType const p_user_init) noexcept
    : Base(static_cast<DataValueType>(p_user_init << Base::s_user_shift))
    {}

    // Return maximum user value.
    static constexpr DataValueType
    get_max_user() noexcept
    {
      return s_user_max;
    }

    // Return the user value.
    DataValueType
    get_user() const noexcept
    {
      return ((this->data().get_count() & s_user_mask) >> Base::s_user_shift);
    }

    // Try to set the user value and return true on success.
    bool
    try_set_user(DataValueType const p_user) noexcept
    {
      if (p_user > s_user_max)
      {
        return false;
      }
      DataValueType const user = static_cast<DataValueType>(p_user << Base::s_user_shift);
      DataValueType count = this->data().get_count();
      while (!this->data().compare_exchange_weak(count, ((count & ~s_user_mask) | user)))
      {}
      return true;
    }

  private:
    static constexpr DataValueType s_user_max =
      (std::numeric_limits<DataValueType>::max() >> (type_bits<DataValueType>() - Base::s_user_bits));
    static constexpr DataValueType s_user_mask = (s_user_max << Base::s_user_shift);
  };


  template<template<typename> class t_counter, typename t_storage, class t_usage_bits, class t_weak_bits,
           class t_offset_bits, class t_size_bits, class t_align_bits>
  class ControlDataUser<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits,
                        std::enable_if_t<t_usage_bits::value + t_weak_bits::value + t_size_bits::value + t_align_bits::value
                                           + (std::is_same_v<t_offset_bits, SharedBits> ? 0u : t_offset_bits::value)
                                         == type_bits<t_storage>()>>
  : public ControlDataAlign<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>
  {
    using Base =
      ControlDataAlign<t_counter, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>;

  public:
    using DataValueType = typename Base::DataValueType;

    explicit ControlDataUser(DataValueType const) noexcept
    : Base(s_user_zero)
    {}

    static constexpr DataValueType
    get_max_user() noexcept
    {
      return s_user_zero;
    }

    DataValueType
    get_user() const noexcept
    {
      return s_user_zero;
    }

    bool
    try_set_user(DataValueType const p_user) noexcept
    {
      return (p_user == s_user_zero);
    }

  private:
    static constexpr DataValueType s_user_zero = static_cast<DataValueType>(0u);
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                 pntr/detail/AllocAdaptBase.hpp                                 //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


template<class t_shared_base, class t_data, class t_allocator>
class ControlAlloc;

namespace detail
{
  template<class t_shared_base, class t_data, class t_allocator, typename = void>
  class AllocAdaptBase;

  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptData
  {
  public:
    enum class Mode
    {
      e_destroy,
      e_deallocate
    };

    using Control = ControlAlloc<t_shared_base, t_data, t_allocator>;
    using Function = void (*)(AllocAdaptBase<t_shared_base, t_data, t_allocator> &, Control &, Mode const) noexcept;

    explicit AllocAdaptData(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    t_data m_data;
  };


  template<class t_shared_base, class t_data, class t_allocator, typename = void>
  class AllocAdaptDataFunction: public AllocAdaptData<t_shared_base, t_data, t_allocator>
  {
  public:
    explicit AllocAdaptDataFunction(typename t_data::DataValueType const p_user_init) noexcept
    : AllocAdaptData<t_shared_base, t_data, t_allocator>(p_user_init)
    {}
  };

  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptDataFunction<t_shared_base, t_data, t_allocator, std::enable_if_t<t_allocator::SupportsStatic::value>>
  : public AllocAdaptData<t_shared_base, t_data, t_allocator>
  {
    using Base = AllocAdaptData<t_shared_base, t_data, t_allocator>;

  public:
    explicit AllocAdaptDataFunction(typename t_data::DataValueType const p_user_init) noexcept
    : Base(p_user_init)
    {}

    typename Base::Function m_function{};
  };


  template<class t_shared_base, class t_data, class t_allocator, typename>
  class AllocAdaptBase: public AllocAdaptDataFunction<t_shared_base, t_data, t_allocator>
  {
  public:
    using SupportsStatic = typename t_allocator::SupportsStatic;

    explicit AllocAdaptBase(typename t_data::DataValueType const p_user_init) noexcept
    : AllocAdaptDataFunction<t_shared_base, t_data, t_allocator>(p_user_init)
    {}

    t_allocator &
    allocator() noexcept
    {
      return m_allocator;
    }

  private:
    t_allocator m_allocator;
  };

  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptBase<t_shared_base, t_data, t_allocator, std::enable_if_t<is_empty_base<t_allocator>>>
  : public AllocAdaptDataFunction<t_shared_base, t_data, t_allocator>
  , private t_allocator
  {
  public:
    using SupportsStatic = typename t_allocator::SupportsStatic;

    explicit AllocAdaptBase(typename t_data::DataValueType const p_user_init) noexcept
    : AllocAdaptDataFunction<t_shared_base, t_data, t_allocator>(p_user_init)
    {}

    t_allocator &
    allocator() noexcept
    {
      return *this;
    }
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                               pntr/detail/AllocAdaptPointer.hpp                                //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


namespace detail
{
  template<class t_allocator, typename = void>
  struct HasPointerDeallocate: std::false_type
  {};

  template<class t_allocator>
  struct HasPointerDeallocate<t_allocator, std::void_t<typename t_allocator::PointerDeallocate>>: std::true_type
  {};


  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptPointer: public AllocAdaptBase<t_shared_base, t_data, t_allocator>
  {
    using Base = AllocAdaptBase<t_shared_base, t_data, t_allocator>;
    using Control = typename Base::Control;
    using Mode = typename Base::Mode;
    inline static constexpr bool s_supports_static = Base::SupportsStatic::value;

  public:
    explicit AllocAdaptPointer(typename t_data::DataValueType const p_user_init) noexcept
    : Base(p_user_init)
    {}

    template<class t_shared, class t_nothrow, class t_shared_allocator, typename... t_args>
    static t_shared *
    create(t_shared_allocator & p_allocator, t_args &&... p_args) noexcept(t_nothrow::value)
    {
      static_assert(std::is_same_v<t_shared_base, typename t_shared::PntrSharedBase>);

#ifdef _WIN32
      // If this fails, read AllocatorMalloc.hpp
      static constexpr bool is_special_aligned = (alignof(t_shared) > alignof(std::max_align_t));
      static_assert(!is_special_aligned || t_data::s_offset_bits != 0u
                    || (s_supports_static && t_data::s_shared_offset));
#endif

      t_shared * shared = nullptr;
      void * storage = nullptr;
      if constexpr (t_nothrow::value)
      {
        try
        {
          storage = p_allocator.allocate(sizeof(t_shared), alignof(t_shared));
          if (storage != nullptr)
          {
            shared = new (storage) t_shared(std::forward<t_args>(p_args)...);
          }
        }
        catch (...)
        {}
      }
      else
      {
        storage = p_allocator.allocate(sizeof(t_shared), alignof(t_shared));
        if (storage != nullptr)
        {
          try
          {
            shared = new (storage) t_shared(std::forward<t_args>(p_args)...);
          }
          catch (...)
          {
#ifdef _WIN32
            p_allocator.deallocate(storage, is_special_aligned);
#else
            p_allocator.deallocate(storage);
#endif
            throw;
          }
        }
        else
        {
          throw std::bad_alloc();
        }
      }
      if (storage != nullptr && shared == nullptr)
      {
#ifdef _WIN32
        p_allocator.deallocate(storage, is_special_aligned);
#else
        p_allocator.deallocate(storage);
#endif
      }
      return shared;
    }

    template<class t_shared, class t_forward>
    static t_shared *
    try_init(AllocAdaptPointer & p_self, t_shared * const p_shared, t_forward && p_allocator) noexcept
    {
#ifdef _WIN32
      static constexpr bool is_special_aligned = (alignof(t_shared) > alignof(std::max_align_t));
#endif
      std::size_t offset = 0u;
      if constexpr (s_supports_static)
      {
        p_self.m_function = destroy_or_deallocate<t_shared>;
        if constexpr (!is_static_castable<t_shared>)
        {
          offset = calc_base_offset(p_shared);
        }
      }
      else
      {
        offset = calc_base_offset(p_shared);
      }
      if (offset != std::numeric_limits<std::size_t>::max())
      {
#ifdef _WIN32
        if constexpr (!s_supports_static)
        {
          offset <<= 1u;
          if constexpr (is_special_aligned)
          {
            offset |= 1u;
          }
        }
#endif
        if (offset <= t_data::get_max_offset())
        {
          if constexpr (s_supports_static)
          {
            p_self.allocator() = std::forward<t_forward>(p_allocator);
            return p_shared;
          }
          else if (p_self.m_data.try_set_offset(offset))
          {
            p_self.allocator() = std::forward<t_forward>(p_allocator);
            return p_shared;
          }
          else
          {
            PNTR_LOG_ERROR("Unable to use shared pointer offset without static support");
          }
        }
#ifndef PNTR_UNITTESTS
        else
        {
          PNTR_LOG_ERROR("Pointer offset is too big");
        }
#endif
      }
      p_shared->~t_shared();
#ifdef _WIN32
      p_allocator.deallocate(p_shared, is_special_aligned);
#else
      p_allocator.deallocate(p_shared);
#endif
      return nullptr;
    }

    template<class t_other>
    void
    destroy(Control & p_control, t_other & p_other) noexcept
    {
      if constexpr (s_supports_static)
      {
        PNTR_TRY_LOG_ERROR(this->m_function == nullptr, "Invalid function pointer");
        if (this->m_function != nullptr)
        {
          this->m_function(*this, p_control, Mode::e_destroy);
        }
      }
      else
      {
        p_other.~t_other();
      }
    }

    static void
    deallocate(AllocAdaptPointer & p_self, Control & p_control) noexcept
    {
      if constexpr (s_supports_static)
      {
        PNTR_TRY_LOG_ERROR(p_self.m_function == nullptr, "Invalid function pointer");
        if (p_self.m_function != nullptr)
        {
          p_self.m_function(p_self, p_control, Mode::e_deallocate);
        }
      }
      else
      {
        t_shared_base * const shared_base = t_shared_base::get_shared_base(p_control);
        std::size_t offset = p_self.m_data.get_offset();
#ifdef _WIN32
        bool const is_special_aligned = (offset & 1u);
        offset >>= 1u;
#endif
        void * const storage = calc_shared_pointer(shared_base, offset);
        t_allocator allocator = std::move(p_self.allocator());
        p_control.~Control();
#ifdef _WIN32
        allocator.deallocate(storage, is_special_aligned);
#else
        allocator.deallocate(storage);
#endif
      }
    }

  private:
    template<class t_shared>
    static void
    destroy_or_deallocate(AllocAdaptBase<t_shared_base, t_data, t_allocator> & p_self, Control & p_control,
                          Mode const p_mode) noexcept
    {
      t_shared_base * const shared_base = t_shared_base::get_shared_base(p_control);
      if constexpr (is_static_castable<t_shared>)
      {
        t_shared * const shared = static_cast<t_shared *>(shared_base);
        switch (p_mode)
        {
          case Mode::e_destroy:
          {
            shared->~t_shared();
            break;
          }
          case Mode::e_deallocate:
          {
            t_allocator allocator = std::move(p_self.allocator());
            p_control.~Control();
#ifdef _WIN32
            static constexpr bool is_special_aligned = (alignof(t_shared) > alignof(std::max_align_t));
            allocator.deallocate(shared, is_special_aligned);
#else
            allocator.deallocate(shared);
#endif
            break;
          }
        }
      }
      else
      {
        switch (p_mode)
        {
          case Mode::e_destroy:
          {
            t_shared * const shared = dynamic_cast<t_shared *>(shared_base);
            p_self.m_data.try_set_offset(calc_base_offset(shared));
            shared->~t_shared();
            break;
          }
          case Mode::e_deallocate:
          {
            void * const storage = calc_shared_pointer(shared_base, p_self.m_data.get_offset());
            t_allocator allocator = std::move(p_self.allocator());
            p_control.~Control();
#ifdef _WIN32
            static constexpr bool is_special_aligned = (alignof(t_shared) > alignof(std::max_align_t));
            allocator.deallocate(storage, is_special_aligned);
#else
            allocator.deallocate(storage);
#endif
            break;
          }
        }
      }
    }
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                               pntr/detail/AllocAdaptTypeInfo.hpp                               //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


namespace detail
{
  template<class t_allocator, typename = void>
  struct HasTypeInfoDeallocate: std::false_type
  {};

  template<class t_allocator>
  struct HasTypeInfoDeallocate<t_allocator, std::void_t<typename t_allocator::TypeInfoDeallocate>>: std::true_type
  {};


  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptTypeInfo: public AllocAdaptBase<t_shared_base, t_data, t_allocator>
  {
    using Base = AllocAdaptBase<t_shared_base, t_data, t_allocator>;
    using Control = typename Base::Control;
    using Mode = typename Base::Mode;
    inline static constexpr bool s_supports_static = Base::SupportsStatic::value;

  public:
    explicit AllocAdaptTypeInfo(typename t_data::DataValueType const p_user_init) noexcept
    : Base(p_user_init)
    {}

    template<class t_shared, class t_nothrow, class t_shared_allocator, typename... t_args>
    static t_shared *
    create(t_shared_allocator & p_allocator, t_args &&... p_args) noexcept(t_nothrow::value)
    {
      static_assert(std::is_same_v<t_shared_base, typename t_shared::PntrSharedBase>);

      static constexpr bool is_special_aligned = (alignof(t_shared) > alignof(std::max_align_t));
      static constexpr bool supports_align =
        (s_supports_static
         || (t_data::get_max_align() != 0u && calc_base_align_offset<t_shared>() <= t_data::get_max_align()));
      static constexpr std::size_t align = (supports_align ? alignof(t_shared) : alignof(std::max_align_t));

      static_assert(s_supports_static || calc_base_size_offset<t_shared>() <= t_data::get_max_size());
      static_assert(supports_align || (t_data::get_max_align() == 0u && !is_special_aligned));

      t_shared * shared = nullptr;
      void * storage = nullptr;
      if constexpr (t_nothrow::value)
      {
        try
        {
          storage = p_allocator.allocate(sizeof(t_shared), align);
          if (storage != nullptr)
          {
            shared = new (storage) t_shared(std::forward<t_args>(p_args)...);
          }
        }
        catch (...)
        {}
      }
      else
      {
        storage = p_allocator.allocate(sizeof(t_shared), align);
        if (storage != nullptr)
        {
          try
          {
            shared = new (storage) t_shared(std::forward<t_args>(p_args)...);
          }
          catch (...)
          {
            p_allocator.deallocate(storage, sizeof(t_shared), align);
            throw;
          }
        }
        else
        {
          throw std::bad_alloc();
        }
      }
      if (storage != nullptr && shared == nullptr)
      {
        p_allocator.deallocate(storage, sizeof(t_shared), align);
      }
      return shared;
    }

    template<class t_shared, class t_forward>
    static t_shared *
    try_init(AllocAdaptTypeInfo & p_self, t_shared * const p_shared, t_forward && p_allocator) noexcept
    {
      static constexpr bool supports_align =
        (s_supports_static
         || (t_data::get_max_align() != 0u && calc_base_align_offset<t_shared>() <= t_data::get_max_align()));
      static constexpr std::size_t align = (supports_align ? alignof(t_shared) : alignof(std::max_align_t));

      std::size_t offset = 0u;
      if constexpr (s_supports_static)
      {
        p_self.m_function = destroy_or_deallocate<t_shared>;
        if constexpr (!is_static_castable<t_shared>)
        {
          offset = calc_base_offset(p_shared);
        }
      }
      else
      {
        offset = calc_base_offset(p_shared);
      }
      if (offset != std::numeric_limits<std::size_t>::max())
      {
        if (offset <= t_data::get_max_offset())
        {
          if constexpr (s_supports_static)
          {
            p_self.allocator() = std::forward<t_forward>(p_allocator);
            return p_shared;
          }
          else if (p_self.m_data.try_set_offset(offset))
          {
            p_self.m_data.try_set_size(calc_base_size_offset<t_shared>());
            p_self.m_data.try_set_align(calc_base_align_offset<t_shared>());
            p_self.allocator() = std::forward<t_forward>(p_allocator);
            return p_shared;
          }
          else
          {
            PNTR_LOG_ERROR("Unable to use shared pointer offset without static support");
          }
        }
#ifndef PNTR_UNITTESTS
        else
        {
          PNTR_LOG_ERROR("Pointer offset is too big");
        }
#endif
      }
      p_shared->~t_shared();
      p_allocator.deallocate(p_shared, sizeof(t_shared), align);
      return nullptr;
    }

    template<class t_other>
    void
    destroy(Control & p_control, t_other & p_other) noexcept
    {
      if constexpr (s_supports_static)
      {
        PNTR_TRY_LOG_ERROR(this->m_function == nullptr, "Invalid function pointer");
        if (this->m_function != nullptr)
        {
          this->m_function(*this, p_control, Mode::e_destroy);
        }
      }
      else
      {
        p_other.~t_other();
      }
    }

    static void
    deallocate(AllocAdaptTypeInfo & p_self, Control & p_control) noexcept
    {
      if constexpr (s_supports_static)
      {
        PNTR_TRY_LOG_ERROR(p_self.m_function == nullptr, "Invalid function pointer");
        if (p_self.m_function != nullptr)
        {
          p_self.m_function(p_self, p_control, Mode::e_deallocate);
        }
      }
      else
      {
        t_shared_base * const shared_base = t_shared_base::get_shared_base(p_control);
        std::size_t offset = p_self.m_data.get_offset();
        void * const storage = calc_shared_pointer(shared_base, offset);
        std::size_t const size = calc_shared_size<t_shared_base>(p_self.m_data.get_size());
        std::size_t const align =
          (t_data::get_max_align() == 0u ? alignof(std::max_align_t)
                                         : calc_shared_align<t_shared_base>(p_self.m_data.get_align()));
        t_allocator allocator = std::move(p_self.allocator());
        p_control.~Control();
        allocator.deallocate(storage, size, align);
      }
    }

  private:
    template<class t_shared>
    static void
    destroy_or_deallocate(AllocAdaptBase<t_shared_base, t_data, t_allocator> & p_self, Control & p_control,
                          Mode const p_mode) noexcept
    {
      t_shared_base * const shared_base = t_shared_base::get_shared_base(p_control);
      if constexpr (is_static_castable<t_shared>)
      {
        t_shared * const shared = static_cast<t_shared *>(shared_base);
        switch (p_mode)
        {
          case Mode::e_destroy:
          {
            shared->~t_shared();
            break;
          }
          case Mode::e_deallocate:
          {
            t_allocator allocator = std::move(p_self.allocator());
            p_control.~Control();
            allocator.deallocate(shared, sizeof(t_shared), alignof(t_shared));
            break;
          }
        }
      }
      else
      {
        switch (p_mode)
        {
          case Mode::e_destroy:
          {
            t_shared * const shared = dynamic_cast<t_shared *>(shared_base);
            p_self.m_data.try_set_offset(calc_base_offset(shared));
            shared->~t_shared();
            break;
          }
          case Mode::e_deallocate:
          {
            void * const storage = calc_shared_pointer(shared_base, p_self.m_data.get_offset());
            t_allocator allocator = std::move(p_self.allocator());
            p_control.~Control();
            allocator.deallocate(storage, sizeof(t_shared), alignof(t_shared));
            break;
          }
        }
      }
    }
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                pntr/detail/AllocAdaptTyped.hpp                                 //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


template<class t_shared_base, class t_data, class t_allocator>
class ControlAlloc;

namespace detail
{
  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptTyped;

  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptTypedData
  {
  public:
    explicit AllocAdaptTypedData(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    enum class Mode
    {
      e_destroy,
      e_deallocate
    };

    using Control = ControlAlloc<t_shared_base, t_data, t_allocator>;
    using Function = void (*)(AllocAdaptTyped<t_shared_base, t_data, t_allocator> &, Control &, Mode const) noexcept;

    t_data m_data;
    Function m_function{};
  };

  template<class t_shared_base, class t_data, class t_allocator, typename = void>
  class AllocAdaptTypedBase: public AllocAdaptTypedData<t_shared_base, t_data, t_allocator>
  {
  public:
    explicit AllocAdaptTypedBase(typename t_data::DataValueType const p_user_init) noexcept
    : AllocAdaptTypedData<t_shared_base, t_data, t_allocator>(p_user_init)
    {}

    template<class t_forward>
    void
    init_allocator(t_forward && p_allocator) noexcept
    {
      new (&m_allocator_storage) t_allocator(std::forward<t_forward>(p_allocator));
    }

    t_allocator &
    allocator() noexcept
    {
      return *std::launder(reinterpret_cast<t_allocator *>(&m_allocator_storage));
    }

  private:
    alignas(t_allocator) char m_allocator_storage[sizeof(t_allocator)];
  };

  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptTypedBase<t_shared_base, t_data, t_allocator, std::enable_if_t<is_empty_base<t_allocator>>>
  : public AllocAdaptTypedData<t_shared_base, t_data, t_allocator>
  , private t_allocator
  {
  public:
    explicit AllocAdaptTypedBase(typename t_data::DataValueType const p_user_init) noexcept
    : AllocAdaptTypedData<t_shared_base, t_data, t_allocator>(p_user_init)
    {}

    template<class t_forward>
    void
    init_allocator(t_forward &&) noexcept
    {}

    t_allocator &
    allocator() noexcept
    {
      return *this;
    }
  };


  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptTyped: public AllocAdaptTypedBase<t_shared_base, t_data, t_allocator>
  {
    using Base = AllocAdaptTypedBase<t_shared_base, t_data, t_allocator>;
    using Control = typename Base::Control;
    using Mode = typename Base::Mode;

  public:
    explicit AllocAdaptTyped(typename t_data::DataValueType const p_user_init) noexcept
    : Base(p_user_init)
    {}

    template<class t_shared, class t_nothrow, class t_shared_allocator, typename... t_args>
    static t_shared *
    create(t_shared_allocator & p_allocator, t_args &&... p_args) noexcept(t_nothrow::value)
    {
      static_assert(std::is_same_v<t_shared_base, typename t_shared::PntrSharedBase>);
      static_assert(std::is_same_v<t_shared, typename t_shared_allocator::value_type>);

      using AllocTraits = std::allocator_traits<t_shared_allocator>;
      t_shared * shared = nullptr;
      try
      {
        shared = AllocTraits::allocate(p_allocator, 1u);
      }
      catch (...)
      {
        if constexpr (!t_nothrow::value)
        {
          throw;
        }
      }
      if (shared != nullptr)
      {
        try
        {
          AllocTraits::construct(p_allocator, shared, std::forward<t_args>(p_args)...);
        }
        catch (...)
        {
          AllocTraits::deallocate(p_allocator, shared, 1u);
          if constexpr (t_nothrow::value)
          {
            shared = nullptr;
          }
          else
          {
            throw;
          }
        }
      }
      return shared;
    }

    template<class t_shared, class t_forward>
    static t_shared *
    try_init(AllocAdaptTyped & p_self, t_shared * p_shared, t_forward && p_allocator) noexcept
    {
      p_self.m_function = destroy_or_deallocate<t_shared>;
      if constexpr (is_static_castable<t_shared>)
      {
        p_self.init_allocator(std::forward<t_forward>(p_allocator));
      }
      else if (calc_base_offset(p_shared) <= t_data::get_max_offset())
      {
        p_self.init_allocator(std::forward<t_forward>(p_allocator));
      }
      else
      {
#ifndef PNTR_UNITTESTS
        PNTR_LOG_ERROR("Pointer offset is too big");
#endif
        using SharedAlloc = typename std::allocator_traits<t_allocator>::template rebind_alloc<t_shared>;
        using AllocTraits = std::allocator_traits<SharedAlloc>;
        AllocTraits::destroy(p_allocator, p_shared);
        AllocTraits::deallocate(p_allocator, p_shared, 1u);
        p_shared = nullptr;
      }
      return p_shared;
    }

    void
    destroy(Control & p_control, t_shared_base &) noexcept
    {
      PNTR_TRY_LOG_ERROR(this->m_function == nullptr, "Invalid function pointer");
      if (this->m_function != nullptr)
      {
        this->m_function(*this, p_control, Mode::e_destroy);
      }
    }

    static void
    deallocate(AllocAdaptTyped & p_self, Control & p_control) noexcept
    {
      PNTR_TRY_LOG_ERROR(p_self.m_function == nullptr, "Invalid function pointer");
      if (p_self.m_function != nullptr)
      {
        p_self.m_function(p_self, p_control, Mode::e_deallocate);
      }
    }

  private:
    template<class t_shared>
    static void
    destroy_or_deallocate(AllocAdaptTyped & p_self, Control & p_control, Mode const p_mode) noexcept
    {
      using SharedAlloc = typename std::allocator_traits<t_allocator>::template rebind_alloc<t_shared>;
      using AllocTraits = std::allocator_traits<SharedAlloc>;

      SharedAlloc alloc(p_self.allocator());
      t_shared_base * const shared_base = t_shared_base::get_shared_base(p_control);

      if constexpr (is_static_castable<t_shared>)
      {
        t_shared * const shared = static_cast<t_shared *>(shared_base);
        switch (p_mode)
        {
          case Mode::e_destroy:
          {
            AllocTraits::destroy(alloc, shared);
            break;
          }
          case Mode::e_deallocate:
          {
            p_control.~Control();
            AllocTraits::deallocate(alloc, shared, 1u);
            break;
          }
        }
      }
      else
      {
        switch (p_mode)
        {
          case Mode::e_destroy:
          {
            t_shared * const shared = dynamic_cast<t_shared *>(shared_base);
            p_self.m_data.try_set_offset(calc_base_offset(shared));
            AllocTraits::destroy(alloc, shared);
            break;
          }
          case Mode::e_deallocate:
          {
            t_shared * const shared =
              static_cast<t_shared *>(calc_shared_pointer(shared_base, p_self.m_data.get_offset()));
            p_control.~Control();
            AllocTraits::deallocate(alloc, shared, 1u);
            break;
          }
        }
      }
    }

    static_assert(!std::is_const_v<typename t_allocator::value_type>);
    static_assert(!std::is_volatile_v<typename t_allocator::value_type>);
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                 pntr/detail/ControlDeleter.hpp                                 //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


namespace detail
{
  // ControlDeleter stores control data and a deleter with Empty Base Optimization.
  template<class t_shared_base, class t_data, typename t_deleter, bool t_is_empty>
  class ControlDeleter;

  // ControlDeleter with templated empty-class deleter
  template<class t_shared_base, class t_data, template<class> class t_deleter>
  class ControlDeleter<t_shared_base, t_data, t_deleter<t_shared_base>, true>: public t_deleter<t_shared_base>
  {
  public:
    explicit ControlDeleter(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    t_deleter<t_shared_base> &
    deleter() noexcept
    {
      return *this;
    }

    template<class t_shared>
    void
    init() noexcept
    {}

    template<class t_shared>
    static void
    destroy(ControlDeleter &, t_shared * p_shared) noexcept
    {
      t_deleter<t_shared>()(p_shared);
    }

    t_data m_data;
  };

  // ControlDeleter with empty-class deleter
  template<class t_shared_base, class t_data, typename t_deleter>
  class ControlDeleter<t_shared_base, t_data, t_deleter, true>: public t_deleter
  {
  public:
    explicit ControlDeleter(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    t_deleter &
    deleter() noexcept
    {
      return *this;
    }

    template<class t_shared>
    void
    init() noexcept
    {}

    template<class t_shared>
    static void
    destroy(ControlDeleter &, t_shared * p_shared) noexcept
    {
      t_deleter()(p_shared);
    }

    t_data m_data;
  };

  // ControlDeleter with templated non-empty-class deleter
  template<class t_shared_base, class t_data, template<class> class t_deleter>
  class ControlDeleter<t_shared_base, t_data, t_deleter<t_shared_base>, false>
  {
  public:
    explicit ControlDeleter(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    t_deleter<t_shared_base> &
    deleter() noexcept
    {
      return m_deleter;
    }

    template<class t_shared>
    void
    init() noexcept
    {
      m_deleter = t_deleter<t_shared>();
    }

    template<class t_shared>
    static void
    destroy(ControlDeleter & p_self, t_shared * p_shared) noexcept
    {
      t_deleter<t_shared_base> const deleter(std::move(p_self.m_deleter));
      deleter(p_shared);
    }

    t_data m_data;
    t_deleter<t_shared_base> m_deleter;
  };

  // ControlDeleter with non-empty-class deleter
  template<class t_shared_base, class t_data, typename t_deleter>
  class ControlDeleter<t_shared_base, t_data, t_deleter, false>
  {
  public:
    explicit ControlDeleter(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    t_deleter &
    deleter() noexcept
    {
      return m_deleter;
    }

    template<class t_shared>
    void
    init() noexcept
    {}

    template<class t_shared>
    static void
    destroy(ControlDeleter & p_self, t_shared * p_shared) noexcept
    {
      t_deleter const deleter(std::move(p_self.m_deleter));
      deleter(p_shared);
    }

    t_data m_data;
    t_deleter m_deleter;
  };
} // namespace detail


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                    pntr/AllocatorMalloc.hpp                                    //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                        AllocatorMalloc                                         //
//                                                                                                //
//                    An allocator for 'ControlAlloc' which uses 'std::malloc'                    //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  'AllocatorMalloc' uses 'std::malloc' and related functions to allocate memory. 'ControlAlloc'
//  can save a pointer offset between a created object and its shared base, which makes it possible
//  to restore and deallocate the original pointer without storing the object's type information.
//
//  The number of bits used for the stored pointer offset is configurable in the control data block
//  and can usually be quite small to minimize the intrusive control block size. It can even be set
//  to zero for classes where the shared base class is at the front of the memory layout of the
//  created class.
//
//  The template parameter can be configured with 'StaticSupport' to enable the proper destruction
//  of non-polymorphic classes. In this case the control block will store an additonal pointer to
//  save type information.
//
//  The Microsoft Standard Library does not support 'std::aligned_alloc', see
//  https://en.cppreference.com/w/cpp/memory/c/aligned_alloc
//  Instead it provides '_aligned_malloc', which is not compatible with 'std::free'. It requires
//  to use '_aligned_free', but 'AllocatorMalloc::deallocate' doesn't have alignment information
//  of the created type. It would be possible to always use '_alligned_malloc' and '_aligned_free',
//  but this would probably cause a performance penalty. So, it is necessary to use a bit in the
//  pointer offset to store the information which function has to be used to free the memory block.
//  A static assertion ensures that this bit is available for types with special alignment.
//

template<class t_static_support = NoStaticSupport>
class AllocatorMalloc
{
public:
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //      The type definitions and member functions required by 'ControlAlloc' start here       //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // 'SupportsStatic' has to be either 'StaticSupport' or 'NoStaticSupport'. The former will
  // request 'ControlAlloc' to store type information in an additonal pointer in the control
  // block that supports the correct object destruction and deallocation from shared pointers
  // to non-polymorphic base classes.
  using SupportsStatic = t_static_support;

  // 'ControlAlloc' identifies the type of this allocator with this type definition.
  using PointerDeallocate = void;

  // Allocate a memory block.
  void *
  allocate(std::size_t p_size, std::size_t p_alignment) noexcept
  {
    void * const storage = (p_alignment <= alignof(std::max_align_t) ? std::malloc(p_size)
#ifdef _WIN32
                                                                     : _aligned_malloc(p_size, p_alignment));
#else
                                                                     : std::aligned_alloc(p_alignment, p_size));
#endif

#ifdef PNTR_UNITTESTS
    s_storage = storage;
#endif

    return storage;
  }

#ifdef _WIN32

  // Deallocate a memory block.
  void
  deallocate(void * p_pointer, bool p_is_special_aligned) noexcept
  {
  #ifdef PNTR_UNITTESTS
    if (s_storage == p_pointer)
    {
      s_storage = nullptr;
    }
  #endif

    if (p_is_special_aligned)
    {
      _aligned_free(p_pointer);
    }
    else
    {
      std::free(p_pointer);
    }
  }

#else

  // Deallocate a memory block.
  void
  deallocate(void * p_pointer) noexcept
  {
  #ifdef PNTR_UNITTESTS
    if (s_storage == p_pointer)
    {
      s_storage = nullptr;
    }
  #endif

    std::free(p_pointer);
  }

#endif

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //       The type definitions and member functions required by 'ControlAlloc' end here        //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PNTR_UNITTESTS
  AllocatorMalloc() noexcept
  {
    ++g_allocator_construct;
  }

  AllocatorMalloc(AllocatorMalloc const &) noexcept
  {
    ++g_allocator_construct;
  }

  AllocatorMalloc(AllocatorMalloc &&) noexcept
  {
    ++g_allocator_construct;
  }

  ~AllocatorMalloc() noexcept
  {
    ++g_allocator_destroy;
  }

  AllocatorMalloc & operator=(AllocatorMalloc &&) = default;
  AllocatorMalloc & operator=(AllocatorMalloc const &) = default;

  inline static void * s_storage = nullptr;
#endif
};


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                pntr/AllocatorMemoryResource.hpp                                //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <version>
#ifdef __cpp_lib_memory_resource
  #include <memory_resource>

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                    AllocatorMemoryResource                                     //
//                                                                                                //
//             An allocator for 'ControlAlloc' which uses 'std::pmr::memory_resource'             //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  'AllocatorMemoryResource' uses 'std::pmr::memory_resource' to allocate memory. 'ControlAlloc'
//  can save a pointer offset between a created object and its shared base, and also the type size
//  and alignment required for deallocation, which makes it possible to restore and deallocate the
//  original pointer and type details without storing the object's actual type.
//
//  The number of bits used for the stored pointer offset is configurable in the control data block
//  and can usually be quite small to minimize the intrusive control block size. It can even be set
//  to zero for classes where the shared base class is at the front of the memory layout of the
//  created class.
//
//  To size is restored using a factor of the control block alignment for the difference between
//  shared base class and shared class. The number of bits for this factor can be zero if no
//  derived class instances are used. The number of bits for the alignment can be set to zero for
//  types without special alignment. A static assertion ensures that enough bits are configured.
//
//  The template parameter can be configured with 'StaticSupport' to enable the proper destruction
//  of non-polymorphic classes. In this case the control block will store an additonal pointer to
//  save type information, which can be used to restore size and alignment details, so no bits for
//  type size and alignment are required.
//

template<class t_static_support = NoStaticSupport>
class AllocatorMemoryResource
{
public:
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //      The type definitions and member functions required by 'ControlAlloc' start here       //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // 'SupportsStatic' has to be either 'StaticSupport' or 'NoStaticSupport'. The former will
  // request 'ControlAlloc' to store type information in an additonal pointer in the control
  // block that supports the correct object destruction and deallocation from shared pointers
  // to non-polymorphic base classes.
  using SupportsStatic = t_static_support;

  // 'ControlAlloc' identifies the type of this allocator with this type definition.
  using TypeInfoDeallocate = void;

  // Allocate a memory block.
  void *
  allocate(std::size_t p_size, std::size_t p_alignment) noexcept
  {
    void * const storage = m_resource->allocate(p_size, p_alignment);
  #ifdef PNTR_UNITTESTS
    s_storage = storage;
    s_size = p_size;
    s_align = p_alignment;
  #endif
    return storage;
  }

  // Deallocate a memory block.
  void
  deallocate(void * p_pointer, std::size_t p_size, std::size_t p_alignment) noexcept
  {
  #ifdef PNTR_UNITTESTS
    if (s_storage == p_pointer && s_size == p_size && s_align == p_alignment)
    {
      s_storage = nullptr;
    }
  #endif
    m_resource->deallocate(p_pointer, p_size, p_alignment);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //       The type definitions and member functions required by 'ControlAlloc' end here        //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  AllocatorMemoryResource(std::pmr::memory_resource * p_resource) noexcept
  : m_resource(p_resource)
  {
    PNTR_ASSERT(p_resource != nullptr);
  }

  std::pmr::memory_resource *
  resource() const noexcept
  {
    return m_resource;
  }

  #ifdef PNTR_UNITTESTS
  AllocatorMemoryResource() noexcept
  {
    ++g_allocator_construct;
  }

  AllocatorMemoryResource(AllocatorMemoryResource const & p_allocator) noexcept
  : m_resource(p_allocator.m_resource)
  {
    ++g_allocator_construct;
  }

  AllocatorMemoryResource(AllocatorMemoryResource && p_allocator) noexcept
  : m_resource(p_allocator.m_resource)
  {
    ++g_allocator_construct;
  }

  ~AllocatorMemoryResource() noexcept
  {
    ++g_allocator_destroy;
  }

  inline static void * s_storage = nullptr;
  inline static std::size_t s_size = 0u;
  inline static std::size_t s_align = 0u;

  #else

  AllocatorMemoryResource() noexcept = default;
  AllocatorMemoryResource(AllocatorMemoryResource const &) noexcept = default;
  AllocatorMemoryResource(AllocatorMemoryResource &&) noexcept = default;

  #endif

  AllocatorMemoryResource & operator=(AllocatorMemoryResource &&) = default;
  AllocatorMemoryResource & operator=(AllocatorMemoryResource const &) = default;

private:
  std::pmr::memory_resource * m_resource = std::pmr::get_default_resource();
};


PNTR_NAMESPACE_END

#endif // __cpp_lib_memory_resource

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                        pntr/Deleter.hpp                                        //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                            Deleter                                             //
//                                                                                                //
//                 A deleter for 'ControlNew' which saves the shared object type                  //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  During construction this deleter saves a pointer to a function that casts the shared object
//  pointer to the original type before it is deleted. This makes it possible to use 'SharedPtr'
//  with base classes of non-polymorphic class hierarchies.
//
//  It requires the size of a pointer in the control block, so it is more efficient not to use it
//  with polymorphic types where it is safe to call the virtual destructor of the base class.
//

template<typename t_shared>
struct Deleter
{
  using SharedBase = std::remove_const_t<detail::BaseType<t_shared>>;

  Deleter & operator=(Deleter &&) = default;
  Deleter & operator=(Deleter const &) = default;

  template<typename t_other,
           typename = std::enable_if_t<std::is_convertible_v<std::remove_const_t<t_other> *, t_shared *>>>
  Deleter(Deleter<t_other> const & p_other) noexcept
  : m_delete_function(p_other.m_delete_function)
#ifndef PNTR_UNITTESTS
  {}

  constexpr Deleter() noexcept = default;
  Deleter(Deleter const &) noexcept = default;
  Deleter(Deleter &&) noexcept = default;

#else
  {
    ++g_deleter_construct;
  }

  Deleter() noexcept
  {
    ++g_deleter_construct;
  }

  Deleter(Deleter const & p_deleter) noexcept
  : m_delete_function(p_deleter.m_delete_function)
  {
    ++g_deleter_construct;
  }

  Deleter(Deleter && p_deleter) noexcept
  : m_delete_function(p_deleter.m_delete_function)
  {
    ++g_deleter_construct;
  }

  ~Deleter() noexcept
  {
    ++g_deleter_destroy;
  }

#endif

  void
  operator()(t_shared * p_shared) const noexcept
  {
    static_assert(sizeof(t_shared) != 0u, "incomplete type");
    m_delete_function(p_shared);
  }

private:
  static void
  delete_shared(SharedBase const * p_base) noexcept
  {
    delete detail::static_or_dynamic_cast<std::add_const_t<t_shared>>(p_base);
  }

  using DeleteFunction = void (*)(SharedBase const *) noexcept;
  DeleteFunction m_delete_function = &delete_shared;

  template<typename t_other>
  friend struct Deleter;
};


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                      pntr/ControlData.hpp                                      //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                          ControlData                                           //
//                                                                                                //
//                             The data storage for the control block                             //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  'ControlData' stores unsigned integer data with a combined size of the 't_storage' type, which
//  is typically one of 'std::uint8_t', 'std::uint16_t', 'std::uint32_t', or 'std::uint64_t'.
//  't_counter' is a counter template class like 'CounterThreadSafe' or 'CounterThreadUnsafe'.
//
//  The remaining template parameters specify how many bits should be reserved for which purpose:
//  - 't_usage_bits':  The usage reference counter.
//  - 't_weak_bits':   The weak reference counter.
//                     This should be zero for control blocks that don't support weak pointers.
//  - 't_offset_bits': An offset value to calculate the original pointer for deallocation.
//                     This can be 'shared_bits' to store the offset in the usage bits after
//                     the object has been destroyed. This is only possible if the allocator
//                     requests 'StaticSupport' to store type information in an extra pointer.
//  - 't_size_bits':   An offset value to calculate the original type size for deallocation.
//                     Size and align bits are only required for allocators compatible to
//                     'AllocatorMemoryResource' with 'NoStaticSupport' configuration.
//  - 't_align_bits':  An offset value to calculate the original type alignment for deallocation.
//                     This can be zero even for an allocator that requires the alignment,
//                     if the object types don't exceed standard alignment ('std::max_align_t').
//  - ('t_user_bits'): The bits used for a custom user value accessible through the 'Intruder'.
//                     This number is automatically calculated by the unused bits of 't_storage',
//                     so no bits get wasted to alignment padding.
//
//  Specializations of the data storage will use separate usage and weak counters for improved
//  performance if the bit values allow it. For example it will use 'std::uint32_t' for the usage
//  counter if the storage type is 'std::uint64_t' with 32 bits reserved for the usage counter.
//

// clang-format off

// A bug in 'Microsoft Visual C++' compiler requires to use type parameters for bit count values
// instead of a value parameters. 'detail::Bits' converts the values to a type.

inline constexpr unsigned shared_bits = std::numeric_limits<unsigned>::max();

template<template<typename> class t_counter,
         typename t_storage,
         unsigned t_usage_bits = detail::type_bits<t_storage>(),
         unsigned t_weak_bits = 0u,
         unsigned t_offset_bits = 0u,
         unsigned t_size_bits = 0u,
         unsigned t_align_bits = 0u>
using ControlData =
  detail::ControlDataUser<t_counter, t_storage, detail::Bits<t_usage_bits>, detail::Bits<t_weak_bits>,
                          detail::Bits<t_offset_bits>, detail::Bits<t_size_bits>, detail::Bits<t_align_bits>>;

// clang-format on

PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                     pntr/ControlAlloc.hpp                                      //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                          ControlAlloc                                          //
//                                                                                                //
//          A control class for 'Intruder', using an allocator, and usage and weak count          //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  'ControlAlloc' supports three types of allocators:
//  - Allocators which only require the original untyped pointer for deallocation. The most common
//    example is 'std::free'. Those allocators are required to provide the type definitions and
//    member functions as implemented by and documented in the class 'AllocatorMalloc'.
//  - Allocators which require the original untyped pointer and the size and alignment of the
//    allocated memory block for deallocation, for example 'std::pmr::memory_resource'. Those
//    allocators are required to provide the type definitions and member functions as implemented
//    by and documented in the class 'AllocatorMemoryResource'.
//  - Allocators which require the original type for deallocation, for example 'std::allocator'.
//    Those allocators are required to provide the type definitions and member functions as
//    documented in the C++ Standard Library, see
//    https://en.cppreference.com/w/cpp/named_req/Allocator
//
//  Requires that the shared base class type specified as template argument provides the
//  type definitions and member functions as implemented by and documented in the class 'Intruder'.
//
//  Requires that the data class type specified as template argument provides the type definitions
//  and member functions as implemented by and documented in the class 'ControlData'.
//

template<class t_shared_base, class t_data, class t_allocator>
class ControlAlloc
{
public:
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //        The type definitions and member functions required by 'Intruder' start here         //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  using SharedBase = t_shared_base;
  using Data = t_data;
  using Deleter = void;
  using Allocator = t_allocator;
  using UsageValueType = typename t_data::UsageValueType;
  using WeakValueType = typename t_data::WeakValueType;
  using DataValueType = typename t_data::DataValueType;
  using SupportsWeak = typename t_data::SupportsWeak;

  explicit ControlAlloc(DataValueType const p_user_init) noexcept
  : m_adapter(p_user_init)
  {}

  // Allocate storage for a new instance of the specified type, call its constructor
  // using the forwarded arguments, and return a pointer to it on success.
  template<class t_shared, class t_nothrow, class t_shared_allocator, typename... t_args>
  static t_shared *
  create(t_shared_allocator & p_allocator, t_args &&... p_args) noexcept(t_nothrow::value)
  {
    return AllocAdapter::template create<t_shared, t_nothrow>(p_allocator, std::forward<t_args>(p_args)...);
  }

  // Try to initialize the control pointer object and to forward the allocator to it.
  template<class t_shared, class t_forward>
  static t_shared *
  try_init(ControlAlloc & p_self, t_shared * p_shared, t_forward && p_allocator) noexcept
  {
    return AllocAdapter::template try_init(p_self.m_adapter, p_shared, std::forward<t_forward>(p_allocator));
  }

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. Return a pointer to the given object on success.
  template<class t_shared>
  t_shared *
  try_control(t_shared & p_shared) noexcept
  {
    // Check if the address returned from get_shared_base() is equal
    // to the address of the implicitly upcasted shared instance.
    if (t_shared::get_shared_base(*this) != &p_shared)
    {
      return nullptr;
    }
    return (m_adapter.m_data.try_control() != ControlStatus::e_invalid ? &p_shared : nullptr);
  }

  // Try to increment the usage counter and return a pointer to the shared object on success.
  template<class t_shared>
  t_shared *
  try_get_shared() noexcept
  {
    if (!m_adapter.m_data.try_add_ref())
    {
      return nullptr;
    }
    return detail::static_or_dynamic_cast<t_shared>(t_shared::get_shared_base(*this));
  }

  // Return true if this is not controlled (yet).
  bool
  is_uncontrolled() const noexcept
  {
    return m_adapter.m_data.is_uncontrolled();
  }

  // Return maximum usage count value.
  static constexpr UsageValueType
  get_max_usage_count() noexcept
  {
    return t_data::get_max_usage_count();
  }

  // Return maximum weak count value.
  static constexpr WeakValueType
  get_max_weak_count() noexcept
  {
    return t_data::get_max_weak_count();
  }

  // Return the usage count.
  UsageValueType
  use_count() const noexcept
  {
    return m_adapter.m_data.use_count();
  }

  // Return the weak count.
  template<typename Weak = SupportsWeak, typename = std::enable_if_t<Weak::value>>
  WeakValueType
  weak_count() const noexcept
  {
    return m_adapter.m_data.weak_count();
  }

  // Increment the usage counter.
  void
  add_ref() noexcept
  {
    m_adapter.m_data.add_ref();
  }

  // Increment the weak counter.
  template<typename Weak = SupportsWeak, typename = std::enable_if_t<Weak::value>>
  void
  weak_add_ref() noexcept
  {
    m_adapter.m_data.weak_add_ref();
  }

  // Increment the weak counter.
  template<typename Weak = SupportsWeak, typename = std::enable_if_t<Weak::value>>
  static ControlAlloc *
  weak_add_ref(ControlAlloc * p_control) noexcept
  {
    if (p_control != nullptr)
    {
      p_control->m_adapter.m_data.weak_add_ref();
    }
    return p_control;
  }

  // Decrement the usage counter and return true if it reaches zero or was invalid.
  bool
  release() noexcept
  {
    return m_adapter.m_data.release();
  }

  // Decrement the weak counter and return true if it reaches or was zero.
  template<typename Weak = SupportsWeak, typename = std::enable_if_t<Weak::value>>
  bool
  weak_release() noexcept
  {
    return m_adapter.m_data.weak_release();
  }

  // Return maximum user value.
  static constexpr DataValueType
  get_max_user() noexcept
  {
    return t_data::get_max_user();
  }

  // Return the user value.
  DataValueType
  get_user() const noexcept
  {
    return m_adapter.m_data.get_user();
  }

  // Try to set the user value and return true on success.
  bool
  try_set_user(DataValueType const p_user) noexcept
  {
    return m_adapter.m_data.try_set_user(p_user);
  }

  // Delete (non-weak) or destroy (weak) the object. Called when 'pntr_release' returns true.
  // Return a pointer to the control block if it should be deallocated.
  template<class t_shared>
  static typename t_shared::PntrControlType *
  dispose(ControlAlloc & p_control, t_shared & p_shared) noexcept
  {
    PNTR_TRY_LOG_WARNING(p_control.m_adapter.m_data.is_alive(), "disposing object which is still alive");
    p_control.m_adapter.destroy(p_control, p_shared);
    ControlAlloc * control = &p_control;
    if constexpr (SupportsWeak::value)
    {
      if (!p_control.m_adapter.m_data.weak_release())
      {
        control = nullptr;
      }
    }
    return control;
  }

  // Deallocate the memory of the shared instance.
  static void
  deallocate(ControlAlloc * p_control) noexcept
  {
    if (p_control != nullptr)
    {
      AllocAdapter::deallocate(p_control->m_adapter, *p_control);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //         The type definitions and member functions required by 'Intruder' end here          //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

private:
  using AllocAdapter =
    std::conditional_t<detail::HasPointerDeallocate<t_allocator>::value,
                       detail::AllocAdaptPointer<t_shared_base, t_data, t_allocator>,
                       std::conditional_t<detail::HasTypeInfoDeallocate<t_allocator>::value,
                                          detail::AllocAdaptTypeInfo<t_shared_base, t_data, t_allocator>,
                                          detail::AllocAdaptTyped<t_shared_base, t_data, t_allocator>>>;

  AllocAdapter m_adapter;

  ControlAlloc(ControlAlloc const &) = delete;
  ControlAlloc(ControlAlloc &&) = delete;
  ControlAlloc & operator=(ControlAlloc const &) = delete;
  ControlAlloc & operator=(ControlAlloc &&) = delete;
};


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                      pntr/ControlNew.hpp                                       //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                           ControlNew                                           //
//                                                                                                //
//        A control class for 'Intruder', using 'new' expression, deleter, and usage count        //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  Requires that the shared base class type specified as template argument provides the type
//  definitions and member functions as implemented by and documented in the class 'Intruder'.
//
//  Requires that the data class type specified as template argument provides the type definitions
//  and member functions as implemented by and documented in the class 'ControlData'.
//

template<class t_shared_base, class t_data, typename t_deleter>
class ControlNew
{
public:
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //        The type definitions and member functions required by 'Intruder' start here         //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  using SharedBase = t_shared_base;
  using Data = t_data;
  using Deleter = t_deleter;
  using Allocator = void;
  using UsageValueType = typename t_data::UsageValueType;
  using WeakValueType = typename t_data::WeakValueType;
  using DataValueType = typename t_data::DataValueType;
  using SupportsWeak = std::false_type;

  explicit ControlNew(DataValueType const p_user_init) noexcept
  : m_control_deleter(p_user_init)
  {}

  // Allocate storage for a new instance of the specified type, call its constructor
  // using the forwarded arguments, and return a pointer to it on success.
  template<class t_shared, class t_nothrow, typename... t_args>
  static t_shared *
  create(t_args &&... p_args) noexcept(t_nothrow::value)
  {
    t_shared * shared = nullptr;
    if constexpr (t_nothrow::value)
    {
      try
      {
        // Ignore allocation exceptions
        shared = new (std::nothrow) t_shared(std::forward<t_args>(p_args)...);
      }
      catch (...) // Ignore constructor exceptions
      {}
    }
    else
    {
      shared = new t_shared(std::forward<t_args>(p_args)...);
    }
    return shared;
  }

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. Return a pointer to the given object on success.
  template<class t_shared>
  t_shared *
  try_control(t_shared & p_shared) noexcept
  {
    // Check if the address returned from get_shared_base() is equal
    // to the address of the implicitly upcasted shared instance.
    PNTR_ASSERT(t_shared::get_shared_base(*this) == &p_shared);

    switch (m_control_deleter.m_data.try_control())
    {
      case ControlStatus::e_invalid:
        return nullptr;
      case ControlStatus::e_acquired:
        m_control_deleter.template init<std::remove_const_t<t_shared>>();
        break;
      case ControlStatus::e_shared:
        break;
    }
    return &p_shared;
  }

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. Assign the deleter and return a pointer to the given object on success.
  template<class t_shared, class t_forward>
  t_shared *
  try_control(t_shared & p_shared, t_forward && p_deleter) noexcept
  {
    // Check if the address returned from get_shared_base() is equal
    // to the address of the implicitly upcasted shared instance.
    PNTR_ASSERT(t_shared::get_shared_base(*this) == &p_shared);

    switch (m_control_deleter.m_data.try_control())
    {
      case ControlStatus::e_invalid:
        return nullptr;
      case ControlStatus::e_acquired:
        m_control_deleter.deleter() = std::forward<t_forward>(p_deleter);
        break;
      case ControlStatus::e_shared:
        break;
    }
    return &p_shared;
  }

  // Try to increment the usage counter and return a pointer to the shared object on success.
  template<class t_shared>
  t_shared *
  try_get_shared() noexcept
  {
    if (!m_control_deleter.m_data.try_add_ref())
    {
      return nullptr;
    }
    return detail::static_or_dynamic_cast<t_shared>(t_shared::get_shared_base(*this));
  }

  // Re-initialize the usage counter of an expired object. Return true if it is or was uncontrolled.
  bool
  try_revive() noexcept
  {
    return m_control_deleter.m_data.try_revive();
  }

  // Return maximum usage count value.
  static constexpr UsageValueType
  get_max_usage_count() noexcept
  {
    return t_data::get_max_usage_count();
  }

  // Return the usage count.
  UsageValueType
  use_count() const noexcept
  {
    return m_control_deleter.m_data.use_count();
  }

  // Increment the usage counter.
  void
  add_ref() noexcept
  {
    m_control_deleter.m_data.add_ref();
  }

  // Decrement the usage counter and return true if it reaches zero or was invalid.
  bool
  release() noexcept
  {
    return m_control_deleter.m_data.release();
  }

  // Return maximum user value.
  static constexpr DataValueType
  get_max_user() noexcept
  {
    return t_data::get_max_user();
  }

  // Return the user value.
  DataValueType
  get_user() const noexcept
  {
    return m_control_deleter.m_data.get_user();
  }

  // Try to set the user value and return true on success.
  bool
  try_set_user(DataValueType const p_user) noexcept
  {
    return m_control_deleter.m_data.try_set_user(p_user);
  }

  // Delete (non-weak) or destroy (weak) the object. Called when 'pntr_release' returns true.
  // Return a pointer to the control block if it should be deallocated.
  template<class t_shared>
  static typename t_shared::PntrControlType *
  dispose(ControlNew & p_control, t_shared & p_shared) noexcept
  {
    PNTR_TRY_LOG_WARNING(p_control.m_control_deleter.m_data.is_alive(), "disposing object which is still alive");
    ControlDeleter::destroy(p_control.m_control_deleter, &p_shared);
    return nullptr;
  }

  // Deallocate the memory of the shared instance.
  static void
  deallocate([[maybe_unused]] ControlNew * p_control) noexcept
  {
    PNTR_TRY_LOG_ERROR(p_control != nullptr, "should never reveice a valid pointer");
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //         The type definitions and member functions required by 'Intruder' end here          //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

private:
  using ControlDeleter = detail::ControlDeleter<t_shared_base, t_data, t_deleter, detail::is_empty_base<t_deleter>>;

  ControlDeleter m_control_deleter;

  ControlNew(ControlNew const &) = delete;
  ControlNew(ControlNew &&) = delete;
  ControlNew & operator=(ControlNew const &) = delete;
  ControlNew & operator=(ControlNew &&) = delete;
};


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                       pntr/Intruder.hpp                                        //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                            Intruder                                            //
//                                                                                                //
//       An intrusive base class that embeds a control block for 'SharedPtr' and 'WeakPtr'        //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  Requires that the control class type specified as template argument
//  provides the type definitions and member functions as implemented
//  by and documented in the classes 'ControlNew' and 'ControlAlloc'.
//

template<class t_control>
class Intruder
{
public:
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  // The type definitions and member functions required by 'SharedPtr' and 'WeakPtr' start here //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // 'PntrControlType' is the intrusive control class. It can be 'void'.
  using PntrControlType = t_control;

  // 'PntrSharedBase' is the base class specified as template parameter for the control class.
  // It has to be statically (not virtual) derived from the 'Intruder' class.
  using PntrSharedBase = typename t_control::SharedBase;

  // 'PntrUsageValueType' is the integer type used for usage reference counting.
  using PntrUsageValueType = typename t_control::UsageValueType;

  // 'PntrWeakValueType' is the integer type used for weak reference counting.
  using PntrWeakValueType = typename t_control::WeakValueType;

  // 'PntrDeleter' type has to be either 'void' or a supported deleter type.
  using PntrDeleter = typename t_control::Deleter;

  // 'PntrAllocator' type has to be either 'void' or a supported allocator type.
  using PntrAllocator = typename t_control::Allocator;

  // 'PntrSupportsWeak' indicates if weak reference counting is supported.
  // It has to be either 'std::true_type' or 'std::false_type'.
  using PntrSupportsWeak = typename t_control::SupportsWeak;

private:
  // Most functions should be private to prevent the derived class from messing around.

  // Allocate storage for a new instance of the specified type, call its constructor
  // using the forwarded arguments, and return a pointer to it on success.
  template<class t_shared, class t_nothrow, typename... t_args>
  static t_shared *
  pntr_create(t_args &&... p_args) noexcept(t_nothrow::value)
  {
    return t_control::template create<t_shared, t_nothrow>(std::forward<t_args>(p_args)...);
  }

  // Use the allocator to allocate storage for a new instance of the specified type,
  // call its constructor using the forwarded arguments, and assign the allocator
  // and return a pointer to the new instance on success.
  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  static std::enable_if_t<!std::is_void_v<PntrAllocator>, t_shared *>
  pntr_create_with_allocator(t_forward && p_allocator, t_args &&... p_args) noexcept(t_nothrow::value)
  {
    t_shared * shared = t_control::template create<t_shared, t_nothrow>(p_allocator, std::forward<t_args>(p_args)...);
    if (shared != nullptr)
    {
      shared = t_control::template try_init(shared->control(), shared, std::forward<t_forward>(p_allocator));
    }
    return shared;
  }

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. Return the given pointer on success.
  template<class t_shared>
  static t_shared *
  pntr_try_control(t_shared * p_shared) noexcept
  {
    return (p_shared == nullptr ? nullptr : p_shared->control().try_control(*p_shared));
  }

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. Assign the deleter and return the given pointer on success.
  template<class t_shared, class t_forward, typename = std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>>>
  static t_shared *
  pntr_try_control(t_shared * p_shared, t_forward && p_deleter) noexcept
  {
    return (p_shared == nullptr ? nullptr
                                : p_shared->control().try_control(*p_shared, std::forward<t_forward>(p_deleter)));
  }

  // Try to increment the usage counter and return a pointer to the shared object on success.
  template<class t_shared>
  static t_shared *
  pntr_try_get_shared(PntrControlType & p_control) noexcept
  {
    return p_control.template try_get_shared<t_shared>();
  }

  // Return an owner-based pointer for ordering in associative containers.
  // All 'SharedPtr' and 'WeakPtr' who own the same object should return the same pointer.
  // Types which support 'WeakPtr' have to return the address of the control class instance.
  void const *
  pntr_get_owner() const noexcept
  {
    // Check assumption that addresses of 'Intruder' and 'm_control_storage' are identical.
    static_assert(offsetof(Intruder<t_control>, m_control_storage) == 0u);
    static_assert(sizeof(Intruder) == sizeof(t_control));

    return this;
  }

public:
  // Return the current usage count.
  PntrUsageValueType
  pntr_use_count() const noexcept
  {
    return control().use_count();
  }

  // Return the current weak count.
  PntrWeakValueType
  pntr_weak_count() const noexcept
  {
    PntrWeakValueType weak_count{};
    if constexpr (PntrSupportsWeak::value)
    {
      weak_count = control().weak_count();
    }
    return weak_count;
  }

private:
  // Increment the usage counter.
  void
  pntr_add_ref() const noexcept
  {
    control().add_ref();
  }

  // Decrement the usage counter and return true if it reaches zero or was invalid.
  bool
  pntr_release() const noexcept
  {
    return control().release();
  }

  // Delete (non-weak) or destroy (weak) the object. Called when 'pntr_release' returns true.
  // Return a pointer to the control block if it should be deallocated.
  template<class t_shared>
  static PntrControlType *
  pntr_dispose(t_shared * p_shared) noexcept
  {
    return (p_shared == nullptr ? nullptr : t_control::template dispose(p_shared->control(), *p_shared));
  }

  // Deallocate the object. Called when the weak count reaches zero.
  static void
  pntr_deallocate(PntrControlType * p_control) noexcept
  {
    t_control::deallocate(p_control);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //  The type definitions and member functions required by 'SharedPtr' and 'WeakPtr' end here  //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

public:
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //      The type definitions and member functions required by control classes start here      //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // Accept a control instance and return a pointer to the shared base instance.
  static PntrSharedBase *
  get_shared_base(t_control & p_control) noexcept
  {
    return static_cast<PntrSharedBase *>(reinterpret_cast<Intruder *>(&p_control));
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //       The type definitions and member functions required by control classes end here       //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // 'Intruder' is copyable and movable, but the control block is not.

  Intruder(Intruder const &) noexcept
  {
    new (&m_control_storage) t_control;
  }

  Intruder(Intruder &&) noexcept
  {
    new (&m_control_storage) t_control;
  }

  Intruder &
  operator=(Intruder const &) noexcept
  {
    return *this;
  }

  Intruder &
  operator=(Intruder &&) noexcept
  {
    return *this;
  }

  // The function 'shared_from_this' is equivalent to the one in 'std::enable_shared_from_this',
  // but does not have its restriction that it is permitted to be called only from an object that
  // is already managed by a shared pointer. So it is permitted to call 'shared_from_this' from
  // 'Intruder' to acquire control of a shared object, but it can be undefined behaviour if the
  // shared base is not fully initialized yet, for example calling it during the member
  // initialization of a constructor of the shared base.

  SharedPtr<PntrSharedBase>
  shared_from_this() noexcept
  {
    return SharedPtr<PntrSharedBase>(static_cast<PntrSharedBase *>(this));
  }

  SharedPtr<PntrSharedBase const>
  shared_from_this() const noexcept
  {
    return SharedPtr<PntrSharedBase const>(static_cast<PntrSharedBase const *>(this));
  }

  // The function 'weak_from_this' is equivalent to the one in 'std::enable_shared_from_this',
  // but does not have its restriction that it is empty until the object is managed by a shared
  // pointer. So it is permitted to call 'weak_from_this' from 'Intruder' at any time. It can
  // even be used to construct a 'SharedPtr' to acquire control of the shared object, though
  // this can be undefined behaviour if the shared base is not fully initialized yet, for
  // example calling it during the member initialization of a constructor of the shared base.

  template<typename t_weak = PntrSupportsWeak, typename = std::enable_if_t<t_weak::value>>
  WeakPtr<PntrSharedBase>
  weak_from_this() noexcept
  {
    return WeakPtr<PntrSharedBase>(control());
  }

  template<typename t_weak = PntrSupportsWeak, typename = std::enable_if_t<t_weak::value>>
  WeakPtr<PntrSharedBase const>
  weak_from_this() const noexcept
  {
    return WeakPtr<PntrSharedBase const>(control());
  }

  // Re-initialize the usage counter of an expired object. Should only be used if an object was
  // recycled with a custom deleter. It is undefined behaviour to call it in other cases.
  template<class t_deleter = PntrDeleter, typename = std::enable_if_t<!std::is_void_v<t_deleter>>>
  bool
  pntr_try_revive() const noexcept
  {
    return control().try_revive();
  }

  // Return maximum usage count value.
  static constexpr PntrUsageValueType
  pntr_get_max_usage_count() noexcept
  {
    return t_control::get_max_usage_count();
  }

  // Return maximum weak count value.
  static constexpr PntrWeakValueType
  pntr_get_max_weak_count() noexcept
  {
    PntrWeakValueType max_weak_count{};
    if constexpr (PntrSupportsWeak::value)
    {
      max_weak_count = t_control::get_max_weak_count();
    }
    return max_weak_count;
  }

  // 'PntrDataValueType' is the integer type used for the user value.
  using PntrDataValueType = typename t_control::DataValueType;

  // Return maximum user value, see 'ControlData'.
  static constexpr PntrDataValueType
  pntr_get_max_user() noexcept
  {
    return t_control::get_max_user();
  }

  // Return the user value, see 'ControlData'.
  PntrDataValueType
  pntr_get_user() const noexcept
  {
    return control().get_user();
  }

  // Try to set the user value and return true on success, see 'ControlData'.
  bool
  pntr_try_set_user(PntrDataValueType const p_user) const noexcept
  {
    return control().try_set_user(p_user);
  }

protected:
  explicit Intruder(PntrDataValueType const p_user_init = 0u) noexcept
  {
    new (&m_control_storage) t_control(p_user_init);
  }

  ~Intruder() noexcept
  {
    if constexpr (!std::is_void_v<PntrDeleter>)
    {
      control().~t_control();
    }
    else if (control().is_uncontrolled())
    {
      control().~t_control();
    }
  }

private:
  t_control &
  control() const noexcept
  {
    return *std::launder(reinterpret_cast<t_control *>(&m_control_storage));
  }

  alignas(t_control) mutable char m_control_storage[sizeof(t_control)];

  template<class t_shared>
  friend class SharedPtr;
  template<class t_shared>
  friend class WeakPtr;

  template<class t_shared, class t_nothrow, typename... t_args>
  friend SharedPtr<t_shared> detail::make_shared_impl(t_args &&... p_args) noexcept(t_nothrow::value);
  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  friend std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>, SharedPtr<t_shared>>
  detail::make_shared_with_deleter_impl(t_forward && p_deleter, t_args &&... p_args) noexcept(t_nothrow::value);
  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  friend std::enable_if_t<!std::is_void_v<typename t_shared::PntrAllocator>, SharedPtr<t_shared>>
  detail::allocate_shared_impl(t_forward && p_allocator, t_args &&... p_args) noexcept(t_nothrow::value);
};


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                       pntr/SharedPtr.hpp                                       //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN

template<class t_shared, class t_other>
SharedPtr<t_shared> static_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
template<class t_shared, class t_other>
SharedPtr<t_shared> static_pointer_cast(SharedPtr<t_other> && p_other) noexcept;
template<class t_shared, class t_other>
SharedPtr<t_shared> dynamic_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
template<class t_shared, class t_other>
SharedPtr<t_shared> dynamic_pointer_cast(SharedPtr<t_other> && p_other) noexcept;
template<class t_shared, class t_other>
SharedPtr<t_shared> const_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
template<class t_shared, class t_other>
SharedPtr<t_shared> const_pointer_cast(SharedPtr<t_other> && p_other) noexcept;


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                           SharedPtr                                            //
//                                                                                                //
//                      A smart pointer that uses an intrusive control block                      //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  Requires that the specified template type is a class that provides the type definitions
//  and member functions as implemented by and documented in the class 'Intruder'.
//
//  It is recommended to inherit the specified class from the class 'Intruder' and to overload
//  selected functions if required.
//
//  Custom reference counting implementations have the benefit that existing reference counters
//  can be used, but it might not be compatible with existing intrusive weak counters.
//
//  The undocumented functions are equivalent to 'std::shared_ptr'. Its aliasing feature is not
//  supported by design, as it would require to store an additional pointer.
//

template<class t_shared>
class SharedPtr
{
public:
  using element_type = t_shared;
  using weak_type = std::conditional_t<t_shared::PntrSupportsWeak::value, WeakPtr<t_shared>, void>;

  constexpr SharedPtr() noexcept
  : m_shared(nullptr)
  {}

  constexpr SharedPtr(std::nullptr_t) noexcept
  : m_shared(nullptr)
  {}

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. On failure the contructed 'SharedPtr' will be empty.
  // It is the responsibility of the user to allocate the object in a way that is compatible
  // to the deallocation used by the control block.
  // It is recommended to use make_shared, make_shared_with_deleter, or allocate_shared to let
  // the control block allocate the object.
  template<class t_other>
  SharedPtr(t_other * const p_shared) noexcept
  : m_shared(t_shared::template pntr_try_control(p_shared))
  {}

  // Same as above, but in addition accept a deleter object or function that is convertible
  // and will be assigned to the deleter stored in the control block.
  template<class t_other, class t_forward, typename = std::enable_if_t<!std::is_void_v<typename t_other::PntrDeleter>>>
  SharedPtr(t_other * p_shared, t_forward && p_deleter) noexcept
  : m_shared(t_shared::template pntr_try_control(p_shared, std::forward<t_forward>(p_deleter)))
  {}

  // Same as above, but accept a 'std::unique_ptr' with deleter object or function that is
  // convertible and will be assigned to the deleter stored in the control block.
  template<class t_other, class t_deleter,
           typename = std::enable_if_t<std::is_convertible_v<t_deleter, typename t_other::PntrDeleter>>>
  SharedPtr(std::unique_ptr<t_other, t_deleter> && p_unique) noexcept
  : m_shared(t_shared::template pntr_try_control(p_unique.release(), std::move(p_unique.get_deleter())))
  {}

  // Same as above, but accept a 'std::unique_ptr' with a const object if the deleter is a templated
  // empty class that can be default constructed and saved in the control block.
  template<class t_other, template<class> class t_deleter,
           typename = std::enable_if_t<
             std::is_convertible_v<t_deleter<std::remove_const_t<t_other>>, typename t_other::PntrDeleter>
             && std::is_nothrow_default_constructible_v<t_deleter<std::remove_const_t<t_other>>>
             && detail::is_empty_base<t_deleter<t_other>>>>
  SharedPtr(std::unique_ptr<t_other, t_deleter<t_other>> && p_unique) noexcept
  : m_shared(t_shared::template pntr_try_control(p_unique.release(), t_deleter<std::remove_const_t<t_other>>()))
  {}

  SharedPtr(SharedPtr const & p_other) noexcept
  : m_shared(p_other.m_shared)
  {
    if (m_shared != nullptr)
    {
      m_shared->pntr_add_ref();
    }
  }

  SharedPtr(SharedPtr && p_other) noexcept
  : m_shared(p_other.m_shared)
  {
    p_other.m_shared = nullptr;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  SharedPtr(SharedPtr<t_other> const & p_other) noexcept
  : m_shared(p_other.m_shared)
  {
    if (m_shared != nullptr)
    {
      m_shared->pntr_add_ref();
    }
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  SharedPtr(SharedPtr<t_other> && p_other) noexcept
  : m_shared(p_other.m_shared)
  {
    p_other.m_shared = nullptr;
  }

  // Throws 'std::bad_weak_ptr' if the weak pointer is empty.
  // The non-throwing alternative is to create the shared pointer with 'WeakPtr::lock()'.
  template<class t_other, typename = std::enable_if_t<t_other::PntrSupportsWeak::value>>
  explicit SharedPtr(WeakPtr<t_other> const & p_weak)
  : m_shared(nullptr)
  {
    if (p_weak.m_control != nullptr)
    {
      m_shared = t_shared::template pntr_try_get_shared<t_other>(*p_weak.m_control);
    }
    else
    {
      throw std::bad_weak_ptr();
    }
  }

  ~SharedPtr() noexcept
  {
    if (m_shared != nullptr && m_shared->pntr_release())
    {
      t_shared::pntr_deallocate(t_shared::template pntr_dispose(const_cast<std::remove_const_t<t_shared> *>(m_shared)));
    }
  }

  SharedPtr &
  operator=(SharedPtr const & p_other) noexcept
  {
    SharedPtr(p_other).swap(*this);
    return *this;
  }

  SharedPtr &
  operator=(SharedPtr && p_other) noexcept
  {
    SharedPtr(std::move(p_other)).swap(*this);
    return *this;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  SharedPtr &
  operator=(SharedPtr<t_other> const & p_other) noexcept
  {
    SharedPtr(p_other).swap(*this);
    return *this;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  SharedPtr &
  operator=(SharedPtr<t_other> && p_other) noexcept
  {
    SharedPtr(std::move(p_other)).swap(*this);
    return *this;
  }

  explicit operator bool() const noexcept
  {
    return m_shared != nullptr;
  }

  t_shared *
  get() const noexcept
  {
    return m_shared;
  }

  t_shared &
  operator*() const noexcept
  {
    PNTR_ASSERT(m_shared != nullptr);
    return *m_shared;
  }

  t_shared *
  operator->() const noexcept
  {
    PNTR_ASSERT(m_shared != nullptr);
    return m_shared;
  }

  typename t_shared::PntrUsageValueType
  use_count() const noexcept
  {
    return m_shared != nullptr ? m_shared->pntr_use_count() : typename t_shared::PntrUsageValueType{};
  }

  typename t_shared::PntrWeakValueType
  weak_count() const noexcept
  {
    return m_shared != nullptr ? m_shared->pntr_weak_count() : typename t_shared::PntrWeakValueType{};
  }

  template<class t_other>
  bool
  owner_before(SharedPtr<t_other> const & p_other) const noexcept
  {
    return (m_shared != nullptr ? m_shared->pntr_get_owner() : nullptr)
         < (p_other.m_shared != nullptr ? p_other.m_shared->pntr_get_owner() : nullptr);
  }

  template<class t_other, typename = std::enable_if_t<t_other::PntrSupportsWeak::value>>
  bool
  owner_before(WeakPtr<t_other> const & p_other) const noexcept
  {
    return (m_shared != nullptr ? m_shared->pntr_get_owner() : nullptr) < p_other.get_owner();
  }

  void
  reset() noexcept
  {
    SharedPtr().swap(*this);
  }

  template<class t_other>
  void
  reset(t_other * const p_shared) noexcept
  {
    SharedPtr(p_shared).swap(*this);
  }

  template<class t_other, class t_forward, typename = std::enable_if_t<!std::is_void_v<typename t_other::PntrDeleter>>>
  void
  reset(t_other * p_shared, t_forward && p_deleter) noexcept
  {
    SharedPtr(p_shared, std::forward<t_forward>(p_deleter)).swap(*this);
  }

  template<class t_other, class t_deleter,
           typename = std::enable_if_t<std::is_convertible_v<t_deleter, typename t_other::PntrDeleter>>>
  void
  reset(std::unique_ptr<t_other, t_deleter> && p_unique) noexcept
  {
    SharedPtr(std::move(p_unique)).swap(*this);
  }

  template<class t_other, template<class> class t_deleter,
           typename = std::enable_if_t<
             std::is_convertible_v<t_deleter<std::remove_const_t<t_other>>, typename t_other::PntrDeleter>
             && std::is_nothrow_default_constructible_v<t_deleter<std::remove_const_t<t_other>>>
             && detail::is_empty_base<t_deleter<t_other>>>>
  void
  reset(std::unique_ptr<t_other, t_deleter<t_other>> && p_unique) noexcept
  {
    SharedPtr(std::move(p_unique)).swap(*this);
  }

  void
  swap(SharedPtr & p_other) noexcept
  {
    std::swap(m_shared, p_other.m_shared);
  }

private:
  // Only called from 'make_shared', 'make_shared_with_deleter', pointer casts, and 'WeakPtr::lock'.
  SharedPtr(t_shared * const p_shared, bool p_add_ref) noexcept
  : m_shared(p_shared)
  {
    if (m_shared != nullptr && p_add_ref)
    {
      m_shared->pntr_add_ref();
    }
  }

  // Only called from pointer casts with move semantics
  t_shared *
  detach() noexcept
  {
    t_shared * const shared = m_shared;
    m_shared = nullptr;
    return shared;
  }

  t_shared * m_shared;

  template<class t_other>
  friend class SharedPtr;
  friend class WeakPtr<t_shared>;

  template<class t_self, class t_nothrow, typename... t_args>
  friend SharedPtr<t_self> detail::make_shared_impl(t_args &&... p_args) noexcept(t_nothrow::value);
  template<class t_self, class t_nothrow, class t_forward, typename... t_args>
  friend std::enable_if_t<!std::is_void_v<typename t_self::PntrDeleter>, SharedPtr<t_self>>
  detail::make_shared_with_deleter_impl(t_forward && p_deleter, t_args &&... p_args) noexcept(t_nothrow::value);
  template<class t_self, class t_nothrow, class t_forward, typename... t_args>
  friend std::enable_if_t<!std::is_void_v<typename t_self::PntrAllocator>, SharedPtr<t_self>>
  detail::allocate_shared_impl(t_forward && p_allocator, t_args &&... p_args) noexcept(t_nothrow::value);

  template<class t_self, class t_other>
  friend SharedPtr<t_self> static_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
  template<class t_self, class t_other>
  friend SharedPtr<t_self> static_pointer_cast(SharedPtr<t_other> && p_other) noexcept;
  template<class t_self, class t_other>
  friend SharedPtr<t_self> dynamic_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
  template<class t_self, class t_other>
  friend SharedPtr<t_self> dynamic_pointer_cast(SharedPtr<t_other> && p_other) noexcept;
  template<class t_self, class t_other>
  friend SharedPtr<t_self> const_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
  template<class t_self, class t_other>
  friend SharedPtr<t_self> const_pointer_cast(SharedPtr<t_other> && p_other) noexcept;
};


// clang-format off

template<class L, class R> inline bool operator==(SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() == r.get(); }
template<class L, class R> inline bool operator!=(SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() != r.get(); }
template<class L, class R> inline bool operator< (SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() <  r.get(); }
template<class L, class R> inline bool operator> (SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() >  r.get(); }
template<class L, class R> inline bool operator<=(SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() <= r.get(); }
template<class L, class R> inline bool operator>=(SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() >= r.get(); }

template<class T> inline bool operator==(SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() == static_cast<T *>(nullptr); }
template<class T> inline bool operator!=(SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() != static_cast<T *>(nullptr); }
template<class T> inline bool operator< (SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() <  static_cast<T *>(nullptr); }
template<class T> inline bool operator> (SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() >  static_cast<T *>(nullptr); }
template<class T> inline bool operator<=(SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() <= static_cast<T *>(nullptr); }
template<class T> inline bool operator>=(SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() >= static_cast<T *>(nullptr); }

template<class T> inline bool operator==(std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) == r.get(); }
template<class T> inline bool operator!=(std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) != r.get(); }
template<class T> inline bool operator< (std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) <  r.get(); }
template<class T> inline bool operator> (std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) >  r.get(); }
template<class T> inline bool operator<=(std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) <= r.get(); }
template<class T> inline bool operator>=(std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) >= r.get(); }

// clang-format on


namespace detail
{
  template<class t_shared, class t_nothrow, typename... t_args>
  SharedPtr<t_shared>
  make_shared_impl(t_args &&... p_args) noexcept(t_nothrow::value)
  {
    using Shared = std::remove_const_t<t_shared>;
    using Allocator = typename Shared::PntrAllocator;
    Shared * created = nullptr;
    if constexpr (std::is_void_v<Allocator>)
    {
      created = Shared::template pntr_create<Shared, t_nothrow>(std::forward<t_args>(p_args)...);
    }
    else
    {
      created =
        Shared::template pntr_create_with_allocator<Shared, t_nothrow>(Allocator(), std::forward<t_args>(p_args)...);
    }
    Shared * const shared = Shared::template pntr_try_control(created);
    if (shared == nullptr)
    {
      Shared::pntr_deallocate(Shared::template pntr_dispose(created));
    }
    return SharedPtr<t_shared>(const_cast<t_shared *>(shared), false);
  }

  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>, SharedPtr<t_shared>>
  make_shared_with_deleter_impl(t_forward && p_deleter, t_args &&... p_args) noexcept(t_nothrow::value)
  {
    using Shared = std::remove_const_t<t_shared>;
    Shared * const created = Shared::template pntr_create<Shared, t_nothrow>(std::forward<t_args>(p_args)...);
    Shared * const shared = Shared::template pntr_try_control(created, std::forward<t_forward>(p_deleter));
    if (shared == nullptr)
    {
      Shared::pntr_deallocate(Shared::template pntr_dispose(created));
    }
    return SharedPtr<t_shared>(const_cast<t_shared *>(shared), false);
  }

  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  std::enable_if_t<!std::is_void_v<typename t_shared::PntrAllocator>, SharedPtr<t_shared>>
  allocate_shared_impl(t_forward && p_allocator, t_args &&... p_args) noexcept(t_nothrow::value)
  {
    using Shared = std::remove_const_t<t_shared>;
    Shared * const created =
      Shared::template pntr_create_with_allocator<Shared, t_nothrow>(std::forward<t_forward>(p_allocator),
                                                                     std::forward<t_args>(p_args)...);
    Shared * const shared = Shared::template pntr_try_control(created);
    if (shared == nullptr)
    {
      Shared::pntr_deallocate(Shared::template pntr_dispose(created));
    }
    return SharedPtr<t_shared>(const_cast<t_shared *>(shared), false);
  }
} // namespace detail


// May throw allocation and constructor exceptions.
template<class t_shared, typename... t_args>
SharedPtr<t_shared>
make_shared(t_args &&... p_args)
{
  return detail::make_shared_impl<t_shared, std::false_type>(std::forward<t_args>(p_args)...);
}

// Doesn't throw. The returned 'SharedPtr' is empty if the allocation or construction fails.
template<class t_shared, typename... t_args>
SharedPtr<t_shared>
make_shared_nothrow(t_args &&... p_args) noexcept
{
  return detail::make_shared_impl<t_shared, std::true_type>(std::forward<t_args>(p_args)...);
}

// May throw allocation and constructor exceptions.
template<class t_shared, class t_forward, typename... t_args>
std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>, SharedPtr<t_shared>>
make_shared_with_deleter(t_forward && p_deleter, t_args &&... p_args)
{
  return detail::make_shared_with_deleter_impl<t_shared, std::false_type>(std::forward<t_forward>(p_deleter),
                                                                          std::forward<t_args>(p_args)...);
}

// Doesn't throw. The returned 'SharedPtr' is empty if the allocation or construction fails.
template<class t_shared, class t_forward, typename... t_args>
std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>, SharedPtr<t_shared>>
make_shared_with_deleter_nothrow(t_forward && p_deleter, t_args &&... p_args) noexcept
{
  return detail::make_shared_with_deleter_impl<t_shared, std::true_type>(std::forward<t_forward>(p_deleter),
                                                                         std::forward<t_args>(p_args)...);
}

// May throw allocation and constructor exceptions.
template<class t_shared, class t_forward, typename... t_args>
std::enable_if_t<!std::is_void_v<typename t_shared::PntrAllocator>, SharedPtr<t_shared>>
allocate_shared(t_forward && p_allocator, t_args &&... p_args)
{
  return detail::allocate_shared_impl<t_shared, std::false_type>(std::forward<t_forward>(p_allocator),
                                                                 std::forward<t_args>(p_args)...);
}

// Doesn't throw. The returned 'SharedPtr' is empty if the allocation or construction fails.
template<class t_shared, class t_forward, typename... t_args>
std::enable_if_t<!std::is_void_v<typename t_shared::PntrAllocator>, SharedPtr<t_shared>>
allocate_shared_nothrow(t_forward && p_allocator, t_args &&... p_args) noexcept
{
  return detail::allocate_shared_impl<t_shared, std::true_type>(std::forward<t_forward>(p_allocator),
                                                                std::forward<t_args>(p_args)...);
}


template<class t_shared, class t_other>
SharedPtr<t_shared>
static_pointer_cast(SharedPtr<t_other> const & p_other) noexcept
{
  return SharedPtr<t_shared>(static_cast<t_shared *>(p_other.get()), true);
}

template<class t_shared, class t_other>
SharedPtr<t_shared>
static_pointer_cast(SharedPtr<t_other> && p_other) noexcept
{
  return SharedPtr<t_shared>(static_cast<t_shared *>(p_other.detach()), false);
}

template<class t_shared, class t_other>
SharedPtr<t_shared>
dynamic_pointer_cast(SharedPtr<t_other> const & p_other) noexcept
{
  return SharedPtr<t_shared>(dynamic_cast<t_shared *>(p_other.get()), true);
}

template<class t_shared, class t_other>
SharedPtr<t_shared>
dynamic_pointer_cast(SharedPtr<t_other> && p_other) noexcept
{
  t_shared * const shared = dynamic_cast<t_shared *>(p_other.get());
  if (shared != nullptr)
  {
    p_other.detach();
  }
  return SharedPtr<t_shared>(shared, false);
}

template<class t_shared, class t_other>
SharedPtr<t_shared>
const_pointer_cast(SharedPtr<t_other> const & p_other) noexcept
{
  return SharedPtr<t_shared>(const_cast<t_shared *>(p_other.get()), true);
}

template<class t_shared, class t_other>
SharedPtr<t_shared>
const_pointer_cast(SharedPtr<t_other> && p_other) noexcept
{
  return SharedPtr<t_shared>(const_cast<t_shared *>(p_other.detach()), false);
}


template<class t_shared>
void
swap(SharedPtr<t_shared> & p_lhs, SharedPtr<t_shared> & p_rhs) noexcept
{
  p_lhs.swap(p_rhs);
}

template<class t_char, class t_traits, class t_shared>
std::basic_ostream<t_char, t_traits> &
operator<<(std::basic_ostream<t_char, t_traits> & p_ostream, SharedPtr<t_shared> const & p_shared) noexcept
{
  return p_ostream << p_shared.get();
}


PNTR_NAMESPACE_END


namespace std
{
  template<class t_shared>
  struct hash<::PNTR_NAMESPACE::SharedPtr<t_shared>>
  {
    std::size_t
    operator()(::PNTR_NAMESPACE::SharedPtr<t_shared> const & p_shared) const noexcept
    {
      return std::hash<t_shared *>()(p_shared.get());
    }
  };
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                        pntr/WeakPtr.hpp                                        //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                            WeakPtr                                             //
//                                                                                                //
//                      A weak pointer that uses an intrusive control block                       //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  Requires that the specified template type is a class that provides the type definitions
//  and member functions as implemented by and documented in the class 'Intruder'.
//

template<class t_shared>
class WeakPtr
{
  using ControlType = typename t_shared::PntrControlType;

public:
  using element_type = t_shared;

  constexpr WeakPtr() noexcept
  : m_control(nullptr)
  {}

  WeakPtr(WeakPtr const & p_other) noexcept
  : m_control(ControlType::weak_add_ref(p_other.m_control))
  {}

  WeakPtr(WeakPtr && p_other) noexcept
  : m_control(p_other.m_control)
  {
    p_other.m_control = nullptr;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr(WeakPtr<t_other> const & p_other) noexcept
  : m_control(ControlType::weak_add_ref(p_other.m_control))
  {}

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr(WeakPtr<t_other> && p_other) noexcept
  : m_control(p_other.m_control)
  {
    p_other.m_control = nullptr;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr(SharedPtr<t_other> const & p_shared_ptr) noexcept
  : m_control(ControlType::weak_add_ref(p_shared_ptr ? &p_shared_ptr->control() : nullptr))
  {}

  WeakPtr(ControlType & p_control) noexcept
  : m_control(&p_control)
  {
    m_control->weak_add_ref();
  }

  ~WeakPtr() noexcept
  {
    if (m_control != nullptr && m_control->weak_release())
    {
      t_shared::pntr_deallocate(m_control);
    }
  }

  WeakPtr &
  operator=(WeakPtr const & p_other) noexcept
  {
    WeakPtr(p_other).swap(*this);
    return *this;
  }

  WeakPtr &
  operator=(WeakPtr && p_other) noexcept
  {
    WeakPtr(std::move(p_other)).swap(*this);
    return *this;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr &
  operator=(WeakPtr<t_other> const & p_other) noexcept
  {
    WeakPtr(p_other).swap(*this);
    return *this;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr &
  operator=(WeakPtr<t_other> && p_other) noexcept
  {
    WeakPtr(std::move(p_other)).swap(*this);
    return *this;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr &
  operator=(SharedPtr<t_other> const & p_shared_ptr) noexcept
  {
    WeakPtr(p_shared_ptr).swap(*this);
    return *this;
  }

  bool
  is_empty() const noexcept
  {
    return (m_control == nullptr);
  }

  void
  reset() noexcept
  {
    WeakPtr().swap(*this);
  }

  void
  swap(WeakPtr & p_other) noexcept
  {
    std::swap(m_control, p_other.m_control);
  }

  typename ControlType::UsageValueType
  use_count() const noexcept
  {
    return m_control != nullptr ? m_control->use_count() : typename ControlType::UsageValueType{};
  }

  typename ControlType::WeakValueType
  weak_count() const noexcept
  {
    return m_control != nullptr ? m_control->weak_count() : typename ControlType::WeakValueType{};
  }

  bool
  expired() const noexcept
  {
    return (m_control != nullptr && m_control->use_count() == 0u);
  }

  SharedPtr<t_shared>
  lock() const noexcept
  {
    return SharedPtr<t_shared>(m_control != nullptr ? m_control->template try_get_shared<t_shared>() : nullptr, false);
  }

  template<class t_other>
  bool
  owner_before(WeakPtr<t_other> const & p_other) const noexcept
  {
    return get_owner() < p_other.get_owner();
  }

  template<class t_other>
  bool
  owner_before(SharedPtr<t_other> const & p_other) const noexcept
  {
    return get_owner() < (p_other ? p_other->pntr_get_owner() : nullptr);
  }

private:
  void const *
  get_owner() const noexcept
  {
    return m_control;
  }

  ControlType * m_control;

  template<class t_other>
  friend class WeakPtr;

  template<class t_self>
  friend class SharedPtr;
};


template<class t_shared>
void
swap(WeakPtr<t_shared> & p_lhs, WeakPtr<t_shared> & p_rhs) noexcept
{
  p_lhs.swap(p_rhs);
}


PNTR_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                         pntr/pntr.hpp                                          //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

PNTR_NAMESPACE_BEGIN


// clang-format off

// 'ThreadSafe' selects an atomic reference counter which is safe to use with multi-threading.
using ThreadSafe = std::true_type;
// 'ThreadUnsafe' selects a regular reference counter which is faster to use with single-threading.
using ThreadUnsafe = std::false_type;


// See 'ControlData' for a documentation of 't_control_value'
template<typename t_control_value = std::uint32_t,
         unsigned t_usage_bits = detail::type_bits<t_control_value>()>
using ControlNewDataThreadSafe = ControlData<CounterThreadSafe, t_control_value, t_usage_bits>;

template<typename t_control_value = std::uint32_t,
         unsigned t_usage_bits = detail::type_bits<t_control_value>()>
using ControlNewDataThreadUnsafe = ControlData<CounterThreadUnsafe, t_control_value, t_usage_bits>;


template<typename t_control_value = std::uint64_t,
         unsigned t_usage_bits    = 32u,
         unsigned t_weak_bits     = 16u,
         unsigned t_offset_bits   = 16u,
         unsigned t_size_bits     = 0u,
         unsigned t_align_bits    = 0u>
using ControlAllocDataThreadSafe   =
  ControlData<CounterThreadSafe, t_control_value, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>;

template<typename t_control_value = std::uint64_t,
         unsigned t_usage_bits    = 32u,
         unsigned t_weak_bits     = 16u,
         unsigned t_offset_bits   = 16u,
         unsigned t_size_bits     = 0u,
         unsigned t_align_bits    = 0u>
using ControlAllocDataThreadUnsafe =
  ControlData<CounterThreadUnsafe, t_control_value, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits, t_align_bits>;


template<class t_shared_base,
         class t_thread_safety    = ThreadSafe,
         typename t_control_value = std::uint32_t,
         unsigned t_usage_bits    = detail::type_bits<t_control_value>(),
         typename t_deleter       = std::default_delete<t_shared_base>>
using IntruderNew =
  Intruder<ControlNew<t_shared_base,
                      std::conditional_t<t_thread_safety::value,
                                         ControlNewDataThreadSafe<t_control_value, t_usage_bits>,
                                         ControlNewDataThreadUnsafe<t_control_value, t_usage_bits>>,
                      t_deleter>>;

template<class t_shared_base,
         class t_thread_safety    = ThreadSafe,
         typename t_control_value = std::conditional_t<sizeof(void *) == 8u, std::uint64_t, std::uint32_t>,
         unsigned t_usage_bits    = 32u>
using IntruderNewStatic =
  Intruder<ControlNew<t_shared_base,
                      std::conditional_t<t_thread_safety::value,
                                         ControlNewDataThreadSafe<t_control_value, t_usage_bits>,
                                         ControlNewDataThreadUnsafe<t_control_value, t_usage_bits>>,
                      Deleter<t_shared_base>>>;


template<class t_shared_base,
         class t_thread_safety    = ThreadSafe,
         typename t_control_value = std::uint64_t,
         unsigned t_usage_bits    = 32u,
         unsigned t_weak_bits     = 16u,
         unsigned t_offset_bits   = 16u,
         unsigned t_size_bits     = 0u,
         unsigned t_align_bits    = 0u,
         class t_allocator        = AllocatorMalloc<NoStaticSupport>>
using IntruderAlloc =
  Intruder<ControlAlloc<t_shared_base,
                        std::conditional_t<t_thread_safety::value,
                                           ControlAllocDataThreadSafe<t_control_value, t_usage_bits, t_weak_bits,
                                                                      t_offset_bits, t_size_bits, t_align_bits>,
                                           ControlAllocDataThreadUnsafe<t_control_value, t_usage_bits, t_weak_bits,
                                                                        t_offset_bits, t_size_bits, t_align_bits>>,
                        t_allocator>>;


template<class t_shared_base, class t_thread_safety = ThreadSafe>
using IntruderMallocStatic = IntruderAlloc<t_shared_base, t_thread_safety, std::uint64_t, 32u, 32u,
                                           shared_bits, 0u, 0u, AllocatorMalloc<StaticSupport>>;


template<class t_shared_base, class t_thread_safety = ThreadSafe>
using IntruderStdAllocator = IntruderAlloc<t_shared_base, t_thread_safety, std::uint64_t, 32u, 32u,
                                           shared_bits, 0u, 0u, std::allocator<t_shared_base>>;

// clang-format on


template<class t_shared>
inline std::size_t
calc_pointer_offset(t_shared & p_shared) noexcept
{
  return detail::calc_base_offset(&p_shared);
}

template<class t_shared>
inline constexpr std::size_t
calc_size_offset() noexcept
{
  return detail::calc_base_size_offset<t_shared>();
}

template<class t_shared>
inline constexpr std::size_t
calc_align_offset() noexcept
{
  return detail::calc_base_align_offset<t_shared>();
}


// Check the efficiency of the 'Intruder' and write possible improvements to the given stream.
// Return true if there are no proposed improvements.
template<class t_shared, class t_char, class t_traits>
inline bool
check_intruder_efficiency(SharedPtr<t_shared> const & p_shared_ptr,
                          std::basic_ostream<t_char, t_traits> & p_ostream) noexcept
{
  using SharedBase = detail::BaseType<t_shared>;
  using Control = typename t_shared::PntrControlType;
  using Data = typename Control::Data;
  using DeleterType = typename t_shared::PntrDeleter;
  using Allocator = typename t_shared::PntrAllocator;

  bool efficient = true;
  constexpr std::size_t align = []
  {
    if constexpr (std::is_void_v<DeleterType>)
    {
      return std::max((detail::supports_static<Allocator> ? alignof(void *) : 0u), alignof(Allocator));
    }
    else
    {
      return alignof(DeleterType);
    }
  }();
  if constexpr (sizeof(Data) < align)
  {
    p_ostream << "Padding detected. You can increase the control data value type to " << align << " bytes."
              << std::endl;
    efficient = false;
  }

  if constexpr (Data::s_usage_bits > 32u)
  {
    p_ostream << "Do you need more than 32 bits for the usage reference count?" << std::endl;
    efficient = false;
  }
  if constexpr (t_shared::PntrSupportsWeak::value)
  {
    if constexpr (Data::s_weak_bits > 32u)
    {
      p_ostream << "Do you need more than 32 bits for the weak reference count?" << std::endl;
      efficient = false;
    }
  }
  else
  {
    if constexpr (Data::s_weak_bits != 0u)
    {
      p_ostream << "The weak bits should be zero because weak pointers are not supported." << std::endl;
      efficient = false;
    }
  }

  if constexpr (!std::is_void_v<DeleterType>)
  {
    if constexpr (Data::s_offset_bits != 0u)
    {
      p_ostream << "The offset bits are not needed and should be zero." << std::endl;
      efficient = false;
    }
    if constexpr (Data::s_size_bits != 0u)
    {
      p_ostream << "The size bits are not needed and should be zero." << std::endl;
      efficient = false;
    }
    if constexpr (Data::s_align_bits != 0u)
    {
      p_ostream << "The alignment bits are not needed and should be zero." << std::endl;
      efficient = false;
    }
    if constexpr (std::is_same_v<DeleterType, Deleter<t_shared>>
                  && (std::has_virtual_destructor_v<SharedBase> || std::is_same_v<t_shared, SharedBase>))
    {
      p_ostream << "The 'pntr::Deleter' is not required. Prefer to use 'std::default_delete'" << std::endl;
      efficient = false;
    }
  }

  if constexpr (!std::is_void_v<Allocator>)
  {
    if (p_shared_ptr)
    {
      std::size_t const pointer_offset = calc_pointer_offset(*p_shared_ptr);
      unsigned const offset_bits = detail::bit_width(pointer_offset);
      bool const can_share = (detail::supports_static<Allocator> && offset_bits < Data::s_usage_bits);
      if (can_share)
      {
        if constexpr (Data::s_offset_bits != 0u)
        {
          p_ostream << "It would be more efficient to configure the offset bits as 'pntr::shared_bits'." << std::endl;
          efficient = false;
        }
      }
      else
      {
        if (offset_bits < Data::s_offset_bits)
        {
          p_ostream << "The offset bits of " << Data::s_offset_bits << " can be reduced to " << offset_bits
                    << " to store the pointer offset of " << pointer_offset << '.' << std::endl;
          efficient = false;
        }
      }
    }
    else
    {
      p_ostream << "Unable to check the pointer offset with an empty pointer." << std::endl;
      efficient = false;
    }

    if constexpr (detail::HasTypeInfoDeallocate<Allocator>::value && !detail::supports_static<Allocator>)
    {
      std::size_t const size_offset = calc_size_offset<t_shared>();
      constexpr unsigned size_bits = detail::bit_width(size_offset);
      if constexpr (size_bits < Data::s_size_bits)
      {
        p_ostream << "The size bits of " << Data::s_size_bits << " can be reduced to " << size_bits
                  << " to store the size offset of " << size_offset << '.' << std::endl;
        efficient = false;
      }
      std::size_t const align_offset = calc_align_offset<t_shared>();
      constexpr unsigned align_bits = detail::bit_width(align_offset);
      if constexpr (align_bits < Data::s_align_bits)
      {
        p_ostream << "The alignment bits of " << Data::s_align_bits << " can be reduced to " << align_bits
                  << " to store the alignment offset of " << align_offset << '.' << std::endl;
        efficient = false;
      }
    }
    else
    {
      if constexpr (Data::s_size_bits != 0u)
      {
        p_ostream << "The size bits are not needed and should be zero." << std::endl;
        efficient = false;
      }
      if constexpr (Data::s_align_bits != 0u)
      {
        p_ostream << "The alignment bits are not needed and should be zero." << std::endl;
        efficient = false;
      }
    }
  }

  return efficient;
}


PNTR_NAMESPACE_END
