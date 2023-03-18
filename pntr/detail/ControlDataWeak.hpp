// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/ControlDataUsage.hpp>

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
