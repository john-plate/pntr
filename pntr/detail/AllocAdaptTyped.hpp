// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/PntrTypeTraits.hpp>

PNTR_NAMESPACE_BEGIN


template<class t_shared_base, class t_data, class t_allocator>
class ControlAlloc;

namespace detail
{
  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptTyped;

  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptTypedData
  {
  public:
    explicit AllocAdaptTypedData(typename t_data::DataValueType const p_user_init) noexcept
    : m_data(p_user_init)
    {}

    enum class Mode
    {
      e_destroy,
      e_deallocate
    };

    using Control = ControlAlloc<t_shared_base, t_data, t_allocator>;
    using Function = void (*)(AllocAdaptTyped<t_shared_base, t_data, t_allocator> &, Control &, Mode const) noexcept;

    t_data m_data;
    Function m_function{};
  };

  template<class t_shared_base, class t_data, class t_allocator, typename = void>
  class AllocAdaptTypedBase: public AllocAdaptTypedData<t_shared_base, t_data, t_allocator>
  {
  public:
    explicit AllocAdaptTypedBase(typename t_data::DataValueType const p_user_init) noexcept
    : AllocAdaptTypedData<t_shared_base, t_data, t_allocator>(p_user_init)
    {}

    template<class t_forward>
    void
    init_allocator(t_forward && p_allocator) noexcept
    {
      new (&m_allocator_storage) t_allocator(std::forward<t_forward>(p_allocator));
    }

    t_allocator &
    allocator() noexcept
    {
      return *std::launder(reinterpret_cast<t_allocator *>(&m_allocator_storage));
    }

  private:
    alignas(t_allocator) char m_allocator_storage[sizeof(t_allocator)];
  };

  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptTypedBase<t_shared_base, t_data, t_allocator, std::enable_if_t<is_empty_base<t_allocator>>>
  : public AllocAdaptTypedData<t_shared_base, t_data, t_allocator>
  , private t_allocator
  {
  public:
    explicit AllocAdaptTypedBase(typename t_data::DataValueType const p_user_init) noexcept
    : AllocAdaptTypedData<t_shared_base, t_data, t_allocator>(p_user_init)
    {}

    template<class t_forward>
    void
    init_allocator(t_forward &&) noexcept
    {}

    t_allocator &
    allocator() noexcept
    {
      return *this;
    }
  };


  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptTyped: public AllocAdaptTypedBase<t_shared_base, t_data, t_allocator>
  {
    using Base = AllocAdaptTypedBase<t_shared_base, t_data, t_allocator>;
    using Control = typename Base::Control;
    using Mode = typename Base::Mode;

  public:
    explicit AllocAdaptTyped(typename t_data::DataValueType const p_user_init) noexcept
    : Base(p_user_init)
    {}

    template<class t_shared, class t_nothrow, class t_shared_allocator, typename... t_args>
    static t_shared *
    create(t_shared_allocator & p_allocator, t_args &&... p_args) noexcept(t_nothrow::value)
    {
      static_assert(std::is_same_v<t_shared_base, typename t_shared::PntrSharedBase>);
      static_assert(std::is_same_v<t_shared, typename t_shared_allocator::value_type>);

      using AllocTraits = std::allocator_traits<t_shared_allocator>;
      t_shared * shared = nullptr;
      try
      {
        shared = AllocTraits::allocate(p_allocator, 1u);
      }
      catch (...)
      {
        if constexpr (!t_nothrow::value)
        {
          throw;
        }
      }
      if (shared != nullptr)
      {
        try
        {
          AllocTraits::construct(p_allocator, shared, std::forward<t_args>(p_args)...);
        }
        catch (...)
        {
          AllocTraits::deallocate(p_allocator, shared, 1u);
          if constexpr (t_nothrow::value)
          {
            shared = nullptr;
          }
          else
          {
            throw;
          }
        }
      }
      return shared;
    }

    template<class t_shared, class t_forward>
    static t_shared *
    try_init(AllocAdaptTyped & p_self, t_shared * p_shared, t_forward && p_allocator) noexcept
    {
      p_self.m_function = destroy_or_deallocate<t_shared>;
      if constexpr (is_static_castable<t_shared>)
      {
        p_self.init_allocator(std::forward<t_forward>(p_allocator));
      }
      else if (calc_base_offset(p_shared) <= t_data::get_max_offset())
      {
        p_self.init_allocator(std::forward<t_forward>(p_allocator));
      }
      else
      {
#ifndef PNTR_UNITTESTS
        PNTR_LOG_ERROR("Pointer offset is too big");
#endif
        using SharedAlloc = typename std::allocator_traits<t_allocator>::template rebind_alloc<t_shared>;
        using AllocTraits = std::allocator_traits<SharedAlloc>;
        AllocTraits::destroy(p_allocator, p_shared);
        AllocTraits::deallocate(p_allocator, p_shared, 1u);
        p_shared = nullptr;
      }
      return p_shared;
    }

    void
    destroy(Control & p_control, t_shared_base &) noexcept
    {
      PNTR_TRY_LOG_ERROR(this->m_function == nullptr, "Invalid function pointer");
      if (this->m_function != nullptr)
      {
        this->m_function(*this, p_control, Mode::e_destroy);
      }
    }

    static void
    deallocate(AllocAdaptTyped & p_self, Control & p_control) noexcept
    {
      PNTR_TRY_LOG_ERROR(p_self.m_function == nullptr, "Invalid function pointer");
      if (p_self.m_function != nullptr)
      {
        p_self.m_function(p_self, p_control, Mode::e_deallocate);
      }
    }

  private:
    template<class t_shared>
    static void
    destroy_or_deallocate(AllocAdaptTyped & p_self, Control & p_control, Mode const p_mode) noexcept
    {
      using SharedAlloc = typename std::allocator_traits<t_allocator>::template rebind_alloc<t_shared>;
      using AllocTraits = std::allocator_traits<SharedAlloc>;

      SharedAlloc alloc(p_self.allocator());
      t_shared_base * const shared_base = t_shared_base::get_shared_base(p_control);

      if constexpr (is_static_castable<t_shared>)
      {
        t_shared * const shared = static_cast<t_shared *>(shared_base);
        switch (p_mode)
        {
          case Mode::e_destroy:
          {
            AllocTraits::destroy(alloc, shared);
            break;
          }
          case Mode::e_deallocate:
          {
            p_control.~Control();
            AllocTraits::deallocate(alloc, shared, 1u);
            break;
          }
        }
      }
      else
      {
        switch (p_mode)
        {
          case Mode::e_destroy:
          {
            t_shared * const shared = dynamic_cast<t_shared *>(shared_base);
            p_self.m_data.try_set_offset(calc_base_offset(shared));
            AllocTraits::destroy(alloc, shared);
            break;
          }
          case Mode::e_deallocate:
          {
            t_shared * const shared =
              static_cast<t_shared *>(calc_shared_pointer(shared_base, p_self.m_data.get_offset()));
            p_control.~Control();
            AllocTraits::deallocate(alloc, shared, 1u);
            break;
          }
        }
      }
    }

    static_assert(!std::is_const_v<typename t_allocator::value_type>);
    static_assert(!std::is_volatile_v<typename t_allocator::value_type>);
  };
} // namespace detail


PNTR_NAMESPACE_END
