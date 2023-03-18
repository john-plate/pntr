#include <pntr/pntr.hpp>

#include <iostream>


static void
tutorial_intruder_new()
{
  struct PolymorphicSharedBase: pntr::IntruderNew<PolymorphicSharedBase>
  {
    PolymorphicSharedBase()
    {
      std::cout << "PolymorphicSharedBase constructor\n";
    }
    virtual ~PolymorphicSharedBase()
    {
      std::cout << "PolymorphicSharedBase destructor\n";
    }
  };

  struct PolymorphicShared: PolymorphicSharedBase
  {
    PolymorphicShared()
    {
      std::cout << "PolymorphicShared constructor\n";
    }
    ~PolymorphicShared() override
    {
      std::cout << "PolymorphicShared destructor\n";
    }
  };

  std::cout << "Size of the default IntruderNew: " << sizeof(pntr::IntruderNew<PolymorphicSharedBase>) << " bytes\n";
  {
    std::cout << "\nCreating a polymorphic shared object with IntruderNew:\n";
    pntr::SharedPtr<PolymorphicSharedBase> base = pntr::make_shared<PolymorphicShared>();
    std::cout << "\nSuccessfully deleting the object from a base pointer:\n";
    // 'base' is automatically deleted when leaving this scope.
  }

  struct StaticSharedBase: pntr::IntruderNew<StaticSharedBase>
  {
    StaticSharedBase()
    {
      std::cout << "StaticSharedBase constructor\n";
    }
    ~StaticSharedBase()
    {
      std::cout << "StaticSharedBase destructor\n";
    }
  };

  struct StaticShared: StaticSharedBase
  {
    StaticShared()
    {
      std::cout << "StaticShared constructor\n";
    }
    ~StaticShared()
    {
      std::cout << "StaticShared destructor\n";
    }
  };

  {
    std::cout << "\nCreating a non-polymorphic shared object with IntruderNew:\n";
    pntr::SharedPtr<StaticShared> shared = pntr::make_shared<StaticShared>();
    std::cout << "\nSuccessfully deleting the object from the original pointer type:\n";
    // 'shared' is automatically deleted when leaving this scope.
  }

  {
    std::cout << "\nCreating a non-polymorphic shared object with IntruderNew:\n";
    pntr::SharedPtr<StaticSharedBase> base = pntr::make_shared<StaticShared>();
    std::cout << "\nFailed to properly delete the object from a base pointer:\n";
    // 'base' is automatically deleted when leaving this scope.
  }

  std::cout << std::flush;
}


static void
tutorial_intruder_new_static()
{
  struct StaticSharedBase: pntr::IntruderNewStatic<StaticSharedBase>
  {
    StaticSharedBase()
    {
      std::cout << "StaticSharedBase constructor\n";
    }
    ~StaticSharedBase()
    {
      std::cout << "StaticSharedBase destructor\n";
    }
  };

  struct StaticShared: StaticSharedBase
  {
    StaticShared()
    {
      std::cout << "StaticShared constructor\n";
    }
    ~StaticShared()
    {
      std::cout << "StaticShared destructor\n";
    }
  };

  std::cout << "\n\nSize of the default IntruderNewStatic: " << sizeof(pntr::IntruderNewStatic<StaticSharedBase>)
            << " bytes\n";
  {
    std::cout << "\nCreating a non-polymorphic shared object with IntruderNewStatic:\n";
    pntr::SharedPtr<StaticSharedBase> base = pntr::make_shared<StaticShared>();
    std::cout << "\nSuccessfully deleting the object from a base pointer:\n";
    // 'base' is automatically deleted when leaving this scope.
  }

  std::cout << std::flush;
}


static void
tutorial_intruder_alloc()
{
  struct PolymorphicSharedBase: pntr::IntruderAlloc<PolymorphicSharedBase>
  {
    PolymorphicSharedBase()
    {
      std::cout << "PolymorphicSharedBase constructor\n";
    }
    virtual ~PolymorphicSharedBase()
    {
      std::cout << "PolymorphicSharedBase destructor\n";
    }
  };

  struct PolymorphicShared: PolymorphicSharedBase
  {
    PolymorphicShared()
    {
      std::cout << "PolymorphicShared constructor\n";
    }
    ~PolymorphicShared() override
    {
      std::cout << "PolymorphicShared destructor\n";
    }
  };

  std::cout << "\n\nSize of the default IntruderAlloc: " << sizeof(pntr::IntruderAlloc<PolymorphicSharedBase>)
            << " bytes\n";
  {
    pntr::WeakPtr<PolymorphicSharedBase> weak_base;
    {
      std::cout << "\nCreating a polymorphic shared object with IntruderAlloc:\n";
      pntr::SharedPtr<PolymorphicSharedBase> base = pntr::make_shared<PolymorphicShared>();
      weak_base = base;
      std::cout << "\nSuccessfully destroying the object from a base pointer:\n";
      // 'base' is automatically deleted when leaving this scope.
    }
    // 'weak_base' is automatically deleted when leaving this scope and will deallocate the memory block.
  }

  struct StaticSharedBase: pntr::IntruderAlloc<StaticSharedBase>
  {
    StaticSharedBase()
    {
      std::cout << "StaticSharedBase constructor\n";
    }
    ~StaticSharedBase()
    {
      std::cout << "StaticSharedBase destructor\n";
    }
  };

  struct StaticShared: StaticSharedBase
  {
    StaticShared()
    {
      std::cout << "StaticShared constructor\n";
    }
    ~StaticShared()
    {
      std::cout << "StaticShared destructor\n";
    }
  };

  {
    pntr::WeakPtr<StaticShared> weak_shared;
    {
      std::cout << "\nCreating a non-polymorphic shared object with IntruderAlloc:\n";
      pntr::SharedPtr<StaticShared> shared = pntr::make_shared<StaticShared>();
      weak_shared = shared;
      std::cout << "\nSuccessfully destroying the object from the original pointer type:\n";
      // 'shared' is automatically deleted when leaving this scope.
    }
    // 'weak_shared' is automatically deleted when leaving this scope and will deallocate the memory block.
  }

  {
    pntr::WeakPtr<StaticSharedBase> weak_base;
    {
      std::cout << "\nCreating a non-polymorphic shared object with IntruderAlloc:\n";
      pntr::SharedPtr<StaticSharedBase> base = pntr::make_shared<StaticShared>();
      weak_base = base;
      std::cout << "\nFailed to properly destroy the object from a base pointer:\n";
      // 'base' is automatically deleted when leaving this scope.
    }
    // 'weak_base' is automatically deleted when leaving this scope and will deallocate the memory block.
  }

  std::cout << std::flush;
}


