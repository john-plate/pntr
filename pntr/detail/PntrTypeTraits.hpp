// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/common.hpp>

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
