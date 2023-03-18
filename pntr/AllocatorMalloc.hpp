// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/common.hpp>

#include <cstdlib>

PNTR_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//                                        AllocatorMalloc                                         //
//                                                                                                //
//                    An allocator for 'ControlAlloc' which uses 'std::malloc'                    //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  'AllocatorMalloc' uses 'std::malloc' and related functions to allocate memory. 'ControlAlloc'
//  can save a pointer offset between a created object and its shared base, which makes it possible
//  to restore and deallocate the original pointer without storing the object's type information.
//
//  The number of bits used for the stored pointer offset is configurable in the control data block
//  and can usually be quite small to minimize the intrusive control block size. It can even be set
//  to zero for classes where the shared base class is at the front of the memory layout of the
//  created class.
//
//  The template parameter can be configured with 'StaticSupport' to enable the proper destruction
//  of non-polymorphic classes. In this case the control block will store an additonal pointer to
//  save type information.
//
//  The Microsoft Standard Library does not support 'std::aligned_alloc', see
//  https://en.cppreference.com/w/cpp/memory/c/aligned_alloc
//  Instead it provides '_aligned_malloc', which is not compatible with 'std::free'. It requires
//  to use '_aligned_free', but 'AllocatorMalloc::deallocate' doesn't have alignment information
//  of the created type. It would be possible to always use '_alligned_malloc' and '_aligned_free',
//  but this would probably cause a performance penalty. So, it is necessary to use a bit in the
//  pointer offset to store the information which function has to be used to free the memory block.
//  A static assertion ensures that this bit is available for types with special alignment.
//

template<class t_static_support = NoStaticSupport>
class AllocatorMalloc
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
  using PointerDeallocate = void;

  // Allocate a memory block.
  void *
  allocate(std::size_t p_size, std::size_t p_alignment) noexcept
  {
    void * const storage = (p_alignment <= alignof(std::max_align_t) ? std::malloc(p_size)
#ifdef _WIN32
                                                                     : _aligned_malloc(p_size, p_alignment));
#else
                                                                     : std::aligned_alloc(p_alignment, p_size));
#endif

#ifdef PNTR_UNITTESTS
    s_storage = storage;
#endif

    return storage;
  }

#ifdef _WIN32

  // Deallocate a memory block.
  void
  deallocate(void * p_pointer, bool p_is_special_aligned) noexcept
  {
  #ifdef PNTR_UNITTESTS
    if (s_storage == p_pointer)
    {
      s_storage = nullptr;
    }
  #endif

    if (p_is_special_aligned)
    {
      _aligned_free(p_pointer);
    }
    else
    {
      std::free(p_pointer);
    }
  }

#else

  // Deallocate a memory block.
  void
  deallocate(void * p_pointer) noexcept
  {
  #ifdef PNTR_UNITTESTS
    if (s_storage == p_pointer)
    {
      s_storage = nullptr;
    }
  #endif

    std::free(p_pointer);
  }

#endif

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //       The type definitions and member functions required by 'ControlAlloc' end here        //
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PNTR_UNITTESTS
  AllocatorMalloc() noexcept
  {
    ++g_allocator_construct;
  }

  AllocatorMalloc(AllocatorMalloc const &) noexcept
  {
    ++g_allocator_construct;
  }

  AllocatorMalloc(AllocatorMalloc &&) noexcept
  {
    ++g_allocator_construct;
  }

  ~AllocatorMalloc() noexcept
  {
    ++g_allocator_destroy;
  }

  AllocatorMalloc & operator=(AllocatorMalloc &&) = default;
  AllocatorMalloc & operator=(AllocatorMalloc const &) = default;

  inline static void * s_storage = nullptr;
#endif
};


PNTR_NAMESPACE_END
