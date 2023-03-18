// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/ControlDeleter.hpp>
#include <pntr/detail/PntrTypeTraits.hpp>

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                           ControlNew                                           //
//                                                                                                //
//        A control class for 'Intruder', using 'new' expression, deleter, and usage count        //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  Requires that the shared base class type specified as template argument provides the type
//  definitions and member functions as implemented by and documented in the class 'Intruder'.
//
//  Requires that the data class type specified as template argument provides the type definitions
//  and member functions as implemented by and documented in the class 'ControlData'.
//

template<class t_shared_base, class t_data, typename t_deleter>
class ControlNew
{
public:
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //        The type definitions and member functions required by 'Intruder' start here         //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  using SharedBase = t_shared_base;
  using Data = t_data;
  using Deleter = t_deleter;
  using Allocator = void;
  using UsageValueType = typename t_data::UsageValueType;
  using WeakValueType = typename t_data::WeakValueType;
  using DataValueType = typename t_data::DataValueType;
  using SupportsWeak = std::false_type;

  explicit ControlNew(DataValueType const p_user_init) noexcept
  : m_control_deleter(p_user_init)
  {}

  // Allocate storage for a new instance of the specified type, call its constructor
  // using the forwarded arguments, and return a pointer to it on success.
  template<class t_shared, class t_nothrow, typename... t_args>
  static t_shared *
  create(t_args &&... p_args) noexcept(t_nothrow::value)
  {
    t_shared * shared = nullptr;
    if constexpr (t_nothrow::value)
    {
      try
      {
        // Ignore allocation exceptions
        shared = new (std::nothrow) t_shared(std::forward<t_args>(p_args)...);
      }
      catch (...) // Ignore constructor exceptions
      {}
    }
    else
    {
      shared = new t_shared(std::forward<t_args>(p_args)...);
    }
    return shared;
  }

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. Return a pointer to the given object on success.
  template<class t_shared>
  t_shared *
  try_control(t_shared & p_shared) noexcept
  {
    // Check if the address returned from get_shared_base() is equal
    // to the address of the implicitly upcasted shared instance.
    PNTR_ASSERT(t_shared::get_shared_base(*this) == &p_shared);

    switch (m_control_deleter.m_data.try_control())
    {
      case ControlStatus::e_invalid:
        return nullptr;
      case ControlStatus::e_acquired:
        m_control_deleter.template init<std::remove_const_t<t_shared>>();
        break;
      case ControlStatus::e_shared:
        break;
    }
    return &p_shared;
  }

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. Assign the deleter and return a pointer to the given object on success.
  template<class t_shared, class t_forward>
  t_shared *
  try_control(t_shared & p_shared, t_forward && p_deleter) noexcept
  {
    // Check if the address returned from get_shared_base() is equal
    // to the address of the implicitly upcasted shared instance.
    PNTR_ASSERT(t_shared::get_shared_base(*this) == &p_shared);

    switch (m_control_deleter.m_data.try_control())
    {
      case ControlStatus::e_invalid:
        return nullptr;
      case ControlStatus::e_acquired:
        m_control_deleter.deleter() = std::forward<t_forward>(p_deleter);
        break;
      case ControlStatus::e_shared:
        break;
    }
    return &p_shared;
  }

  // Try to increment the usage counter and return a pointer to the shared object on success.
  template<class t_shared>
  t_shared *
  try_get_shared() noexcept
  {
    if (!m_control_deleter.m_data.try_add_ref())
    {
      return nullptr;
    }
    return detail::static_or_dynamic_cast<t_shared>(t_shared::get_shared_base(*this));
  }

  // Re-initialize the usage counter of an expired object. Return true if it is or was uncontrolled.
  bool
  try_revive() noexcept
  {
    return m_control_deleter.m_data.try_revive();
  }

  // Return maximum usage count value.
  static constexpr UsageValueType
  get_max_usage_count() noexcept
  {
    return t_data::get_max_usage_count();
  }

  // Return the usage count.
  UsageValueType
  use_count() const noexcept
  {
    return m_control_deleter.m_data.use_count();
  }

  // Increment the usage counter.
  void
  add_ref() noexcept
  {
    m_control_deleter.m_data.add_ref();
  }

  // Decrement the usage counter and return true if it reaches zero or was invalid.
  bool
  release() noexcept
  {
    return m_control_deleter.m_data.release();
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
    return m_control_deleter.m_data.get_user();
  }

  // Try to set the user value and return true on success.
  bool
  try_set_user(DataValueType const p_user) noexcept
  {
    return m_control_deleter.m_data.try_set_user(p_user);
  }

  // Delete (non-weak) or destroy (weak) the object. Called when 'pntr_release' returns true.
  // Return a pointer to the control block if it should be deallocated.
  template<class t_shared>
  static typename t_shared::PntrControlType *
  dispose(ControlNew & p_control, t_shared & p_shared) noexcept
  {
    PNTR_TRY_LOG_WARNING(p_control.m_control_deleter.m_data.is_alive(), "disposing object which is still alive");
    ControlDeleter::destroy(p_control.m_control_deleter, &p_shared);
    return nullptr;
  }

  // Deallocate the memory of the shared instance.
  static void
  deallocate([[maybe_unused]] ControlNew * p_control) noexcept
  {
    PNTR_TRY_LOG_ERROR(p_control != nullptr, "should never reveice a valid pointer");
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //         The type definitions and member functions required by 'Intruder' end here          //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

private:
  using ControlDeleter = detail::ControlDeleter<t_shared_base, t_data, t_deleter, detail::is_empty_base<t_deleter>>;

  ControlDeleter m_control_deleter;

  ControlNew(ControlNew const &) = delete;
  ControlNew(ControlNew &&) = delete;
  ControlNew & operator=(ControlNew const &) = delete;
  ControlNew & operator=(ControlNew &&) = delete;
};


PNTR_NAMESPACE_END
