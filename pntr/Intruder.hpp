// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/common.hpp>

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                            Intruder                                            //
//                                                                                                //
//       An intrusive base class that embeds a control block for 'SharedPtr' and 'WeakPtr'        //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  Requires that the control class type specified as template argument
//  provides the type definitions and member functions as implemented
//  by and documented in the classes 'ControlNew' and 'ControlAlloc'.
//

template<class t_control>
class Intruder
{
public:
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  // The type definitions and member functions required by 'SharedPtr' and 'WeakPtr' start here //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // 'PntrControlType' is the intrusive control class. It can be 'void'.
  using PntrControlType = t_control;

  // 'PntrSharedBase' is the base class specified as template parameter for the control class.
  // It has to be statically (not virtual) derived from the 'Intruder' class.
  using PntrSharedBase = typename t_control::SharedBase;

  // 'PntrUsageValueType' is the integer type used for usage reference counting.
  using PntrUsageValueType = typename t_control::UsageValueType;

  // 'PntrWeakValueType' is the integer type used for weak reference counting.
  using PntrWeakValueType = typename t_control::WeakValueType;

  // 'PntrDeleter' type has to be either 'void' or a supported deleter type.
  using PntrDeleter = typename t_control::Deleter;

  // 'PntrAllocator' type has to be either 'void' or a supported allocator type.
  using PntrAllocator = typename t_control::Allocator;

  // 'PntrSupportsWeak' indicates if weak reference counting is supported.
  // It has to be either 'std::true_type' or 'std::false_type'.
  using PntrSupportsWeak = typename t_control::SupportsWeak;

private:
  // Most functions should be private to prevent the derived class from messing around.

  // Allocate storage for a new instance of the specified type, call its constructor
  // using the forwarded arguments, and return a pointer to it on success.
  template<class t_shared, class t_nothrow, typename... t_args>
  static t_shared *
  pntr_create(t_args &&... p_args) noexcept(t_nothrow::value)
  {
    return t_control::template create<t_shared, t_nothrow>(std::forward<t_args>(p_args)...);
  }

  // Use the allocator to allocate storage for a new instance of the specified type,
  // call its constructor using the forwarded arguments, and assign the allocator
  // and return a pointer to the new instance on success.
  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  static std::enable_if_t<!std::is_void_v<PntrAllocator>, t_shared *>
  pntr_create_with_allocator(t_forward && p_allocator, t_args &&... p_args) noexcept(t_nothrow::value)
  {
    t_shared * shared = t_control::template create<t_shared, t_nothrow>(p_allocator, std::forward<t_args>(p_args)...);
    if (shared != nullptr)
    {
      shared = t_control::template try_init(shared->control(), shared, std::forward<t_forward>(p_allocator));
    }
    return shared;
  }

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. Return the given pointer on success.
  template<class t_shared>
  static t_shared *
  pntr_try_control(t_shared * p_shared) noexcept
  {
    return (p_shared == nullptr ? nullptr : p_shared->control().try_control(*p_shared));
  }

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. Assign the deleter and return the given pointer on success.
  template<class t_shared, class t_forward, typename = std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>>>
  static t_shared *
  pntr_try_control(t_shared * p_shared, t_forward && p_deleter) noexcept
  {
    return (p_shared == nullptr ? nullptr
                                : p_shared->control().try_control(*p_shared, std::forward<t_forward>(p_deleter)));
  }

  // Try to increment the usage counter and return a pointer to the shared object on success.
  template<class t_shared>
  static t_shared *
  pntr_try_get_shared(PntrControlType & p_control) noexcept
  {
    return p_control.template try_get_shared<t_shared>();
  }

  // Return an owner-based pointer for ordering in associative containers.
  // All 'SharedPtr' and 'WeakPtr' who own the same object should return the same pointer.
  // Types which support 'WeakPtr' have to return the address of the control class instance.
  void const *
  pntr_get_owner() const noexcept
  {
    // Check assumption that addresses of 'Intruder' and 'm_control_storage' are identical.
    static_assert(offsetof(Intruder<t_control>, m_control_storage) == 0u);
    static_assert(sizeof(Intruder) == sizeof(t_control));

    return this;
  }

public:
  // Return the current usage count.
  PntrUsageValueType
  pntr_use_count() const noexcept
  {
    return control().use_count();
  }

  // Return the current weak count.
  PntrWeakValueType
  pntr_weak_count() const noexcept
  {
    PntrWeakValueType weak_count{};
    if constexpr (PntrSupportsWeak::value)
    {
      weak_count = control().weak_count();
    }
    return weak_count;
  }

private:
  // Increment the usage counter.
  void
  pntr_add_ref() const noexcept
  {
    control().add_ref();
  }

  // Decrement the usage counter and return true if it reaches zero or was invalid.
  bool
  pntr_release() const noexcept
  {
    return control().release();
  }

  // Delete (non-weak) or destroy (weak) the object. Called when 'pntr_release' returns true.
  // Return a pointer to the control block if it should be deallocated.
  template<class t_shared>
  static PntrControlType *
  pntr_dispose(t_shared * p_shared) noexcept
  {
    return (p_shared == nullptr ? nullptr : t_control::template dispose(p_shared->control(), *p_shared));
  }

