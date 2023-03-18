#include "tests-common.hpp"


struct Minimal
{
  template<template<class> class t_intruder>
  using Shared = MinimalSharedClass<t_intruder>;
  static constexpr bool s_const = false;
};

struct MinimalAlign
{
  template<template<class> class t_intruder>
  using Shared = MinimalAlignSharedClass<t_intruder>;
  static constexpr bool s_const = false;
};

struct MinimalConst
{
  template<template<class> class t_intruder>
  using Shared = MinimalSharedClass<t_intruder>;
  static constexpr bool s_const = true;
};

struct Static
{
  template<template<class> class t_intruder>
  using Shared = StaticDerivedStaticClass<t_intruder>;
  static constexpr bool s_const = false;
};

struct StaticAlign
{
  template<template<class> class t_intruder>
  using Shared = StaticAlignDerivedStaticClass<t_intruder>;
  static constexpr bool s_const = false;
};

struct StaticConst
{
  template<template<class> class t_intruder>
  using Shared = StaticDerivedStaticClass<t_intruder>;
  static constexpr bool s_const = true;
};

struct Polymorphic
{
  template<template<class> class t_intruder>
  using Shared = StaticDerivedPolymorphicClass<t_intruder>;
  static constexpr bool s_const = false;
};

struct PolymorphicAlign
{
  template<template<class> class t_intruder>
  using Shared = StaticAlignDerivedPolymorphicClass<t_intruder>;
  static constexpr bool s_const = false;
};

struct PolymorphicConst
{
  template<template<class> class t_intruder>
  using Shared = StaticDerivedPolymorphicClass<t_intruder>;
  static constexpr bool s_const = true;
};

struct Virtual
{
  template<template<class> class t_intruder>
  using Shared = VirtualDerivedPolymorphicClass<t_intruder>;
  static constexpr bool s_const = false;
};

struct VirtualAlign
{
  template<template<class> class t_intruder>
  using Shared = VirtualAlignDerivedPolymorphicClass<t_intruder>;
  static constexpr bool s_const = false;
};

struct VirtualConst
{
  template<template<class> class t_intruder>
  using Shared = VirtualDerivedPolymorphicClass<t_intruder>;
  static constexpr bool s_const = true;
};


template<class t_shared>
struct DefaultDeleter
{
  template<class t_shared_base>
  using Intruder =
    pntr::IntruderNew<t_shared_base, pntr::ThreadUnsafe, std::uint8_t, 8u, std::default_delete<t_shared_base>>;

  using Shared = typename t_shared::template Shared<Intruder>;
  static constexpr bool s_const = t_shared::s_const;

  template<typename t_type>
  using Deleter = std::default_delete<t_type>;

  template<typename t_type>
  static Deleter<t_type>
  create() noexcept
  {
    return Deleter<t_type>();
  }

  // Require that the size of a minimal class is as expected.
  static_assert(sizeof(MinimalSharedClass<Intruder>)
                == sizeof(typename MinimalSharedClass<Intruder>::PntrUsageValueType));

  static constexpr bool s_saves_type = false;
  static constexpr bool s_destroys_shared = true;
};


template<class t_shared>
struct TemplateDeleter
{
  template<typename t_type>
  struct Deleter
  {
    Deleter() noexcept
    {
      ++g_deleter_construct;
    }

    Deleter(Deleter const &) noexcept
    {
      ++g_deleter_construct;
    }

    Deleter(Deleter &&) noexcept
    {
      ++g_deleter_construct;
    }

    template<typename t_other, typename = std::enable_if_t<std::is_convertible_v<t_other *, t_type *>>>
    Deleter(Deleter<t_other> const &) noexcept
    {
      ++g_deleter_construct;
    }

    ~Deleter() noexcept
    {
      ++g_deleter_destroy;
    }

    Deleter & operator=(Deleter &&) = default;
    Deleter & operator=(Deleter const &) = default;

    void
    operator()(t_type * p_type) const noexcept
    {
      static_assert(sizeof(t_type) != 0u, "incomplete type");
      delete p_type;
    }
  };

  template<class t_shared_base>
  using Intruder = pntr::IntruderNew<t_shared_base, pntr::ThreadUnsafe, std::uint8_t, 8u, Deleter<t_shared_base>>;

