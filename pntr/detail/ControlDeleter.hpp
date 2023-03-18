// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/common.hpp>

PNTR_NAMESPACE_BEGIN


namespace detail
{
  // ControlDeleter stores control data and a deleter with Empty Base Optimization.
  template<class t_shared_base, class t_data, typename t_deleter, bool t_is_empty>
  class ControlDeleter;

  // ControlDeleter with templated empty-class deleter
  template<class t_shared_base, class t_data, template<class> class t_deleter>
  class ControlDeleter<t_shared_base, t_data, t_deleter<t_shared_base>, true>: public t_deleter<t_shared_base>
  {
  public:
    explicit ControlDeleter(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    t_deleter<t_shared_base> &
    deleter() noexcept
    {
      return *this;
    }

    template<class t_shared>
    void
    init() noexcept
    {}

    template<class t_shared>
    static void
    destroy(ControlDeleter &, t_shared * p_shared) noexcept
    {
      t_deleter<t_shared>()(p_shared);
    }

    t_data m_data;
  };

  // ControlDeleter with empty-class deleter
  template<class t_shared_base, class t_data, typename t_deleter>
  class ControlDeleter<t_shared_base, t_data, t_deleter, true>: public t_deleter
  {
  public:
    explicit ControlDeleter(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    t_deleter &
    deleter() noexcept
    {
      return *this;
    }

    template<class t_shared>
    void
    init() noexcept
    {}

    template<class t_shared>
    static void
    destroy(ControlDeleter &, t_shared * p_shared) noexcept
    {
      t_deleter()(p_shared);
    }

    t_data m_data;
  };

  // ControlDeleter with templated non-empty-class deleter
  template<class t_shared_base, class t_data, template<class> class t_deleter>
  class ControlDeleter<t_shared_base, t_data, t_deleter<t_shared_base>, false>
  {
  public:
    explicit ControlDeleter(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    t_deleter<t_shared_base> &
    deleter() noexcept
    {
      return m_deleter;
    }

    template<class t_shared>
    void
    init() noexcept
    {
      m_deleter = t_deleter<t_shared>();
    }

    template<class t_shared>
    static void
    destroy(ControlDeleter & p_self, t_shared * p_shared) noexcept
    {
      t_deleter<t_shared_base> const deleter(std::move(p_self.m_deleter));
      deleter(p_shared);
    }

    t_data m_data;
    t_deleter<t_shared_base> m_deleter;
  };

  // ControlDeleter with non-empty-class deleter
  template<class t_shared_base, class t_data, typename t_deleter>
  class ControlDeleter<t_shared_base, t_data, t_deleter, false>
  {
  public:
    explicit ControlDeleter(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    t_deleter &
    deleter() noexcept
    {
      return m_deleter;
    }

    template<class t_shared>
    void
    init() noexcept
    {}

    template<class t_shared>
    static void
    destroy(ControlDeleter & p_self, t_shared * p_shared) noexcept
    {
      t_deleter const deleter(std::move(p_self.m_deleter));
      deleter(p_shared);
    }

    t_data m_data;
    t_deleter m_deleter;
  };
} // namespace detail


PNTR_NAMESPACE_END