  // Deallocate the object. Called when the weak count reaches zero.
  static void
  pntr_deallocate(PntrControlType * p_control) noexcept
  {
    t_control::deallocate(p_control);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //  The type definitions and member functions required by 'SharedPtr' and 'WeakPtr' end here  //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

public:
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //      The type definitions and member functions required by control classes start here      //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // Accept a control instance and return a pointer to the shared base instance.
  static PntrSharedBase *
  get_shared_base(t_control & p_control) noexcept
  {
    return static_cast<PntrSharedBase *>(reinterpret_cast<Intruder *>(&p_control));
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //       The type definitions and member functions required by control classes end here       //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // 'Intruder' is copyable and movable, but the control block is not.

  Intruder(Intruder const &) noexcept
  {
    new (&m_control_storage) t_control;
  }

  Intruder(Intruder &&) noexcept
  {
    new (&m_control_storage) t_control;
  }

  Intruder &
  operator=(Intruder const &) noexcept
  {
    return *this;
  }

  Intruder &
  operator=(Intruder &&) noexcept
  {
    return *this;
  }

  // The function 'shared_from_this' is equivalent to the one in 'std::enable_shared_from_this',
  // but does not have its restriction that it is permitted to be called only from an object that
  // is already managed by a shared pointer. So it is permitted to call 'shared_from_this' from
  // 'Intruder' to acquire control of a shared object, but it can be undefined behaviour if the
  // shared base is not fully initialized yet, for example calling it during the member
  // initialization of a constructor of the shared base.

  SharedPtr<PntrSharedBase>
  shared_from_this() noexcept
  {
    return SharedPtr<PntrSharedBase>(static_cast<PntrSharedBase *>(this));
  }

  SharedPtr<PntrSharedBase const>
  shared_from_this() const noexcept
  {
    return SharedPtr<PntrSharedBase const>(static_cast<PntrSharedBase const *>(this));
  }

  // The function 'weak_from_this' is equivalent to the one in 'std::enable_shared_from_this',
  // but does not have its restriction that it is empty until the object is managed by a shared
  // pointer. So it is permitted to call 'weak_from_this' from 'Intruder' at any time. It can
  // even be used to construct a 'SharedPtr' to acquire control of the shared object, though
  // this can be undefined behaviour if the shared base is not fully initialized yet, for
  // example calling it during the member initialization of a constructor of the shared base.

  template<typename t_weak = PntrSupportsWeak, typename = std::enable_if_t<t_weak::value>>
  WeakPtr<PntrSharedBase>
  weak_from_this() noexcept
  {
    return WeakPtr<PntrSharedBase>(control());
  }

  template<typename t_weak = PntrSupportsWeak, typename = std::enable_if_t<t_weak::value>>
  WeakPtr<PntrSharedBase const>
  weak_from_this() const noexcept
  {
    return WeakPtr<PntrSharedBase const>(control());
  }

  // Re-initialize the usage counter of an expired object. Should only be used if an object was
  // recycled with a custom deleter. It is undefined behaviour to call it in other cases.
  template<class t_deleter = PntrDeleter, typename = std::enable_if_t<!std::is_void_v<t_deleter>>>
  bool
  pntr_try_revive() const noexcept
  {
    return control().try_revive();
  }

  // Return maximum usage count value.
  static constexpr PntrUsageValueType
  pntr_get_max_usage_count() noexcept
  {
    return t_control::get_max_usage_count();
  }

  // Return maximum weak count value.
  static constexpr PntrWeakValueType
  pntr_get_max_weak_count() noexcept
  {
    PntrWeakValueType max_weak_count{};
    if constexpr (PntrSupportsWeak::value)
    {
      max_weak_count = t_control::get_max_weak_count();
    }
    return max_weak_count;
  }

  // 'PntrDataValueType' is the integer type used for the user value.
  using PntrDataValueType = typename t_control::DataValueType;

  // Return maximum user value, see 'ControlData'.
  static constexpr PntrDataValueType
  pntr_get_max_user() noexcept
  {
    return t_control::get_max_user();
  }

  // Return the user value, see 'ControlData'.
  PntrDataValueType
  pntr_get_user() const noexcept
  {
    return control().get_user();
  }

  // Try to set the user value and return true on success, see 'ControlData'.
  bool
  pntr_try_set_user(PntrDataValueType const p_user) const noexcept
  {
    return control().try_set_user(p_user);
  }

protected:
  explicit Intruder(PntrDataValueType const p_user_init = 0u) noexcept
  {
    new (&m_control_storage) t_control(p_user_init);
  }

  ~Intruder() noexcept
  {
    if constexpr (!std::is_void_v<PntrDeleter>)
    {
      control().~t_control();
    }
    else if (control().is_uncontrolled())
    {
      control().~t_control();
    }
  }

private:
  t_control &
  control() const noexcept
  {
    return *std::launder(reinterpret_cast<t_control *>(&m_control_storage));
  }

  alignas(t_control) mutable char m_control_storage[sizeof(t_control)];

  template<class t_shared>
  friend class SharedPtr;
  template<class t_shared>
  friend class WeakPtr;

  template<class t_shared, class t_nothrow, typename... t_args>
  friend SharedPtr<t_shared> detail::make_shared_impl(t_args &&... p_args) noexcept(t_nothrow::value);
  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  friend std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>, SharedPtr<t_shared>>
  detail::make_shared_with_deleter_impl(t_forward && p_deleter, t_args &&... p_args) noexcept(t_nothrow::value);
  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  friend std::enable_if_t<!std::is_void_v<typename t_shared::PntrAllocator>, SharedPtr<t_shared>>
  detail::allocate_shared_impl(t_forward && p_allocator, t_args &&... p_args) noexcept(t_nothrow::value);
};


PNTR_NAMESPACE_END
