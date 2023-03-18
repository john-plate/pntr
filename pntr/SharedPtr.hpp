// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/PntrTypeTraits.hpp>

PNTR_NAMESPACE_BEGIN

template<class t_shared, class t_other>
SharedPtr<t_shared> static_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
template<class t_shared, class t_other>
SharedPtr<t_shared> static_pointer_cast(SharedPtr<t_other> && p_other) noexcept;
template<class t_shared, class t_other>
SharedPtr<t_shared> dynamic_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
template<class t_shared, class t_other>
SharedPtr<t_shared> dynamic_pointer_cast(SharedPtr<t_other> && p_other) noexcept;
template<class t_shared, class t_other>
SharedPtr<t_shared> const_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
template<class t_shared, class t_other>
SharedPtr<t_shared> const_pointer_cast(SharedPtr<t_other> && p_other) noexcept;


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                           SharedPtr                                            //
//                                                                                                //
//                      A smart pointer that uses an intrusive control block                      //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  Requires that the specified template type is a class that provides the type definitions
//  and member functions as implemented by and documented in the class 'Intruder'.
//
//  It is recommended to inherit the specified class from the class 'Intruder' and to overload
//  selected functions if required.
//
//  Custom reference counting implementations have the benefit that existing reference counters
//  can be used, but it might not be compatible with existing intrusive weak counters.
//
//  The undocumented functions are equivalent to 'std::shared_ptr'. Its aliasing feature is not
//  supported by design, as it would require to store an additional pointer.
//

template<class t_shared>
class SharedPtr
{
public:
  using element_type = t_shared;
  using weak_type = std::conditional_t<t_shared::PntrSupportsWeak::value, WeakPtr<t_shared>, void>;

  constexpr SharedPtr() noexcept
  : m_shared(nullptr)
  {}

  constexpr SharedPtr(std::nullptr_t) noexcept
  : m_shared(nullptr)
  {}

  // Try to take control of the given object, either by taking new or by sharing the existing
  // ownership. On failure the contructed 'SharedPtr' will be empty.
  // It is the responsibility of the user to allocate the object in a way that is compatible
  // to the deallocation used by the control block.
  // It is recommended to use make_shared, make_shared_with_deleter, or allocate_shared to let
  // the control block allocate the object.
  template<class t_other>
  SharedPtr(t_other * const p_shared) noexcept
  : m_shared(t_shared::template pntr_try_control(p_shared))
  {}

  // Same as above, but in addition accept a deleter object or function that is convertible
  // and will be assigned to the deleter stored in the control block.
  template<class t_other, class t_forward, typename = std::enable_if_t<!std::is_void_v<typename t_other::PntrDeleter>>>
  SharedPtr(t_other * p_shared, t_forward && p_deleter) noexcept
  : m_shared(t_shared::template pntr_try_control(p_shared, std::forward<t_forward>(p_deleter)))
  {}

  // Same as above, but accept a 'std::unique_ptr' with deleter object or function that is
  // convertible and will be assigned to the deleter stored in the control block.
  template<class t_other, class t_deleter,
           typename = std::enable_if_t<std::is_convertible_v<t_deleter, typename t_other::PntrDeleter>>>
  SharedPtr(std::unique_ptr<t_other, t_deleter> && p_unique) noexcept
  : m_shared(t_shared::template pntr_try_control(p_unique.release(), std::move(p_unique.get_deleter())))
  {}

  // Same as above, but accept a 'std::unique_ptr' with a const object if the deleter is a templated
  // empty class that can be default constructed and saved in the control block.
  template<class t_other, template<class> class t_deleter,
           typename = std::enable_if_t<
             std::is_convertible_v<t_deleter<std::remove_const_t<t_other>>, typename t_other::PntrDeleter>
             && std::is_nothrow_default_constructible_v<t_deleter<std::remove_const_t<t_other>>>
             && detail::is_empty_base<t_deleter<t_other>>>>
  SharedPtr(std::unique_ptr<t_other, t_deleter<t_other>> && p_unique) noexcept
  : m_shared(t_shared::template pntr_try_control(p_unique.release(), t_deleter<std::remove_const_t<t_other>>()))
  {}

