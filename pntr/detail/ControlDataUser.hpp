// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/ControlDataAlign.hpp>

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
