#include <pntr/pntr.hpp>

#include <iostream>
#include <version>
#ifdef __cpp_lib_memory_resource
  #include <memory_resource>

  #include <array>
  #include <chrono>
  #include <cstdint>
  #include <cstdlib> // std::rand


// Use a fast global memory pool. Thread safety is not required for this single-threaded example.
static std::pmr::monotonic_buffer_resource &
get_resource() noexcept
{
  static std::pmr::monotonic_buffer_resource resource;
  return resource;
}


// A minimal allocator for 'pntr::ContralAlloc' that needs size and alignment to deallocate.
// It doesn't need to save a pointer to the memory resource like 'AllocatorMemoryResource',
// because we use a global pool.
class MyAllocator
{
public:
  // No need for static support as only the base class is used.
  using SupportsStatic = pntr::NoStaticSupport;

  // ControlAlloc uses this to detect the allocator type.
  using TypeInfoDeallocate = void;

  void *
  allocate(std::size_t p_size, std::size_t p_alignment) noexcept
  {
    return get_resource().allocate(p_size, p_alignment);
  }

  void
  deallocate(void * p_pointer, std::size_t p_size, std::size_t p_alignment) noexcept
  {
    get_resource().deallocate(p_pointer, p_size, p_alignment);
  }
};


class Object
: public pntr::IntruderAlloc< // Object class contains IntruderAlloc, configured to use MyAllocator.
    Object,
    pntr::ThreadUnsafe, // Thread safety is disabled and not required for the single-threaded example.
    std::uint8_t,       // Minimal control block size to show off its configurability.
    6u,          // 6 bits for the usage counter allows 30 SharedPtr (one value is reserved for unmanaged objects).
    2u,          // 2 bits for the weak counter allows 2 WeakPtr (one value is reserved to track the lifetime).
    0u, 0u, 0u,  // No bits for position, size, or alignment as no inheritance is used.
    MyAllocator> // MyAllocator is an empty class that is optimized away in the control block.
{
public:
  std::uint8_t m_lifetime{}; // One byte for the Object, one byte for the Intruder.
};

static_assert(sizeof(Object) == 2u); // The resulting class is really only 2 bytes!


inline std::uint8_t
get_random_digit()
{
  return static_cast<std::uint8_t>(1 + std::rand() % 9);
}

#endif // __cpp_lib_memory_resource

// Program which randomly creates and destroys objects in an array and outputs
// the number of objects in the array. Try to find a pattern. :)
int
main()
{
#ifdef __cpp_lib_memory_resource

  std::srand(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()));

  std::array<pntr::SharedPtr<Object>, 9u> objects;
  std::uint32_t create = 1u;

  for (std::uint32_t row = 0u; row < 80u; ++row)
  {
    for (std::uint32_t col = 0u; col < 40u; ++col)
    {
      std::uint32_t alive = 0u;
      for (auto & object_ptr: objects)
      {
        if (object_ptr && --object_ptr->m_lifetime == 0u)
        {
          object_ptr.reset(); // This destroys and deallocates the object and returns the memory to the pool.
        }
        if (!object_ptr && --create == 0u)
        {
          object_ptr = pntr::make_shared<Object>(); // This creates an object with a default constructed allocator.
          if (object_ptr)
          {
            object_ptr->m_lifetime = get_random_digit();
          }
          create = get_random_digit();
        }
        if (object_ptr)
        {
          ++alive;
        }
      }
      std::cout << ' ' << alive << std::flush;
    }
    std::cout << std::endl;
  }

#else

  std::cout << "This example requires a standard library with polymorphic memory resource." << std::endl;

#endif

  return 0;
}
