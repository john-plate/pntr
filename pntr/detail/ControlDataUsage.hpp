// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/ControlDataStorage.hpp>

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
