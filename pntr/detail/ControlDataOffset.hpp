// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/ControlDataWeak.hpp>

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