  SharedPtr(SharedPtr const & p_other) noexcept
  : m_shared(p_other.m_shared)
  {
    if (m_shared != nullptr)
    {
      m_shared->pntr_add_ref();
    }
  }

  SharedPtr(SharedPtr && p_other) noexcept
  : m_shared(p_other.m_shared)
  {
    p_other.m_shared = nullptr;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  SharedPtr(SharedPtr<t_other> const & p_other) noexcept
  : m_shared(p_other.m_shared)
  {
    if (m_shared != nullptr)
    {
      m_shared->pntr_add_ref();
    }
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  SharedPtr(SharedPtr<t_other> && p_other) noexcept
  : m_shared(p_other.m_shared)
  {
    p_other.m_shared = nullptr;
  }

  // Throws 'std::bad_weak_ptr' if the weak pointer is empty.
  // The non-throwing alternative is to create the shared pointer with 'WeakPtr::lock()'.
  template<class t_other, typename = std::enable_if_t<t_other::PntrSupportsWeak::value>>
  explicit SharedPtr(WeakPtr<t_other> const & p_weak)
  : m_shared(nullptr)
  {
    if (p_weak.m_control != nullptr)
    {
      m_shared = t_shared::template pntr_try_get_shared<t_other>(*p_weak.m_control);
    }
    else
    {
      throw std::bad_weak_ptr();
    }
  }

  ~SharedPtr() noexcept
  {
    if (m_shared != nullptr && m_shared->pntr_release())
    {
      t_shared::pntr_deallocate(t_shared::template pntr_dispose(const_cast<std::remove_const_t<t_shared> *>(m_shared)));
    }
  }

  SharedPtr &
  operator=(SharedPtr const & p_other) noexcept
  {
    SharedPtr(p_other).swap(*this);
    return *this;
  }

  SharedPtr &
  operator=(SharedPtr && p_other) noexcept
  {
    SharedPtr(std::move(p_other)).swap(*this);
    return *this;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  SharedPtr &
  operator=(SharedPtr<t_other> const & p_other) noexcept
  {
    SharedPtr(p_other).swap(*this);
    return *this;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  SharedPtr &
  operator=(SharedPtr<t_other> && p_other) noexcept
  {
    SharedPtr(std::move(p_other)).swap(*this);
    return *this;
  }

  explicit operator bool() const noexcept
  {
    return m_shared != nullptr;
  }

  t_shared *
  get() const noexcept
  {
    return m_shared;
  }

  t_shared &
  operator*() const noexcept
  {
    PNTR_ASSERT(m_shared != nullptr);
    return *m_shared;
  }

  t_shared *
  operator->() const noexcept
  {
    PNTR_ASSERT(m_shared != nullptr);
    return m_shared;
  }

  typename t_shared::PntrUsageValueType
  use_count() const noexcept
  {
    return m_shared != nullptr ? m_shared->pntr_use_count() : typename t_shared::PntrUsageValueType{};
  }

  typename t_shared::PntrWeakValueType
  weak_count() const noexcept
  {
    return m_shared != nullptr ? m_shared->pntr_weak_count() : typename t_shared::PntrWeakValueType{};
  }

  template<class t_other>
  bool
  owner_before(SharedPtr<t_other> const & p_other) const noexcept
  {
    return (m_shared != nullptr ? m_shared->pntr_get_owner() : nullptr)
         < (p_other.m_shared != nullptr ? p_other.m_shared->pntr_get_owner() : nullptr);
  }

  template<class t_other, typename = std::enable_if_t<t_other::PntrSupportsWeak::value>>
  bool
  owner_before(WeakPtr<t_other> const & p_other) const noexcept
  {
    return (m_shared != nullptr ? m_shared->pntr_get_owner() : nullptr) < p_other.get_owner();
  }

  void
  reset() noexcept
  {
    SharedPtr().swap(*this);
  }

  template<class t_other>
  void
  reset(t_other * const p_shared) noexcept
  {
    SharedPtr(p_shared).swap(*this);
  }

  template<class t_other, class t_forward, typename = std::enable_if_t<!std::is_void_v<typename t_other::PntrDeleter>>>
  void
  reset(t_other * p_shared, t_forward && p_deleter) noexcept
  {
    SharedPtr(p_shared, std::forward<t_forward>(p_deleter)).swap(*this);
  }

  template<class t_other, class t_deleter,
           typename = std::enable_if_t<std::is_convertible_v<t_deleter, typename t_other::PntrDeleter>>>
  void
  reset(std::unique_ptr<t_other, t_deleter> && p_unique) noexcept
  {
    SharedPtr(std::move(p_unique)).swap(*this);
  }

  template<class t_other, template<class> class t_deleter,
           typename = std::enable_if_t<
             std::is_convertible_v<t_deleter<std::remove_const_t<t_other>>, typename t_other::PntrDeleter>
             && std::is_nothrow_default_constructible_v<t_deleter<std::remove_const_t<t_other>>>
             && detail::is_empty_base<t_deleter<t_other>>>>
  void
  reset(std::unique_ptr<t_other, t_deleter<t_other>> && p_unique) noexcept
  {
    SharedPtr(std::move(p_unique)).swap(*this);
  }

  void
  swap(SharedPtr & p_other) noexcept
  {
    std::swap(m_shared, p_other.m_shared);
  }

private:
  // Only called from 'make_shared', 'make_shared_with_deleter', pointer casts, and 'WeakPtr::lock'.
  SharedPtr(t_shared * const p_shared, bool p_add_ref) noexcept
  : m_shared(p_shared)
  {
    if (m_shared != nullptr && p_add_ref)
    {
      m_shared->pntr_add_ref();
    }
  }

  // Only called from pointer casts with move semantics
  t_shared *
  detach() noexcept
  {
    t_shared * const shared = m_shared;
    m_shared = nullptr;
    return shared;
  }

  t_shared * m_shared;

  template<class t_other>
  friend class SharedPtr;
  friend class WeakPtr<t_shared>;

  template<class t_self, class t_nothrow, typename... t_args>
  friend SharedPtr<t_self> detail::make_shared_impl(t_args &&... p_args) noexcept(t_nothrow::value);
  template<class t_self, class t_nothrow, class t_forward, typename... t_args>
  friend std::enable_if_t<!std::is_void_v<typename t_self::PntrDeleter>, SharedPtr<t_self>>
  detail::make_shared_with_deleter_impl(t_forward && p_deleter, t_args &&... p_args) noexcept(t_nothrow::value);
  template<class t_self, class t_nothrow, class t_forward, typename... t_args>
  friend std::enable_if_t<!std::is_void_v<typename t_self::PntrAllocator>, SharedPtr<t_self>>
  detail::allocate_shared_impl(t_forward && p_allocator, t_args &&... p_args) noexcept(t_nothrow::value);

  template<class t_self, class t_other>
  friend SharedPtr<t_self> static_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
  template<class t_self, class t_other>
  friend SharedPtr<t_self> static_pointer_cast(SharedPtr<t_other> && p_other) noexcept;
  template<class t_self, class t_other>
  friend SharedPtr<t_self> dynamic_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
  template<class t_self, class t_other>
  friend SharedPtr<t_self> dynamic_pointer_cast(SharedPtr<t_other> && p_other) noexcept;
  template<class t_self, class t_other>
  friend SharedPtr<t_self> const_pointer_cast(SharedPtr<t_other> const & p_other) noexcept;
  template<class t_self, class t_other>
  friend SharedPtr<t_self> const_pointer_cast(SharedPtr<t_other> && p_other) noexcept;
};


// clang-format off

template<class L, class R> inline bool operator==(SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() == r.get(); }
template<class L, class R> inline bool operator!=(SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() != r.get(); }
template<class L, class R> inline bool operator< (SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() <  r.get(); }
template<class L, class R> inline bool operator> (SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() >  r.get(); }
template<class L, class R> inline bool operator<=(SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() <= r.get(); }
template<class L, class R> inline bool operator>=(SharedPtr<L> const & l, SharedPtr<R> const & r) noexcept { return l.get() >= r.get(); }

template<class T> inline bool operator==(SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() == static_cast<T *>(nullptr); }
template<class T> inline bool operator!=(SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() != static_cast<T *>(nullptr); }
template<class T> inline bool operator< (SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() <  static_cast<T *>(nullptr); }
template<class T> inline bool operator> (SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() >  static_cast<T *>(nullptr); }
template<class T> inline bool operator<=(SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() <= static_cast<T *>(nullptr); }
template<class T> inline bool operator>=(SharedPtr<T> const & l, std::nullptr_t) noexcept { return l.get() >= static_cast<T *>(nullptr); }

template<class T> inline bool operator==(std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) == r.get(); }
template<class T> inline bool operator!=(std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) != r.get(); }
template<class T> inline bool operator< (std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) <  r.get(); }
template<class T> inline bool operator> (std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) >  r.get(); }
template<class T> inline bool operator<=(std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) <= r.get(); }
template<class T> inline bool operator>=(std::nullptr_t, SharedPtr<T> const & r) noexcept { return static_cast<T *>(nullptr) >= r.get(); }

