// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/common.hpp>

#include <version>
#ifdef __cpp_lib_memory_resource
  #include <memory_resource>

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                    AllocatorMemoryResource                                     //
//                                                                                                //
//             An allocator for 'ControlAlloc' which uses 'std::pmr::memory_resource'             //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  'AllocatorMemoryResource' uses 'std::pmr::memory_resource' to allocate memory. 'ControlAlloc'
//  can save a pointer offset between a created object and its shared base, and also the type size
//  and alignment required for deallocation, which makes it possible to restore and deallocate the
//  original pointer and type details without storing the object's actual type.
//
//  The number of bits used for the stored pointer offset is configurable in the control data block
//  and can usually be quite small to minimize the intrusive control block size. It can even be set
//  to zero for classes where the shared base class is at the front of the memory layout of the
//  created class.
//
//  To size is restored using a factor of the control block alignment for the difference between
//  shared base class and shared class. The number of bits for this factor can be zero if no
//  derived class instances are used. The number of bits for the alignment can be set to zero for
//  types without special alignment. A static assertion ensures that enough bits are configured.
//
//  The template parameter can be configured with 'StaticSupport' to enable the proper destruction
//  of non-polymorphic classes. In this case the control block will store an additonal pointer to
//  save type information, which can be used to restore size and alignment details, so no bits for
//  type size and alignment are required.
//

template<class t_static_support = NoStaticSupport>
class AllocatorMemoryResource
{
public:
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //      The type definitions and member functions required by 'ControlAlloc' start here       //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // 'SupportsStatic' has to be either 'StaticSupport' or 'NoStaticSupport'. The former will
  // request 'ControlAlloc' to store type information in an additonal pointer in the control
  // block that supports the correct object destruction and deallocation from shared pointers
  // to non-polymorphic base classes.
  using SupportsStatic = t_static_support;

  // 'ControlAlloc' identifies the type of this allocator with this type definition.
  using TypeInfoDeallocate = void;

  // Allocate a memory block.
  void *
  allocate(std::size_t p_size, std::size_t p_alignment) noexcept
  {
    void * const storage = m_resource->allocate(p_size, p_alignment);
  #ifdef PNTR_UNITTESTS
    s_storage = storage;
    s_size = p_size;
    s_align = p_alignment;
  #endif
    return storage;
  }

  // Deallocate a memory block.
  void
  deallocate(void * p_pointer, std::size_t p_size, std::size_t p_alignment) noexcept
  {
  #ifdef PNTR_UNITTESTS
    if (s_storage == p_pointer && s_size == p_size && s_align == p_alignment)
    {
      s_storage = nullptr;
    }
  #endif
    m_resource->deallocate(p_pointer, p_size, p_alignment);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //       The type definitions and member functions required by 'ControlAlloc' end here        //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

  AllocatorMemoryResource(std::pmr::memory_resource * p_resource) noexcept
  : m_resource(p_resource)
  {
    PNTR_ASSERT(p_resource != nullptr);
  }

  std::pmr::memory_resource *
  resource() const noexcept
  {
    return m_resource;
  }

  #ifdef PNTR_UNITTESTS
  AllocatorMemoryResource() noexcept
  {
    ++g_allocator_construct;
  }

  AllocatorMemoryResource(AllocatorMemoryResource const & p_allocator) noexcept
  : m_resource(p_allocator.m_resource)
  {
    ++g_allocator_construct;
  }

  AllocatorMemoryResource(AllocatorMemoryResource && p_allocator) noexcept
  : m_resource(p_allocator.m_resource)
  {
    ++g_allocator_construct;
  }

  ~AllocatorMemoryResource() noexcept
  {
    ++g_allocator_destroy;
  }

  inline static void * s_storage = nullptr;
  inline static std::size_t s_size = 0u;
  inline static std::size_t s_align = 0u;

  #else

  AllocatorMemoryResource() noexcept = default;
  AllocatorMemoryResource(AllocatorMemoryResource const &) noexcept = default;
  AllocatorMemoryResource(AllocatorMemoryResource &&) noexcept = default;

  #endif

  AllocatorMemoryResource & operator=(AllocatorMemoryResource &&) = default;
  AllocatorMemoryResource & operator=(AllocatorMemoryResource const &) = default;

private:
  std::pmr::memory_resource * m_resource = std::pmr::get_default_resource();
};


PNTR_NAMESPACE_END

#endif // __cpp_lib_memory_resource
