// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/detail/AllocAdaptBase.hpp>

PNTR_NAMESPACE_BEGIN


namespace detail
{
  template<class t_allocator, typename = void>
  struct HasTypeInfoDeallocate: std::false_type
  {};

  template<class t_allocator>
  struct HasTypeInfoDeallocate<t_allocator, std::void_t<typename t_allocator::TypeInfoDeallocate>>: std::true_type
  {};


  template<class t_shared_base, class t_data, class t_allocator>
  class AllocAdaptTypeInfo: public AllocAdaptBase<t_shared_base, t_data, t_allocator>
  {
    using Base = AllocAdaptBase<t_shared_base, t_data, t_allocator>;
    using Control = typename Base::Control;
    using Mode = typename Base::Mode;
    inline static constexpr bool s_supports_static = Base::SupportsStatic::value;

  public:
    explicit AllocAdaptTypeInfo(typename t_data::DataValueType const p_user_init) noexcept
    : Base(p_user_init)
    {}

    template<class t_shared, class t_nothrow, class t_shared_allocator, typename... t_args>
    static t_shared *
    create(t_shared_allocator & p_allocator, t_args &&... p_args) noexcept(t_nothrow::value)
    {
      static_assert(std::is_same_v<t_shared_base, typename t_shared::PntrSharedBase>);

      static constexpr bool is_special_aligned = (alignof(t_shared) > alignof(std::max_align_t));
      static constexpr bool supports_align =
        (s_supports_static
         || (t_data::get_max_align() != 0u && calc_base_align_offset<t_shared>() <= t_data::get_max_align()));
      static constexpr std::size_t align = (supports_align ? alignof(t_shared) : alignof(std::max_align_t));

      static_assert(s_supports_static || calc_base_size_offset<t_shared>() <= t_data::get_max_size());
      static_assert(supports_align || (t_data::get_max_align() == 0u && !is_special_aligned));

      t_shared * shared = nullptr;
      void * storage = nullptr;
      if constexpr (t_nothrow::value)
      {
        try
        {
          storage = p_allocator.allocate(sizeof(t_shared), align);
          if (storage != nullptr)
          {
            shared = new (storage) t_shared(std::forward<t_args>(p_args)...);
          }
        }
        catch (...)
        {}
      }
      else
      {
        storage = p_allocator.allocate(sizeof(t_shared), align);
        if (storage != nullptr)
        {
          try
          {
            shared = new (storage) t_shared(std::forward<t_args>(p_args)...);
          }
          catch (...)
          {
            p_allocator.deallocate(storage, sizeof(t_shared), align);
            throw;
          }
        }
        else
        {
          throw std::bad_alloc();
        }
      }
      if (storage != nullptr && shared == nullptr)
      {
        p_allocator.deallocate(storage, sizeof(t_shared), align);
      }
      return shared;
    }

    template<class t_shared, class t_forward>
    static t_shared *
    try_init(AllocAdaptTypeInfo & p_self, t_shared * const p_shared, t_forward && p_allocator) noexcept
    {
      static constexpr bool supports_align =
        (s_supports_static
         || (t_data::get_max_align() != 0u && calc_base_align_offset<t_shared>() <= t_data::get_max_align()));
      static constexpr std::size_t align = (supports_align ? alignof(t_shared) : alignof(std::max_align_t));

      std::size_t offset = 0u;
      if constexpr (s_supports_static)
      {
        p_self.m_function = destroy_or_deallocate<t_shared>;
        if constexpr (!is_static_castable<t_shared>)
        {
          offset = calc_base_offset(p_shared);
        }
      }
      else
      {
        offset = calc_base_offset(p_shared);
      }
      if (offset != std::numeric_limits<std::size_t>::max())
      {
        if (offset <= t_data::get_max_offset())
        {
          if constexpr (s_supports_static)
          {
            p_self.allocator() = std::forward<t_forward>(p_allocator);
            return p_shared;
          }
          else if (p_self.m_data.try_set_offset(offset))
          {
            p_self.m_data.try_set_size(calc_base_size_offset<t_shared>());
            p_self.m_data.try_set_align(calc_base_align_offset<t_shared>());
            p_self.allocator() = std::forward<t_forward>(p_allocator);
            return p_shared;
          }
          else
          {
            PNTR_LOG_ERROR("Unable to use shared pointer offset without static support");
          }
        }
#ifndef PNTR_UNITTESTS
        else
        {
          PNTR_LOG_ERROR("Pointer offset is too big");
        }
#endif
      }
      p_shared->~t_shared();
      p_allocator.deallocate(p_shared, sizeof(t_shared), align);
      return nullptr;
    }

    template<class t_other>
    void
    destroy(Control & p_control, t_other & p_other) noexcept
    {
      if constexpr (s_supports_static)
      {
        PNTR_TRY_LOG_ERROR(this->m_function == nullptr, "Invalid function pointer");
        if (this->m_function != nullptr)
        {
          this->m_function(*this, p_control, Mode::e_destroy);
        }
      }
      else
      {
        p_other.~t_other();
      }
    }

    static void
    deallocate(AllocAdaptTypeInfo & p_self, Control & p_control) noexcept
    {
      if constexpr (s_supports_static)
      {
        PNTR_TRY_LOG_ERROR(p_self.m_function == nullptr, "Invalid function pointer");
        if (p_self.m_function != nullptr)
        {
          p_self.m_function(p_self, p_control, Mode::e_deallocate);
        }
      }
      else
      {
        t_shared_base * const shared_base = t_shared_base::get_shared_base(p_control);
        std::size_t offset = p_self.m_data.get_offset();
        void * const storage = calc_shared_pointer(shared_base, offset);
        std::size_t const size = calc_shared_size<t_shared_base>(p_self.m_data.get_size());
        std::size_t const align =
          (t_data::get_max_align() == 0u ? alignof(std::max_align_t)
                                         : calc_shared_align<t_shared_base>(p_self.m_data.get_align()));
        t_allocator allocator = std::move(p_self.allocator());
        p_control.~Control();
        allocator.deallocate(storage, size, align);
      }
    }

  private:
    template<class t_shared>
    static void
    destroy_or_deallocate(AllocAdaptBase<t_shared_base, t_data, t_allocator> & p_self, Control & p_control,
                          Mode const p_mode) noexcept
    {
      t_shared_base * const shared_base = t_shared_base::get_shared_base(p_control);
      if constexpr (is_static_castable<t_shared>)
      {
        t_shared * const shared = static_cast<t_shared *>(shared_base);
        switch (p_mode)
        {
          case Mode::e_destroy:
          {
            shared->~t_shared();
            break;
          }
          case Mode::e_deallocate:
          {
            t_allocator allocator = std::move(p_self.allocator());
            p_control.~Control();
            allocator.deallocate(shared, sizeof(t_shared), alignof(t_shared));
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
            shared->~t_shared();
            break;
          }
          case Mode::e_deallocate:
          {
            void * const storage = calc_shared_pointer(shared_base, p_self.m_data.get_offset());
            t_allocator allocator = std::move(p_self.allocator());
            p_control.~Control();
            allocator.deallocate(storage, sizeof(t_shared), alignof(t_shared));
            break;
          }
        }
      }
    }
  };
} // namespace detail


PNTR_NAMESPACE_END
