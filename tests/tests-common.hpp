#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

// Enables features for unit tests
#define PNTR_UNITTESTS

inline static unsigned g_deleter_construct = 0u;
inline static unsigned g_deleter_destroy = 0u;
inline static unsigned g_allocator_construct = 0u;
inline static unsigned g_allocator_destroy = 0u;

// Use Catch2 logging
#define PNTR_ENABLE_LOGGING         1
#define PNTR_LOG_WARNING(x_message) WARN(x_message)
#define PNTR_LOG_ERROR(x_message)   FAIL_CHECK(x_message)

#include <pntr/pntr.hpp>

#ifndef TEST_PREFIX
  #define TEST_PREFIX
#endif


template<template<class> class t_intruder>
struct MinimalSharedClass: t_intruder<MinimalSharedClass<t_intruder>>
{
  static unsigned int &
  get_construct_count() noexcept
  {
    static unsigned int s_count = 0u;
    return s_count;
  }

  static unsigned int &
  get_destroy_count() noexcept
  {
    static unsigned int s_count = 0u;
    return s_count;
  }

  MinimalSharedClass() noexcept
  {
    ++get_construct_count();
  }

  ~MinimalSharedClass()
  {
    ++get_destroy_count();
  }
};


template<template<class> class t_intruder>
struct MinimalAlignSharedClass: t_intruder<MinimalAlignSharedClass<t_intruder>>
{
  static unsigned int &
  get_construct_count() noexcept
  {
    static unsigned int s_count = 0u;
    return s_count;
  }

  static unsigned int &
  get_destroy_count() noexcept
  {
    static unsigned int s_count = 0u;
    return s_count;
  }

  MinimalAlignSharedClass() noexcept
  {
    ++get_construct_count();
  }

  ~MinimalAlignSharedClass()
  {
    ++get_destroy_count();
  }

  alignas(64u) std::uint8_t const m_value{23u};
};


struct Base
{
  static unsigned int &
  get_construct_count() noexcept
  {
    static unsigned int s_count = 0u;
    return s_count;
  }

  static unsigned int &
  get_destroy_count() noexcept
  {
    static unsigned int s_count = 0u;
    return s_count;
  }

  std::uint8_t const m_base_value{23u};
};


template<template<class> class t_intruder>
struct StaticSharedBaseClass
: Base
, t_intruder<StaticSharedBaseClass<t_intruder>>
{
  std::uint8_t const m_shared_base_value{42u};
};


template<template<class> class t_intruder>
struct StaticDerivedStaticClass: StaticSharedBaseClass<t_intruder>
{
  StaticDerivedStaticClass() noexcept
  {
    ++this->get_construct_count();
  }

  ~StaticDerivedStaticClass()
  {
    ++this->get_destroy_count();
  }

  std::uint8_t m_shared_value{71u};
};


template<template<class> class t_intruder>
struct StaticAlignDerivedStaticClass: StaticSharedBaseClass<t_intruder>
{
  StaticAlignDerivedStaticClass() noexcept
  {
    ++this->get_construct_count();
  }

  ~StaticAlignDerivedStaticClass()
  {
    ++this->get_destroy_count();
  }

  alignas(64u) std::uint8_t m_shared_value{71u};
};


template<template<class> class t_intruder>
struct PolymorphicSharedBaseClass
: Base
, t_intruder<PolymorphicSharedBaseClass<t_intruder>>
{
  virtual ~PolymorphicSharedBaseClass() = default;

  std::uint8_t const m_shared_base_value{42u};
};


template<template<class> class t_intruder>
struct StaticDerivedPolymorphicClass: PolymorphicSharedBaseClass<t_intruder>
{
  StaticDerivedPolymorphicClass() noexcept
  {
    ++this->get_construct_count();
  }

  ~StaticDerivedPolymorphicClass() override
  {
    ++this->get_destroy_count();
  }

  std::uint8_t m_shared_value{71u};
};


template<template<class> class t_intruder>
struct StaticAlignDerivedPolymorphicClass: PolymorphicSharedBaseClass<t_intruder>
{
  StaticAlignDerivedPolymorphicClass() noexcept
  {
    ++this->get_construct_count();
  }

  ~StaticAlignDerivedPolymorphicClass() override
  {
    ++this->get_destroy_count();
  }

  alignas(64u) std::uint8_t m_shared_value{71u};
};


template<template<class> class t_intruder>
struct VirtualDerivedPolymorphicClass: virtual PolymorphicSharedBaseClass<t_intruder>
{
  VirtualDerivedPolymorphicClass() noexcept
  {
    ++this->get_construct_count();
  }

  ~VirtualDerivedPolymorphicClass() override
  {
    ++this->get_destroy_count();
  }

  std::uint8_t m_shared_value{71u};
};


template<template<class> class t_intruder>
struct VirtualAlignDerivedPolymorphicClass: virtual PolymorphicSharedBaseClass<t_intruder>
{
  VirtualAlignDerivedPolymorphicClass() noexcept
  {
    ++this->get_construct_count();
  }

  ~VirtualAlignDerivedPolymorphicClass() override
  {
    ++this->get_destroy_count();
  }

  alignas(64u) std::uint8_t m_shared_value{71u};
};
