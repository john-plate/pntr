#include "tests-common.hpp"


template<typename t_type>
class TestAllocator
{
public:
  using value_type = t_type;

  TestAllocator() noexcept
  {
    ++g_allocator_construct;
  }

  TestAllocator(TestAllocator const &) noexcept
  {
    ++g_allocator_construct;
  }

  TestAllocator(TestAllocator &&) noexcept
  {
    ++g_allocator_construct;
  }

  template<typename t_other>
  constexpr TestAllocator(TestAllocator<t_other> const &) noexcept
  {
    ++g_allocator_construct;
  }

  ~TestAllocator() noexcept
  {
    ++g_allocator_destroy;
  }

  TestAllocator & operator=(TestAllocator &&) = default;
  TestAllocator & operator=(TestAllocator const &) = default;

  t_type *
  allocate(std::size_t p_count)
  {
    t_type * const storage = static_cast<t_type *>(std::malloc(p_count * sizeof(t_type)));
    if (storage == nullptr)
    {
      throw std::bad_alloc();
    }
    TestAllocator<pntr::detail::BaseType<t_type>>::s_storage = storage;
    return storage;
  }

  void
  deallocate(t_type * p_storage, std::size_t) noexcept
  {
    if (TestAllocator<pntr::detail::BaseType<t_type>>::s_storage == p_storage)
    {
      TestAllocator<pntr::detail::BaseType<t_type>>::s_storage = nullptr;
    }
    std::free(p_storage);
  }

  inline static void * s_storage = nullptr;
};

template<class t_type, class t_other>
bool
operator==(TestAllocator<t_type> const &, TestAllocator<t_other> const &)
{
  return true;
}

template<class t_type, class t_other>
bool
operator!=(TestAllocator<t_type> const &, TestAllocator<t_other> const &)
{
  return false;
}


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
struct AllocatorMalloc
{
  template<class t_shared_base>
  using Intruder = pntr::IntruderAlloc<t_shared_base, pntr::ThreadUnsafe>;

  using Shared = typename t_shared::template Shared<Intruder>;
  static constexpr bool s_const = t_shared::s_const;

  template<typename t_type>
  using Allocator = typename Shared::PntrAllocator;

  template<typename t_type>
  static Allocator<t_type>
  create() noexcept
  {
    return Allocator<t_type>();
  }

  // Require that the size of a minimal class is as expected.
  static_assert(sizeof(MinimalSharedClass<Intruder>) == sizeof(std::uint64_t));

  static constexpr bool s_saves_type = false;
  static constexpr bool s_destroys_shared = true;
  static constexpr bool s_supports_virtual = true;
  static constexpr bool s_supports_aligned = true;
};


template<class t_shared>
struct AllocatorMalloc8
{
  template<class t_shared_base>
  using Intruder = pntr::IntruderAlloc<t_shared_base, pntr::ThreadUnsafe, std::uint8_t, 4u, 4u, 0u>;

  using Shared = typename t_shared::template Shared<Intruder>;
  static constexpr bool s_const = t_shared::s_const;

  template<typename t_type>
  using Allocator = typename Shared::PntrAllocator;

  template<typename t_type>
  static Allocator<t_type>
  create() noexcept
  {
    return Allocator<t_type>();
  }

  // Require that the size of a minimal class is as expected.
  static_assert(sizeof(MinimalSharedClass<Intruder>) == sizeof(std::uint8_t));

  static constexpr bool s_saves_type = false;
  static constexpr bool s_destroys_shared = true;
  static constexpr bool s_supports_virtual = false;
  static constexpr bool s_supports_aligned = false;
};


template<class t_shared>
struct AllocatorMallocStatic
{
  template<class t_shared_base>
  using Intruder = pntr::IntruderMallocStatic<t_shared_base, pntr::ThreadUnsafe>;

  using Shared = typename t_shared::template Shared<Intruder>;
  static constexpr bool s_const = t_shared::s_const;

  template<typename t_type>
  using Allocator = typename Shared::PntrAllocator;

  template<typename t_type>
  static Allocator<t_type>
  create() noexcept
  {
    return Allocator<t_type>();
  }

  // Require that the size of a minimal class is as expected.
  static_assert(sizeof(MinimalSharedClass<Intruder>) == sizeof(std::uint64_t) + sizeof(void *));

  static constexpr bool s_saves_type = true;
  static constexpr bool s_destroys_shared = false;
  static constexpr bool s_supports_virtual = true;
  static constexpr bool s_supports_aligned = true;
};


#ifdef __cpp_lib_memory_resource

template<class t_shared>
struct AllocatorMemoryResource
{
  template<class t_shared_base>
  using Intruder = pntr::IntruderAlloc<t_shared_base, pntr::ThreadUnsafe, std::uint64_t, 32u, 16u, 6u, 6u, 4u,
                                       pntr::AllocatorMemoryResource<pntr::NoStaticSupport>>;

  using Shared = typename t_shared::template Shared<Intruder>;
  static constexpr bool s_const = t_shared::s_const;

  template<typename t_type>
  using Allocator = typename Shared::PntrAllocator;

  template<typename t_type>
  static Allocator<t_type>
  create() noexcept
  {
    return Allocator<t_type>();
  }

  // Require that the size of a minimal class is as expected.
  static_assert(sizeof(MinimalSharedClass<Intruder>) == sizeof(std::uint64_t) + sizeof(void *));

  static constexpr bool s_saves_type = false;
  static constexpr bool s_destroys_shared = true;
  static constexpr bool s_supports_virtual = true;
  static constexpr bool s_supports_aligned = true;
};


