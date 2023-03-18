// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/ControlDataOffset.hpp>

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