// clang-format on


namespace detail
{
  template<class t_shared, class t_nothrow, typename... t_args>
  SharedPtr<t_shared>
  make_shared_impl(t_args &&... p_args) noexcept(t_nothrow::value)
  {
    using Shared = std::remove_const_t<t_shared>;
    using Allocator = typename Shared::PntrAllocator;
    Shared * created = nullptr;
    if constexpr (std::is_void_v<Allocator>)
    {
      created = Shared::template pntr_create<Shared, t_nothrow>(std::forward<t_args>(p_args)...);
    }
    else
    {
      created =
        Shared::template pntr_create_with_allocator<Shared, t_nothrow>(Allocator(), std::forward<t_args>(p_args)...);
    }
    Shared * const shared = Shared::template pntr_try_control(created);
    if (shared == nullptr)
    {
      Shared::pntr_deallocate(Shared::template pntr_dispose(created));
    }
    return SharedPtr<t_shared>(const_cast<t_shared *>(shared), false);
  }

  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>, SharedPtr<t_shared>>
  make_shared_with_deleter_impl(t_forward && p_deleter, t_args &&... p_args) noexcept(t_nothrow::value)
  {
    using Shared = std::remove_const_t<t_shared>;
    Shared * const created = Shared::template pntr_create<Shared, t_nothrow>(std::forward<t_args>(p_args)...);
    Shared * const shared = Shared::template pntr_try_control(created, std::forward<t_forward>(p_deleter));
    if (shared == nullptr)
    {
      Shared::pntr_deallocate(Shared::template pntr_dispose(created));
    }
    return SharedPtr<t_shared>(const_cast<t_shared *>(shared), false);
  }

  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  std::enable_if_t<!std::is_void_v<typename t_shared::PntrAllocator>, SharedPtr<t_shared>>
  allocate_shared_impl(t_forward && p_allocator, t_args &&... p_args) noexcept(t_nothrow::value)
  {
    using Shared = std::remove_const_t<t_shared>;
    Shared * const created =
      Shared::template pntr_create_with_allocator<Shared, t_nothrow>(std::forward<t_forward>(p_allocator),
                                                                     std::forward<t_args>(p_args)...);
    Shared * const shared = Shared::template pntr_try_control(created);
    if (shared == nullptr)
    {
      Shared::pntr_deallocate(Shared::template pntr_dispose(created));
    }
    return SharedPtr<t_shared>(const_cast<t_shared *>(shared), false);
  }
} // namespace detail