template<class t_shared>
struct AllocatorMemoryResourceStatic
{
  template<class t_shared_base>
  using Intruder = pntr::IntruderAlloc<t_shared_base, pntr::ThreadUnsafe, std::uint64_t, 32u, 32u, pntr::shared_bits,
                                       0u, 0u, pntr::AllocatorMemoryResource<pntr::StaticSupport>>;

  using Shared = typename t_shared::template Shared<Intruder>;
  static constexpr bool s_const = t_shared::s_const;

  template<typename t_type>
  using Allocator = typename Shared::PntrAllocator;

  template<typename t_type>
  static Allocator<t_type>
  create() noexcept
  {
    return Allocator<t_type>();
  }

  // Require that the size of a minimal class is as expected.
  static_assert(sizeof(MinimalSharedClass<Intruder>) == sizeof(std::uint64_t) + 2u * sizeof(void *));

  static constexpr bool s_saves_type = true;
  static constexpr bool s_destroys_shared = false;
  static constexpr bool s_supports_virtual = true;
  static constexpr bool s_supports_aligned = true;
};

#endif // __cpp_lib_memory_resource


template<class t_shared>
struct AllocatorTest
{
  template<class t_shared_base>
  using Intruder = pntr::IntruderAlloc<t_shared_base, pntr::ThreadUnsafe, std::uint64_t, 32u, 32u, pntr::shared_bits,
                                       0u, 0u, TestAllocator<t_shared_base>>;

  using Shared = typename t_shared::template Shared<Intruder>;
  static constexpr bool s_const = t_shared::s_const;

  template<typename t_type>
  using Allocator = TestAllocator<t_type>;

  template<typename t_type>
  static Allocator<t_type>
  create() noexcept
  {
    return Allocator<t_type>();
  }

  // Require that the size of a minimal class is as expected.
  static_assert(sizeof(MinimalSharedClass<Intruder>) == sizeof(std::uint64_t) + sizeof(void *));

  static constexpr bool s_saves_type = true;
  static constexpr bool s_destroys_shared = false;
  static constexpr bool s_supports_virtual = true;
  static constexpr bool s_supports_aligned = true;
};


#ifdef __cpp_lib_memory_resource
TEMPLATE_PRODUCT_TEST_CASE(TEST_PREFIX "ControlAlloc", "",
                           (AllocatorMalloc, AllocatorMalloc8, AllocatorMallocStatic, AllocatorMemoryResource,
                            AllocatorMemoryResourceStatic, AllocatorTest),
                           (Minimal, MinimalAlign, MinimalConst, Static, StaticAlign, StaticConst, Polymorphic,
                            PolymorphicAlign, PolymorphicConst, Virtual, VirtualAlign, VirtualConst))
#else
TEMPLATE_PRODUCT_TEST_CASE(TEST_PREFIX "ControlAlloc", "",
                           (AllocatorMalloc, AllocatorMalloc8, AllocatorMallocStatic, AllocatorTest),
                           (Minimal, MinimalAlign, MinimalConst, Static, StaticAlign, StaticConst, Polymorphic,
                            PolymorphicAlign, PolymorphicConst, Virtual, VirtualAlign, VirtualConst))
#endif
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

  if constexpr (alignof(Shared) <= alignof(std::max_align_t) || TestType::s_supports_aligned)
  {
    if constexpr (pntr::detail::is_static_castable<Shared> || TestType::s_supports_virtual)
    {
      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::WeakPtr<PtrType> ws;
        {
          pntr::SharedPtr<PtrType> s = pntr::allocate_shared_nothrow<PtrType>(TestType::template create<Shared>());
          ws = s;
          REQUIRE(Shared::get_construct_count() == 1u);
          REQUIRE(Shared::get_destroy_count() == 0u);
          REQUIRE(Shared::PntrAllocator::s_storage != nullptr);
        }
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == (supports_shared_delete ? 1u : 0u));
        REQUIRE(Shared::PntrAllocator::s_storage != nullptr);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_shared_delete ? 1u : 0u));
      REQUIRE(Shared::PntrAllocator::s_storage == nullptr);
      REQUIRE(g_allocator_construct == g_allocator_destroy);

      Shared::get_construct_count() = 0u;
      Shared::get_destroy_count() = 0u;
      {
        pntr::WeakPtr<BasePtrType> wb;
        {
          pntr::SharedPtr<BasePtrType> b = pntr::allocate_shared<PtrType>(TestType::template create<Shared>());
          wb = b;
          REQUIRE(Shared::get_construct_count() == 1u);
          REQUIRE(Shared::get_destroy_count() == 0u);
          REQUIRE(Shared::PntrAllocator::s_storage != nullptr);
        }
        REQUIRE(Shared::get_construct_count() == 1u);
        REQUIRE(Shared::get_destroy_count() == (supports_base_delete ? 1u : 0u));
        REQUIRE(Shared::PntrAllocator::s_storage != nullptr);
      }
      REQUIRE(Shared::get_construct_count() == 1u);
      REQUIRE(Shared::get_destroy_count() == (supports_base_delete ? 1u : 0u));
      REQUIRE(Shared::PntrAllocator::s_storage == nullptr);
      REQUIRE(g_allocator_construct == g_allocator_destroy);
    }
    else
    {
      REQUIRE(pntr::allocate_shared_nothrow<PtrType>(TestType::template create<PtrType>()).get() == nullptr);
    }
  }
}
