// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/AllocAdaptPointer.hpp>
#include <pntr/detail/AllocAdaptTypeInfo.hpp>
#include <pntr/detail/AllocAdaptTyped.hpp>
#include <pntr/detail/PntrTypeTraits.hpp>

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                          ControlAlloc                                          //
//                                                                                                //
//          A control class for 'Intruder', using an allocator, and usage and weak count          //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  'ControlAlloc' supports three types of allocators:
//  - Allocators which only require the original untyped pointer for deallocation. The most common
//    example is 'std::free'. Those allocators are required to provide the type definitions and
//    member functions as implemented by and documented in the class 'AllocatorMalloc'.
//  - Allocators which require the original untyped pointer and the size and alignment of the
//    allocated memory block for deallocation, for example 'std::pmr::memory_resource'. Those
//    allocators are required to provide the type definitions and member functions as implemented
//    by and documented in the class 'AllocatorMemoryResource'.
//  - Allocators which require the original type for deallocation, for example 'std::allocator'.
//    Those allocators are required to provide the type definitions and member functions as
//    documented in the C++ Standard Library, see
//    https://en.cppreference.com/w/cpp/named_req/Allocator
//
//  Requires that the shared base class type specified as template argument provides the
//  type definitions and member functions as implemented by and documented in the class 'Intruder'.
//
//  Requires that the data class type specified as template argument provides the type definitions
//  and member functions as implemented by and documented in the class 'ControlData'.
//

template<class t_shared_base, class t_data, class t_allocator>
class ControlAlloc
{
public:
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //        The type definitions and member functions required by 'Intruder' start here         //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  using SharedBase = t_shared_base;
  using Data = t_data;
  using Deleter = void;
  using Allocator = t_allocator;
  using UsageValueType = typename t_data::UsageValueType;
  using WeakValueType = typename t_data::WeakValueType;
  using DataValueType = typename t_data::DataValueType;
  using SupportsWeak = typename t_data::SupportsWeak;

  explicit ControlAlloc(DataValueType const p_user_init) noexcept
  : m_adapter(p_user_init)
  {}

  // Allocate storage for a new instance of the specified type, call its constructor
  // using the forwarded arguments, and return a pointer to it on success.
  template<class t_shared, class t_nothrow, class t_shared_allocator, typename... t_args>
  static t_shared *
  create(t_shared_allocator & p_allocator, t_args &&... p_args) noexcept(t_nothrow::value)
  {
    return AllocAdapter::template create<t_shared, t_nothrow>(p_allocator, std::forward<t_args>(p_args)...);
  }

  // Try to initialize the control pointer object and to forward the allocator to it.
  template<class t_shared, class t_forward>
  static t_shared *
  try_init(ControlAlloc & p_self, t_shared * p_shared, t_forward && p_allocator) noexcept
  {
    return AllocAdapter::template try_init(p_self.m_adapter, p_shared, std::forward<t_forward>(p_allocator));
  }

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. Return a pointer to the given object on success.
  template<class t_shared>
  t_shared *
  try_control(t_shared & p_shared) noexcept
  {
    // Check if the address returned from get_shared_base() is equal
    // to the address of the implicitly upcasted shared instance.
    if (t_shared::get_shared_base(*this) != &p_shared)
    {
      return nullptr;
    }
    return (m_adapter.m_data.try_control() != ControlStatus::e_invalid ? &p_shared : nullptr);
  }

  // Try to increment the usage counter and return a pointer to the shared object on success.
  template<class t_shared>
  t_shared *
  try_get_shared() noexcept
  {
    if (!m_adapter.m_data.try_add_ref())
    {
      return nullptr;
    }
    return detail::static_or_dynamic_cast<t_shared>(t_shared::get_shared_base(*this));
  }

  // Return true if this is not controlled (yet).
  bool
  is_uncontrolled() const noexcept
  {
    return m_adapter.m_data.is_uncontrolled();
  }

  // Return maximum usage count value.
  static constexpr UsageValueType
  get_max_usage_count() noexcept
  {
    return t_data::get_max_usage_count();
  }