// May throw allocation and constructor exceptions.
template<class t_shared, typename... t_args>
SharedPtr<t_shared>
make_shared(t_args &&... p_args)
{
  return detail::make_shared_impl<t_shared, std::false_type>(std::forward<t_args>(p_args)...);
}

// Doesn't throw. The returned 'SharedPtr' is empty if the allocation or construction fails.
template<class t_shared, typename... t_args>
SharedPtr<t_shared>
make_shared_nothrow(t_args &&... p_args) noexcept
{
  return detail::make_shared_impl<t_shared, std::true_type>(std::forward<t_args>(p_args)...);
}

// May throw allocation and constructor exceptions.
template<class t_shared, class t_forward, typename... t_args>
std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>, SharedPtr<t_shared>>
make_shared_with_deleter(t_forward && p_deleter, t_args &&... p_args)
{
  return detail::make_shared_with_deleter_impl<t_shared, std::false_type>(std::forward<t_forward>(p_deleter),
                                                                          std::forward<t_args>(p_args)...);
}

// Doesn't throw. The returned 'SharedPtr' is empty if the allocation or construction fails.
template<class t_shared, class t_forward, typename... t_args>
std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>, SharedPtr<t_shared>>
make_shared_with_deleter_nothrow(t_forward && p_deleter, t_args &&... p_args) noexcept
{
  return detail::make_shared_with_deleter_impl<t_shared, std::true_type>(std::forward<t_forward>(p_deleter),
                                                                         std::forward<t_args>(p_args)...);
}

