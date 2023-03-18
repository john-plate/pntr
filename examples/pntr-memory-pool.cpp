#include <pntr/pntr.hpp>

#include <iostream>
#include <version>
#ifdef __cpp_lib_memory_resource
  #include <memory_resource>

  #include <array>
  #include <chrono>
  #include <cstdint>
  #include <cstdlib> // std::rand


class Object
: public pntr::IntruderAlloc< // Object class contains IntruderAlloc.
    Object,
    pntr::ThreadUnsafe, // Thread safety is disabled and not required for the single-threaded example.
    std::uint64_t,      // Control block size defined by this type.
                        // It doesn't make sense to use a smaller type here as it would be padded to
                        // the size of a pointer, because the allocator saves a pointer.
    32u,                // 32 bits for the usage counter, internally using a separate 'std::uint32_t'.
    32u,                // 32 bits for the weak counter, internally using a separate 'std::uint32_t'.
    0u, 0u, 0u,         // No bits for position, size, or alignment as no inheritance is used.
    // Using the 'pntr' allocator that uses a 'std::pmr::memory_resource'.
    // We could also use 'std::pmr::polymorphic_allocator', but as every allocator which requires type
    // information for deallocation it would require to store an additonal pointer in the control block.
    // No need for static support as only the base class is used.
    pntr::AllocatorMemoryResource<pntr::NoStaticSupport>>
{
public:
  std::uint32_t m_lifetime{};
};

// The resulting class size is the size of the control data, plus the size of one pointer
// for the allocator to save the memory resource, plus the size of a pointer for the lifetime
// variable, which includes padding for the alignment of the control block.
static_assert(sizeof(Object) == sizeof(std::uint64_t) + 2u * sizeof(void *));


inline std::uint32_t
get_random_digit()
{
  return static_cast<std::uint32_t>(1 + std::rand() % 9);
}

#endif // __cpp_lib_memory_resource

// Program which randomly creates and destroys objects in an array and outputs
// the number of objects in the array. Try to find a pattern. :)
int
main()
{
#ifdef __cpp_lib_memory_resource
  // Use a fast memory pool. Thread safety is not required for this single-threaded example.
  std::pmr::monotonic_buffer_resource resource;

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
          // Create an object with the given allocator and save it in the control block for deallocation.
          object_ptr = pntr::allocate_shared<Object>(Object::PntrAllocator(&resource));
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