  // Return maximum weak count value.
  static constexpr WeakValueType
  get_max_weak_count() noexcept
  {
    return t_data::get_max_weak_count();
  }

  // Return the usage count.
  UsageValueType
  use_count() const noexcept
  {
    return m_adapter.m_data.use_count();
  }

  // Return the weak count.
  template<typename Weak = SupportsWeak, typename = std::enable_if_t<Weak::value>>
  WeakValueType
  weak_count() const noexcept
  {
    return m_adapter.m_data.weak_count();
  }

  // Increment the usage counter.
  void
  add_ref() noexcept
  {
    m_adapter.m_data.add_ref();
  }

  // Increment the weak counter.
  template<typename Weak = SupportsWeak, typename = std::enable_if_t<Weak::value>>
  void
  weak_add_ref() noexcept
  {
    m_adapter.m_data.weak_add_ref();
  }

  // Increment the weak counter.
  template<typename Weak = SupportsWeak, typename = std::enable_if_t<Weak::value>>
  static ControlAlloc *
  weak_add_ref(ControlAlloc * p_control) noexcept
  {
    if (p_control != nullptr)
    {
      p_control->m_adapter.m_data.weak_add_ref();
    }
    return p_control;
  }

  // Decrement the usage counter and return true if it reaches zero or was invalid.
  bool
  release() noexcept
  {
    return m_adapter.m_data.release();
  }

  // Decrement the weak counter and return true if it reaches or was zero.
  template<typename Weak = SupportsWeak, typename = std::enable_if_t<Weak::value>>
  bool
  weak_release() noexcept
  {
    return m_adapter.m_data.weak_release();
  }

  // Return maximum user value.
  static constexpr DataValueType
  get_max_user() noexcept
  {
    return t_data::get_max_user();
  }

  // Return the user value.
  DataValueType
  get_user() const noexcept
  {
    return m_adapter.m_data.get_user();
  }

  // Try to set the user value and return true on success.
  bool
  try_set_user(DataValueType const p_user) noexcept
  {
    return m_adapter.m_data.try_set_user(p_user);
  }

  // Delete (non-weak) or destroy (weak) the object. Called when 'pntr_release' returns true.
  // Return a pointer to the control block if it should be deallocated.
  template<class t_shared>
  static typename t_shared::PntrControlType *
  dispose(ControlAlloc & p_control, t_shared & p_shared) noexcept
  {
    PNTR_TRY_LOG_WARNING(p_control.m_adapter.m_data.is_alive(), "disposing object which is still alive");
    p_control.m_adapter.destroy(p_control, p_shared);
    ControlAlloc * control = &p_control;
    if constexpr (SupportsWeak::value)
    {
      if (!p_control.m_adapter.m_data.weak_release())
      {
        control = nullptr;
      }
    }
    return control;
  }

  // Deallocate the memory of the shared instance.
  static void
  deallocate(ControlAlloc * p_control) noexcept
  {
    if (p_control != nullptr)
    {
      AllocAdapter::deallocate(p_control->m_adapter, *p_control);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //         The type definitions and member functions required by 'Intruder' end here          //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

private:
  using AllocAdapter =
    std::conditional_t<detail::HasPointerDeallocate<t_allocator>::value,
                       detail::AllocAdaptPointer<t_shared_base, t_data, t_allocator>,
                       std::conditional_t<detail::HasTypeInfoDeallocate<t_allocator>::value,
                                          detail::AllocAdaptTypeInfo<t_shared_base, t_data, t_allocator>,
                                          detail::AllocAdaptTyped<t_shared_base, t_data, t_allocator>>>;

  AllocAdapter m_adapter;

  ControlAlloc(ControlAlloc const &) = delete;
  ControlAlloc(ControlAlloc &&) = delete;
  ControlAlloc & operator=(ControlAlloc const &) = delete;
  ControlAlloc & operator=(ControlAlloc &&) = delete;
};


PNTR_NAMESPACE_END