static void
tutorial_intruder_malloc_static()
{
  struct StaticSharedBase: pntr::IntruderMallocStatic<StaticSharedBase>
  {
    StaticSharedBase()
    {
      std::cout << "StaticSharedBase constructor\n";
    }
    ~StaticSharedBase()
    {
      std::cout << "StaticSharedBase destructor\n";
    }
  };

  struct StaticShared: StaticSharedBase
  {
    StaticShared()
    {
      std::cout << "StaticShared constructor\n";
    }
    ~StaticShared()
    {
      std::cout << "StaticShared destructor\n";
    }
  };

  std::cout << "\n\nSize of the default IntruderMallocStatic: " << sizeof(pntr::IntruderMallocStatic<StaticSharedBase>)
            << " bytes\n";
  {
    pntr::WeakPtr<StaticSharedBase> weak_base;
    {
      std::cout << "\nCreating a non-polymorphic shared object with IntruderMallocStatic:\n";
      pntr::SharedPtr<StaticSharedBase> base = pntr::make_shared<StaticShared>();
      weak_base = base;
      std::cout << "\nSuccessfully deleting the object from a base pointer:\n";
      // 'base' is automatically deleted when leaving this scope.
    }
    // 'weak_base' is automatically deleted when leaving this scope and will deallocate the memory block.
  }

  std::cout << std::flush;
}


static void
tutorial_check_intruder_efficiency()
{
  {
    struct Shared
    : pntr::Intruder<pntr::ControlNew<
        Shared, pntr::ControlData<pntr::CounterThreadUnsafe, std::uint32_t, 8u, 8u, 4u, 4u, 4u>, pntr::Deleter<Shared>>>
    {};

    std::cout << "\n\nChecking the Intruder efficiency, example 1:\n";
    if (pntr::check_intruder_efficiency(pntr::make_shared<Shared>(), std::cout))
    {
      std::cout << "Maximum efficiency confirmed.\n";
    }
  }

  {
    struct Shared: pntr::IntruderAlloc<Shared, pntr::ThreadUnsafe, std::uint64_t, 16u, 16u, 8u, 8u, 8u>
    {};

    std::cout << "\n\nChecking the Intruder efficiency, example 2:\n";
    if (pntr::check_intruder_efficiency(pntr::make_shared<Shared>(), std::cout))
    {
      std::cout << "Maximum efficiency confirmed.\n";
    }
  }

  {
    struct Shared
    : pntr::IntruderAlloc<Shared, pntr::ThreadUnsafe, std::uint32_t, 8u, 8u, 4u, 4u, 4u, std::allocator<Shared>>
    {};

    std::cout << "\n\nChecking the Intruder efficiency, example 3:\n";
    if (pntr::check_intruder_efficiency(pntr::make_shared<Shared>(), std::cout))
    {
      std::cout << "Maximum efficiency confirmed.\n";
    }
  }

#ifdef __cpp_lib_memory_resource

  {
    struct Block
    {
      alignas(32u) char c[32u];
    };
    struct SharedBase
    : pntr::IntruderAlloc<SharedBase, pntr::ThreadUnsafe, std::uint64_t, 12u, 12u, 12u, 12u, 12u,
                          pntr::AllocatorMemoryResource<pntr::NoStaticSupport>>
    {};
    struct Shared
    : Block
    , SharedBase
    {};

    std::pmr::monotonic_buffer_resource resource;
    std::cout << "\n\nChecking the Intruder efficiency, example 4:\n";
    if (pntr::check_intruder_efficiency(pntr::allocate_shared<Shared>(Shared::PntrAllocator(&resource)), std::cout))
    {
      std::cout << "Maximum efficiency confirmed.\n";
    }
  }

  {
    struct Block
    {
      alignas(32u) char c[32u];
    };
    struct SharedBase
    : pntr::IntruderAlloc<SharedBase, pntr::ThreadUnsafe, std::uint64_t, 28u, 28u, 3u, 3u, 2u,
                          pntr::AllocatorMemoryResource<pntr::NoStaticSupport>>
    {};
    struct Shared
    : Block
    , SharedBase
    {};

    std::pmr::monotonic_buffer_resource resource;
    std::cout << "\n\nChecking the Intruder efficiency, example 5:\n";
    if (pntr::check_intruder_efficiency(pntr::allocate_shared<Shared>(Shared::PntrAllocator(&resource)), std::cout))
    {
      std::cout << "Maximum efficiency confirmed.\n";
    }
  }

#endif // __cpp_lib_memory_resource

  std::cout << std::flush;
}


int
main()
{
  tutorial_intruder_new();
  tutorial_intruder_new_static();
  tutorial_intruder_alloc();
  tutorial_intruder_malloc_static();
  tutorial_check_intruder_efficiency();
  return 0;
}
