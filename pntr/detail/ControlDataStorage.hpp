// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/CounterThreadSafe.hpp>
#include <pntr/CounterThreadUnsafe.hpp>
#include <pntr/detail/PntrTypeTraits.hpp>

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