  using Shared = typename t_shared::template Shared<Intruder>;
  static constexpr bool s_const = t_shared::s_const;

  template<typename t_type>
  static Deleter<t_type>
  create() noexcept
  {
    return Deleter<t_type>();
  }

  // Require that the size of a minimal class is as expected.
  static_assert(sizeof(MinimalSharedClass<Intruder>)
                == sizeof(typename MinimalSharedClass<Intruder>::PntrUsageValueType));

  static constexpr bool s_saves_type = false;
  static constexpr bool s_destroys_shared = true;
};


template<class t_shared>
struct EmptyDeleter
{
  struct DeleterImpl
  {
    DeleterImpl() noexcept
    {
      ++g_deleter_construct;
    }

    DeleterImpl(DeleterImpl const &) noexcept
    {
      ++g_deleter_construct;
    }

    DeleterImpl(DeleterImpl &&) noexcept
    {
      ++g_deleter_construct;
    }

    ~DeleterImpl() noexcept
    {
      ++g_deleter_destroy;
    }

    DeleterImpl & operator=(DeleterImpl &&) = default;
    DeleterImpl & operator=(DeleterImpl const &) = default;

    template<typename t_type>
    void
    operator()(t_type * p_type) const noexcept
    {
      static_assert(sizeof(t_type) != 0u, "incomplete type");
      delete p_type;
    }
  };

  template<class t_shared_base>
  using Intruder = pntr::IntruderNew<t_shared_base, pntr::ThreadUnsafe, std::uint8_t, 8u, DeleterImpl>;

  using Shared = typename t_shared::template Shared<Intruder>;
  static constexpr bool s_const = t_shared::s_const;

  template<typename t_type>
  using Deleter = DeleterImpl;

  template<typename t_type>
  static Deleter<t_type>
  create() noexcept
  {
    return Deleter<t_type>();
  }

  // Require that the size of a minimal class is as expected.
  static_assert(sizeof(MinimalSharedClass<Intruder>)
                == sizeof(typename MinimalSharedClass<Intruder>::PntrUsageValueType));

  static constexpr bool s_saves_type = false;
  static constexpr bool s_destroys_shared = true;
};


template<class t_shared>
struct PntrDeleter
{
  template<class t_shared_base>
  using Intruder = pntr::IntruderNew<t_shared_base, pntr::ThreadUnsafe, std::uint8_t, 8u, pntr::Deleter<t_shared_base>>;

  using Shared = typename t_shared::template Shared<Intruder>;
  static constexpr bool s_const = t_shared::s_const;

  template<typename t_type>
  using Deleter = pntr::Deleter<t_type>;

  template<typename t_type>
  static Deleter<t_type>
  create() noexcept
  {
    return Deleter<t_type>();
  }

  // Require that the size of a minimal class is as expected.
  static_assert(sizeof(MinimalSharedClass<Intruder>) == 2u * sizeof(void *));

  static constexpr bool s_saves_type = true;
  static constexpr bool s_destroys_shared = false;
};


template<class t_shared>
struct FunctionDeleter
{
  template<class t_shared_base>
  using FunctionDelete = std::function<void(t_shared_base const *)>;

  template<class t_shared_base>
  using Intruder =
    pntr::IntruderNew<t_shared_base, pntr::ThreadUnsafe, std::uint8_t, 8u, FunctionDelete<t_shared_base>>;

  using Shared = typename t_shared::template Shared<Intruder>;
  static constexpr bool s_const = t_shared::s_const;

  template<typename t_type>
  using Deleter = typename Shared::PntrDeleter;

  template<typename t_type>
  static Deleter<t_type>
  create() noexcept
  {
    return Deleter<t_type>([](typename Shared::PntrSharedBase const * p) noexcept { delete p; });
  }

  // Require that the size of a minimal class is as expected.
  static_assert(sizeof(MinimalSharedClass<Intruder>) == alignof(FunctionDelete<void>) + sizeof(FunctionDelete<void>));

  static constexpr bool s_saves_type = false;
  static constexpr bool s_destroys_shared = false;
};


