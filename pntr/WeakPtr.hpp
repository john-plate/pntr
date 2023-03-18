// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/common.hpp>

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                            WeakPtr                                             //
//                                                                                                //
//                      A weak pointer that uses an intrusive control block                       //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  Requires that the specified template type is a class that provides the type definitions
//  and member functions as implemented by and documented in the class 'Intruder'.
//

template<class t_shared>
class WeakPtr
{
  using ControlType = typename t_shared::PntrControlType;

public:
  using element_type = t_shared;

  constexpr WeakPtr() noexcept
  : m_control(nullptr)
  {}

  WeakPtr(WeakPtr const & p_other) noexcept
  : m_control(ControlType::weak_add_ref(p_other.m_control))
  {}

  WeakPtr(WeakPtr && p_other) noexcept
  : m_control(p_other.m_control)
  {
    p_other.m_control = nullptr;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr(WeakPtr<t_other> const & p_other) noexcept
  : m_control(ControlType::weak_add_ref(p_other.m_control))
  {}

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr(WeakPtr<t_other> && p_other) noexcept
  : m_control(p_other.m_control)
  {
    p_other.m_control = nullptr;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr(SharedPtr<t_other> const & p_shared_ptr) noexcept
  : m_control(ControlType::weak_add_ref(p_shared_ptr ? &p_shared_ptr->control() : nullptr))
  {}

  WeakPtr(ControlType & p_control) noexcept
  : m_control(&p_control)
  {
    m_control->weak_add_ref();
  }

  ~WeakPtr() noexcept
  {
    if (m_control != nullptr && m_control->weak_release())
    {
      t_shared::pntr_deallocate(m_control);
    }
  }

  WeakPtr &
  operator=(WeakPtr const & p_other) noexcept
  {
    WeakPtr(p_other).swap(*this);
    return *this;
  }

  WeakPtr &
  operator=(WeakPtr && p_other) noexcept
  {
    WeakPtr(std::move(p_other)).swap(*this);
    return *this;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr &
  operator=(WeakPtr<t_other> const & p_other) noexcept
  {
    WeakPtr(p_other).swap(*this);
    return *this;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr &
  operator=(WeakPtr<t_other> && p_other) noexcept
  {
    WeakPtr(std::move(p_other)).swap(*this);
    return *this;
  }

  template<class t_other, typename = std::enable_if_t<std::is_convertible_v<t_other, t_shared>>>
  WeakPtr &
  operator=(SharedPtr<t_other> const & p_shared_ptr) noexcept
  {
    WeakPtr(p_shared_ptr).swap(*this);
    return *this;
  }

  bool
  is_empty() const noexcept
  {
    return (m_control == nullptr);
  }

  void
  reset() noexcept
  {
    WeakPtr().swap(*this);
  }

  void
  swap(WeakPtr & p_other) noexcept
  {
    std::swap(m_control, p_other.m_control);
  }

  typename ControlType::UsageValueType
  use_count() const noexcept
  {
    return m_control != nullptr ? m_control->use_count() : typename ControlType::UsageValueType{};
  }

  typename ControlType::WeakValueType
  weak_count() const noexcept
  {
    return m_control != nullptr ? m_control->weak_count() : typename ControlType::WeakValueType{};
  }

  bool
  expired() const noexcept
  {
    return (m_control != nullptr && m_control->use_count() == 0u);
  }

  SharedPtr<t_shared>
  lock() const noexcept
  {
    return SharedPtr<t_shared>(m_control != nullptr ? m_control->template try_get_shared<t_shared>() : nullptr, false);
  }

  template<class t_other>
  bool
  owner_before(WeakPtr<t_other> const & p_other) const noexcept
  {
    return get_owner() < p_other.get_owner();
  }

  template<class t_other>
  bool
  owner_before(SharedPtr<t_other> const & p_other) const noexcept
  {
    return get_owner() < (p_other ? p_other->pntr_get_owner() : nullptr);
  }

private:
  void const *
  get_owner() const noexcept
  {
    return m_control;
  }

  ControlType * m_control;

  template<class t_other>
  friend class WeakPtr;

  template<class t_self>
  friend class SharedPtr;
};


template<class t_shared>
void
swap(WeakPtr<t_shared> & p_lhs, WeakPtr<t_shared> & p_rhs) noexcept
{
  p_lhs.swap(p_rhs);
}


PNTR_NAMESPACE_END
