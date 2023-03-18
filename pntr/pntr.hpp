// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/AllocatorMalloc.hpp>
#include <pntr/AllocatorMemoryResource.hpp>
#include <pntr/ControlAlloc.hpp>
#include <pntr/ControlData.hpp>
#include <pntr/ControlNew.hpp>
#include <pntr/CounterThreadSafe.hpp>
#include <pntr/CounterThreadUnsafe.hpp>
#include <pntr/Deleter.hpp>
#include <pntr/Intruder.hpp>
#include <pntr/SharedPtr.hpp>
#include <pntr/WeakPtr.hpp>

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