TEMPLATE_PRODUCT_TEST_CASE(TEST_PREFIX "ControlNew default construct", "",
                           (DefaultDeleter, TemplateDeleter, EmptyDeleter, PntrDeleter),
                           (Minimal, MinimalAlign, MinimalConst, Static, StaticAlign, StaticConst, Polymorphic,
                            PolymorphicAlign, PolymorphicConst, Virtual, VirtualAlign, VirtualConst))
{
  using Shared = typename TestType::Shared;
  using SharedBase = typename Shared::PntrSharedBase;
  using PtrType = std::conditional_t<TestType::s_const, Shared const, Shared>;
  using BasePtrType = std::conditional_t<TestType::s_const, SharedBase const, SharedBase>;

  constexpr bool supports_shared_delete =
    (std::is_same_v<Shared, SharedBase> || std::has_virtual_destructor_v<SharedBase> || TestType::s_saves_type
     || TestType::s_destroys_shared);

  constexpr bool supports_base_delete =
    (std::is_same_v<Shared, SharedBase> || std::has_virtual_destructor_v<SharedBase> || TestType::s_saves_type);

  SECTION("Construct from raw pointer")
  {
    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_shared_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<PtrType> s(new Shared);
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_shared_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }

    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_base_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<BasePtrType> b(new Shared);
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_base_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }
  }

  SECTION("Construct from std::unique_ptr")
  {
    using UniquePtr = std::unique_ptr<PtrType, typename TestType::template Deleter<PtrType>>;

    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_shared_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<PtrType> s(UniquePtr(new PtrType));
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_shared_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }

    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_base_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<BasePtrType> b(UniquePtr(new PtrType));
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_base_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }
  }

  SECTION("Construct from make_shared")
  {
    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_shared_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<PtrType> s = pntr::make_shared_nothrow<PtrType>();
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_shared_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }

    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_base_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<BasePtrType> b = pntr::make_shared<PtrType>();
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_base_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }
  }
}


TEMPLATE_PRODUCT_TEST_CASE(TEST_PREFIX "ControlNew explicit construct", "",
                           (DefaultDeleter, TemplateDeleter, EmptyDeleter, PntrDeleter, FunctionDeleter),
                           (Minimal, MinimalAlign, Static, StaticAlign, Polymorphic, PolymorphicAlign, Virtual,
                            VirtualAlign))
{
  using Shared = typename TestType::Shared;
  using SharedBase = typename Shared::PntrSharedBase;
  using PtrType = std::conditional_t<TestType::s_const, Shared const, Shared>;
  using BasePtrType = std::conditional_t<TestType::s_const, SharedBase const, SharedBase>;

  constexpr bool supports_shared_delete =
    (std::is_same_v<Shared, SharedBase> || std::has_virtual_destructor_v<SharedBase> || TestType::s_saves_type
     || TestType::s_destroys_shared);

  constexpr bool supports_base_delete =
    (std::is_same_v<Shared, SharedBase> || std::has_virtual_destructor_v<SharedBase> || TestType::s_saves_type);

  SECTION("Construct from raw pointer with deleter")
  {
    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_shared_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<PtrType> s(new Shared, TestType::template create<Shared>());
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_shared_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }

    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_base_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<BasePtrType> b(new Shared, TestType::template create<Shared>());
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_base_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }
  }

  SECTION("Construct from std::unique_ptr with deleter")
  {
    using UniquePtr = std::unique_ptr<PtrType, typename TestType::template Deleter<Shared>>;

    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_shared_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<PtrType> s(UniquePtr(new PtrType, TestType::template create<Shared>()));
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_shared_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }

    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_base_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<BasePtrType> b(UniquePtr(new PtrType, TestType::template create<Shared>()));
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_base_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }
  }

  SECTION("Construct from make_shared_with_deleter")
  {
    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_shared_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<PtrType> s =
          pntr::make_shared_with_deleter_nothrow<PtrType>(TestType::template create<Shared>());
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_shared_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }

    if constexpr (alignof(Shared) <= alignof(std::max_align_t) || supports_base_delete)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::SharedPtr<BasePtrType> b = pntr::make_shared_with_deleter<PtrType>(TestType::template create<Shared>());
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == 0u);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_base_delete ? 1u : 0u));
      REQUIRE(g_deleter_construct == g_deleter_destroy);
    }
  }
}