// May throw allocation and constructor exceptions.
template<class t_shared, class t_forward, typename... t_args>
std::enable_if_t<!std::is_void_v<typename t_shared::PntrAllocator>, SharedPtr<t_shared>>
allocate_shared(t_forward && p_allocator, t_args &&... p_args)
{
  return detail::allocate_shared_impl<t_shared, std::false_type>(std::forward<t_forward>(p_allocator),
                                                                 std::forward<t_args>(p_args)...);
}

// Doesn't throw. The returned 'SharedPtr' is empty if the allocation or construction fails.
template<class t_shared, class t_forward, typename... t_args>
std::enable_if_t<!std::is_void_v<typename t_shared::PntrAllocator>, SharedPtr<t_shared>>
allocate_shared_nothrow(t_forward && p_allocator, t_args &&... p_args) noexcept
{
  return detail::allocate_shared_impl<t_shared, std::true_type>(std::forward<t_forward>(p_allocator),
                                                                std::forward<t_args>(p_args)...);
}


template<class t_shared, class t_other>
SharedPtr<t_shared>
static_pointer_cast(SharedPtr<t_other> const & p_other) noexcept
{
  return SharedPtr<t_shared>(static_cast<t_shared *>(p_other.get()), true);
}

template<class t_shared, class t_other>
SharedPtr<t_shared>
static_pointer_cast(SharedPtr<t_other> && p_other) noexcept
{
  return SharedPtr<t_shared>(static_cast<t_shared *>(p_other.detach()), false);
}

template<class t_shared, class t_other>
SharedPtr<t_shared>
dynamic_pointer_cast(SharedPtr<t_other> const & p_other) noexcept
{
  return SharedPtr<t_shared>(dynamic_cast<t_shared *>(p_other.get()), true);
}

template<class t_shared, class t_other>
SharedPtr<t_shared>
dynamic_pointer_cast(SharedPtr<t_other> && p_other) noexcept
{
  t_shared * const shared = dynamic_cast<t_shared *>(p_other.get());
  if (shared != nullptr)
  {
    p_other.detach();
  }
  return SharedPtr<t_shared>(shared, false);
}

template<class t_shared, class t_other>
SharedPtr<t_shared>
const_pointer_cast(SharedPtr<t_other> const & p_other) noexcept
{
  return SharedPtr<t_shared>(const_cast<t_shared *>(p_other.get()), true);
}

template<class t_shared, class t_other>
SharedPtr<t_shared>
const_pointer_cast(SharedPtr<t_other> && p_other) noexcept
{
  return SharedPtr<t_shared>(const_cast<t_shared *>(p_other.detach()), false);
}


template<class t_shared>
void
swap(SharedPtr<t_shared> & p_lhs, SharedPtr<t_shared> & p_rhs) noexcept
{
  p_lhs.swap(p_rhs);
}

template<class t_char, class t_traits, class t_shared>
std::basic_ostream<t_char, t_traits> &
operator<<(std::basic_ostream<t_char, t_traits> & p_ostream, SharedPtr<t_shared> const & p_shared) noexcept
{
  return p_ostream << p_shared.get();
}


PNTR_NAMESPACE_END


namespace std
{
  template<class t_shared>
  struct hash<::PNTR_NAMESPACE::SharedPtr<t_shared>>
  {
    std::size_t
    operator()(::PNTR_NAMESPACE::SharedPtr<t_shared> const & p_shared) const noexcept
    {
      return std::hash<t_shared *>()(p_shared.get());
    }
  };
}
