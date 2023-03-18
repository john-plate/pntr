// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/PntrTypeTraits.hpp>

PNTR_NAMESPACE_BEGIN


template<class t_shared_base, class t_data, class t_allocator>
class ControlAlloc;

namespace detail
{
  template<class t_shared_base, class t_data, class t_allocator, typename = void>
  class AllocAdaptBase;

  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptData
  {
  public:
    enum class Mode
    {
      e_destroy,
      e_deallocate
    };

    using Control = ControlAlloc<t_shared_base, t_data, t_allocator>;
    using Function = void (*)(AllocAdaptBase<t_shared_base, t_data, t_allocator> &, Control &, Mode const) noexcept;

    explicit AllocAdaptData(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    t_data m_data;
  };


  template<class t_shared_base, class t_data, class t_allocator, typename = void>
  class AllocAdaptDataFunction: public AllocAdaptData<t_shared_base, t_data, t_allocator>
  {
  public:
    explicit AllocAdaptDataFunction(typename t_data::DataValueType const p_user_init) noexcept
    : AllocAdaptData<t_shared_base, t_data, t_allocator>(p_user_init)
    {}
  };

  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptDataFunction<t_shared_base, t_data, t_allocator, std::enable_if_t<t_allocator::SupportsStatic::value>>
  : public AllocAdaptData<t_shared_base, t_data, t_allocator>
  {
    using Base = AllocAdaptData<t_shared_base, t_data, t_allocator>;

  public:
    explicit AllocAdaptDataFunction(typename t_data::DataValueType const p_user_init) noexcept
    : Base(p_user_init)
    {}

    typename Base::Function m_function{};
  };


  template<class t_shared_base, class t_data, class t_allocator, typename>
  class AllocAdaptBase: public AllocAdaptDataFunction<t_shared_base, t_data, t_allocator>
  {
  public:
    using SupportsStatic = typename t_allocator::SupportsStatic;

    explicit AllocAdaptBase(typename t_data::DataValueType const p_user_init) noexcept
    : AllocAdaptDataFunction<t_shared_base, t_data, t_allocator>(p_user_init)
    {}

    t_allocator &
    allocator() noexcept
    {
      return m_allocator;
    }

  private:
    t_allocator m_allocator;
  };

  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptBase<t_shared_base, t_data, t_allocator, std::enable_if_t<is_empty_base<t_allocator>>>
  : public AllocAdaptDataFunction<t_shared_base, t_data, t_allocator>
  , private t_allocator
  {
  public:
    using SupportsStatic = typename t_allocator::SupportsStatic;

    explicit AllocAdaptBase(typename t_data::DataValueType const p_user_init) noexcept
    : AllocAdaptDataFunction<t_shared_base, t_data, t_allocator>(p_user_init)
    {}

    t_allocator &
    allocator() noexcept
    {
      return *this;
    }
  };
} // namespace detail


PNTR_NAMESPACE_END
