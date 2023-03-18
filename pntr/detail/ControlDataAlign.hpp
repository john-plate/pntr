// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/ControlDataSize.hpp>

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
