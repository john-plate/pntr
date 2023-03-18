// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/PntrTypeTraits.hpp>

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                            Deleter                                             //
//                                                                                                //
//                 A deleter for 'ControlNew' which saves the shared object type                  //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  During construction this deleter saves a pointer to a function that casts the shared object
//  pointer to the original type before it is deleted. This makes it possible to use 'SharedPtr'
//  with base classes of non-polymorphic class hierarchies.
//
//  It requires the size of a pointer in the control block, so it is more efficient not to use it
//  with polymorphic types where it is safe to call the virtual destructor of the base class.
//

template<typename t_shared>
struct Deleter
{
  using SharedBase = std::remove_const_t<detail::BaseType<t_shared>>;

  Deleter & operator=(Deleter &&) = default;
  Deleter & operator=(Deleter const &) = default;

  template<typename t_other,
           typename = std::enable_if_t<std::is_convertible_v<std::remove_const_t<t_other> *, t_shared *>>>
  Deleter(Deleter<t_other> const & p_other) noexcept
  : m_delete_function(p_other.m_delete_function)
#ifndef PNTR_UNITTESTS
  {}

  constexpr Deleter() noexcept = default;
  Deleter(Deleter const &) noexcept = default;
  Deleter(Deleter &&) noexcept = default;

#else
  {
    ++g_deleter_construct;
  }

  Deleter() noexcept
  {
    ++g_deleter_construct;
  }

  Deleter(Deleter const & p_deleter) noexcept
  : m_delete_function(p_deleter.m_delete_function)
  {
    ++g_deleter_construct;
  }

  Deleter(Deleter && p_deleter) noexcept
  : m_delete_function(p_deleter.m_delete_function)
  {
    ++g_deleter_construct;
  }

  ~Deleter() noexcept
  {
    ++g_deleter_destroy;
  }

#endif

  void
  operator()(t_shared * p_shared) const noexcept
  {
    static_assert(sizeof(t_shared) != 0u, "incomplete type");
    m_delete_function(p_shared);
  }

private:
  static void
  delete_shared(SharedBase const * p_base) noexcept
  {
    delete detail::static_or_dynamic_cast<std::add_const_t<t_shared>>(p_base);
  }

  using DeleteFunction = void (*)(SharedBase const *) noexcept;
  DeleteFunction m_delete_function = &delete_shared;

  template<typename t_other>
  friend struct Deleter;
};


PNTR_NAMESPACE_END
