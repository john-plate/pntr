// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/ControlDataUser.hpp>

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
