// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <iostream>
#include <type_traits>
#include <utility>

#ifndef PNTR_NAMESPACE
  #define PNTR_NAMESPACE pntr
#endif

// clang-format off
#define PNTR_NAMESPACE_BEGIN namespace PNTR_NAMESPACE {
#define PNTR_NAMESPACE_END   }
// clang-format on

#ifndef PNTR_ASSERT
  #include <cassert>
  #define PNTR_ASSERT(x_condition) assert(x_condition)
#endif


#ifndef PNTR_ENABLE_LOGGING
  #ifdef NDEBUG
    #define PNTR_ENABLE_LOGGING 0
  #else
    #define PNTR_ENABLE_LOGGING 1
  #endif
#endif

#ifndef PNTR_LOG_MESSAGE
  #if PNTR_ENABLE_LOGGING
    #define PNTR_LOG_MESSAGE(x_severity, x_message) \
      std::cerr << #x_severity << " in " << __FILE__ << ", line " << __LINE__ << ": " << (x_message) << std::endl
  #else
    #define PNTR_LOG_MESSAGE(x_severity, x_message) static_cast<void>(0)
  #endif
#endif

#ifndef PNTR_LOG_WARNING
  #if PNTR_ENABLE_LOGGING
    #define PNTR_LOG_WARNING(x_message) PNTR_LOG_MESSAGE(Warning, x_message)
  #else
    #define PNTR_LOG_WARNING(x_message) static_cast<void>(0)
  #endif
#endif

#ifndef PNTR_LOG_ERROR
  #if PNTR_ENABLE_LOGGING
    #define PNTR_LOG_ERROR(x_message) PNTR_LOG_MESSAGE(Error, x_message)
  #else
    #define PNTR_LOG_ERROR(x_message) static_cast<void>(0)
  #endif
#endif

#ifndef PNTR_TRY_LOG_WARNING
  #if PNTR_ENABLE_LOGGING
    #define PNTR_TRY_LOG_WARNING(x_condition, x_message) \
      do \
      { \
        if (x_condition) \
        { \
          PNTR_LOG_WARNING(x_message); \
        } \
      } \
      while (false)
  #else
    #define PNTR_TRY_LOG_WARNING(x_condition, x_message) static_cast<void>(0)
  #endif
#endif

#ifndef PNTR_TRY_LOG_ERROR
  #if PNTR_ENABLE_LOGGING
    #define PNTR_TRY_LOG_ERROR(x_condition, x_message) \
      do \
      { \
        if (x_condition) \
        { \
          PNTR_LOG_ERROR(x_message); \
        } \
      } \
      while (false)
  #else
    #define PNTR_TRY_LOG_ERROR(x_condition, x_message) static_cast<void>(0)
  #endif
#endif


PNTR_NAMESPACE_BEGIN


using StaticSupport = std::true_type;
using NoStaticSupport = std::false_type;

enum class ControlStatus
{
  e_invalid,
  e_acquired,
  e_shared
};

template<class t_shared>
class SharedPtr;

template<class t_shared>
class WeakPtr;


namespace detail
{
  template<class t_shared, class t_nothrow, typename... t_args>
  SharedPtr<t_shared> make_shared_impl(t_args &&... p_args) noexcept(t_nothrow::value);

  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  std::enable_if_t<!std::is_void_v<typename t_shared::PntrDeleter>, SharedPtr<t_shared>>
  make_shared_with_deleter_impl(t_forward && p_deleter, t_args &&... p_args) noexcept(t_nothrow::value);

  template<class t_shared, class t_nothrow, class t_forward, typename... t_args>
  std::enable_if_t<!std::is_void_v<typename t_shared::PntrAllocator>, SharedPtr<t_shared>>
  allocate_shared_impl(t_forward && p_allocator, t_args &&... p_args) noexcept(t_nothrow::value);
}


PNTR_NAMESPACE_END
