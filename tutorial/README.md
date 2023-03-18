# `pntr` - Tutorial

This tutorial assumes that you are familiar with modern C++ (C++11 or later) and the smart pointers provided by the [C++ Standard Library](https://en.cppreference.com/w/cpp/standard_library). If not, please first search and read some of the available great smart pointer tutorials.

The source code contains a lot of documentation. If you have any question after reading the tutorial, please check the source code first before you ask on the project site.

## The smart pointers

The `pntr` project contains the two smart pointers [`SharedPtr`](https://github.com/john-plate/pntr/blob/main/pntr/SharedPtr.hpp) and [`WeakPtr`](https://github.com/john-plate/pntr/blob/main/pntr/WeakPtr.hpp), which are as compatible as possible to [`std::shared_ptr`](https://en.cppreference.com/w/cpp/memory/shared_ptr) and [`std::weak_ptr`](https://en.cppreference.com/w/cpp/memory/weak_ptr). The main differences are:
- They have only **the size of one raw pointer**, while the shared pointers of the Standard Library typically have the size of two raw pointers.
- They require that the control block which includes the reference counters is embedded in the managed object, while the control block of the Standard Library is typically either a separate object or is constructed in a memory block which is shared with the managed object.

## The magic of the control block

This is the "behind the scenes" part. Feel free to skip to [How to use the control block?](#how-to-use-the-control-block).

### `Intruder`

The purpose of the [`Intruder`](https://github.com/john-plate/pntr/blob/main/pntr/Intruder.hpp) class is to:
- Be the parent class for any object (or any of its base classes) that likes to be managed with a `SharedPtr`.
- Reserve a memory block for the control block.
- Construct the control block together with the managed object.
- Provide a unified interface for the smart pointers to use the different control block types.
- In specific cases destroy the control block together with the managed object.
- Provide the functionality of [`std::enable_shared_from_this`](https://en.cppreference.com/w/cpp/memory/enable_shared_from_this) without additional overhead.

`SharedPtr` does not depend on the class `Intruder`; only on its functionality. So, `SharedPtr` and `WeakPtr` can be used without modification with any class that provides the required type and function definitions.

Using the same principle, you can also use custom implementations for other classes, like the counter or the control block. But in most cases the only custom classes will be custom deleter and allocator classes, if any.

### `ControlNew`

The control block [`ControlNew`](https://github.com/john-plate/pntr/blob/main/pntr/ControlNew.hpp) contains a usage counter and a deleter. It does not support weak pointers, because with an intrusive control block they require to separate an object's destruction and deallocation, which is not possible with a deleter. This is also the reason why the Standard Library has `std::allocate_shared` to allocate a combined memory block for the control block and the managed object (to save the overhead of one memory allocation), but it doesn't have a similar function for a deleter. `pntr` doesn't have that limitation and is able to support a deleter while saving that extra memory allocation.

`ControlNew` creates objects with a regular `new` expression and should accept any deleter that would also be accepted by `std::shared_ptr`.

If the deleter is an empty class like `std::default_delete`, the size of the control block is only determined by the usage counter and can be as **small as one byte**.

### `ControlAlloc`

The control block [`ControlAlloc`](https://github.com/john-plate/pntr/blob/main/pntr/ControlAlloc.hpp) contains a usage counter, an optional weak counter, an allocator, and optionally more type information. The challenge for `ControlAlloc` is to provide all information required by the allocator's deallocate function after the managed object is already destroyed.

`ControlAlloc` supports three types of allocators:
- Allocators which only require the original untyped pointer for deallocation. The most common example is [`std::free`](https://en.cppreference.com/w/cpp/memory/c/free). For these allocators it is only required to restore the offset between the control block and memory block pointer.
- Allocators which require the original untyped pointer and the size and alignment of the allocated memory block for deallocation, for example [`std::pmr::memory_resource`](https://en.cppreference.com/w/cpp/memory/memory_resource).
- Allocators which require the original type for deallocation, for example [`std::allocator`](https://en.cppreference.com/w/cpp/memory/allocator). Those allocators require to store an extra pointer in the control block, which points to an instanced template function that destroys or deallocates the correctly type-casted pointer.

If you use one of the first two allocators with an allocator that is an empty class like [`AllocatorMalloc`](https://github.com/john-plate/pntr/blob/main/pntr/AllocatorMalloc.hpp), the size of the control block is only determined by the control block data and can be as **small as one byte**.

### Base classes without virtual destructor

It can also be a challenge to call the right destructor. If you delete a raw pointer to a base class without virtual destructor, the destructor of a derived class won't be called. Both control block types provide solutions for those non-polymorphic classes, which are simply called "static" here:
- `ControlNew` provides a custom [`Deleter`](https://github.com/john-plate/pntr/blob/main/pntr/Deleter.hpp), which stores an extra pointer that points to an instanced type-specific destroy function. It ensures that the deleter is always called on the originally created type. By the way, a polymorphic class has the same overhead of one pointer, so I recommend to prefer the solution to use a virtual destructor in the base class if possible.
- `ControlAlloc` can be configured with the type definition `SupportsStatic` in the allocator. The default is `NoStaticSupport`. When configured with `StaticSupport`, `ControlAlloc` will also store an extra pointer to an instanced type-specific destroy and deallocate function. The allocator types compatible to `std::allocator` don't need that definition as they require that extra pointer anyway.  
Though storing an extra pointer is a bit of overhead, `SupportsStatic` also comes with advantages: It doesn't require bits for the size and alignment in the control block, and the pointer offset can be calculated after the managed object is expired, which makes it possible to store the offset in the usage counter, as this is not required after it reaches zero. This feature is enabled by configuring `pntr::shared_bits` for the offset bits. It has to be done manually as there has to be an option to disable the feature if more offset bits are required than available in the usage counter bits.

### `ControlData`

[`ControlData`](https://github.com/john-plate/pntr/blob/main/pntr/ControlData.hpp) stores the integer data for the control block and combines up to six values in up to 64 bit. Please see the source code for their documentation.

## How to use the control block?

The control block is highy configurable and contains complex template classes with many parameters, that most users might never need. The [main header file](https://github.com/john-plate/pntr/blob/main/pntr/pntr.hpp) defines some template aliases with useful default values, which should hide the complexity for most use cases.

The first templete parameter of the `Intruder` aliases is always the base class which should be managed. The second is always the thread safety, with `ThreadSafe` as default, which uses atomic counters. Please use `ThreadUnsafe` is you prefer faster regular counters for managing thread-local objects.

The [tutorial source file](pntr-tutorial.cpp) contains functions with examples, which are described below.

### `tutorial_intruder_new()`

`IntruderNew` is an alias for an `Intruder` with the `ControlNew` control block, which is the recommended choice if no `WeakPtr` is needed. The alias allows to configure the thread safety (default = `ThreadSafe`), the control data value type (default = `std::uint32_t`), the number of bits for the usage counter (default = all available bits), and the deleter type (default = [`std::default_delete`](https://en.cppreference.com/w/cpp/memory/default_delete)).

The size of `IntruderNew` is the combined size of the counter, the deleter, and maybe alignment padding. So, by default the size is four bytes for the 32 bit counter and zero bytes for the empty class `std::default_delete`.

In the tutorial function the class `PolymorphicSharedBase` is derived from `IntruderNew`. The control block always requires the managed base class as parameter, so this is always the first. The remaining parameters are not used to select the defaults.

Note: The managed base class is not allowed to inherit the Intruder virtually. You can use an intermediate helper class if you need to inherit the Intruder virtually.

The function shows first how an object of the derived class `PolymorphicShared` is successfully created, and also successfully destroyed using a shared pointer to the base class.

The second example in the function shows how the non-polymorphic derived class `StaticShared` is successfully created and destroyed using a shared pointer to the derived class.

In the third example it shows how the destruction fails to call the derived destructor if a non-polymorphic class is deleted using a pointer to the base class.

### `tutorial_intruder_new_static()`

This function shows how to use the alias `IntruderNewStatic`, which exists specifically to support non-polymorphic class hierarchies like the last example in the previous function. The alias is pre-configured with the `Deleter` for that purpose. The default control data value type has the size of one pointer, so no padding is lost due to the extra pointer stored in the deleter. The usage counter has 32 bits by default, which leaves 32 bits for the user value (see `ControlData`) on architectures with 64 bit pointers.

The default size of `IntruderNewStatic` is the size of two raw pointers.

In the function output you can see how the destructor of the `StaticShared` class is successfully called using a pointer to the base class. The deleter works even when it is default constructed by the control block.

### `tutorial_intruder_alloc()`

`IntruderAlloc` is an alias for an `Intruder` with the `ControlAlloc` control block, which is required to support `WeakPtr`. The alias allows to configure:
- Thread safety (default = `ThreadSafe`)
- The control data value type (default = `std::uint64_t`)
- The number of bits for the usage counter (default = 32)
- The number of bits for the weak counter (default = 16)
- The number of bits for the pointer offset (default = 16)
- The number of bits for the size offset (default = 0)
- The number of bits for the alignment offset (default = 0)
- Implicitly the number of bits for the user value (default = 0)
- The allocator type (default = `AllocatorMalloc<NoStaticSupport>`)

The three examples are similar to `tutorial_intruder_new()` and show the same issue with a non-polymorphic class.

### `tutorial_intruder_malloc_static()`

Similar as in `tutorial_intruder_new_static()` you can use `IntruderMallocStatic` with non-polymorphic classes. As explained before, this requires an extra pointer in the control block, but we can use the whole control data for reference counting as the offset can be shared with the usage count, see `shared_bits` in the alias definition.

### `tutorial_check_intruder_efficiency()`

The configurability of the control block can lead to situations where you are not sure if you have configured values that might not be needed, or if some values might require fewer bits. For this case you can use the function `check_intruder_efficiency()` defined in the [main header file](https://github.com/john-plate/pntr/blob/main/pntr/pntr.hpp).

Of course the function can't predict how many usage and weak references your application will use, so it will only complain if more than 32 bits are configured for the reference counters. It is hard to imagine a use case with more than four billion shared pointers to the same object.

The first example in the tutorial function shows an `Intruder` configured with `ControlNew`, control data with bits for usage and weak counter, and for offset, size, alignment, and with a deleter. The output of `check_intruder_efficiency()` reveals:
```
Checking the Intruder efficiency, example 1:
Padding detected. You can increase the control data value type to 8 bytes.
The weak bits should be zero because weak pointers are not supported.
The offset bits are not needed and should be zero.
The size bits are not needed and should be zero.
The alignment bits are not needed and should be zero.
The 'pntr::Deleter' is not required. Prefer to use 'std::default_delete'
```

The second example uses an `IntruderAlloc` configured with bits for usage and weak counter, and for offset, size and alignment. The output of `check_intruder_efficiency()` reveals:
```
Checking the Intruder efficiency, example 2:
The offset bits of 8 can be reduced to 0 to store the pointer offset of 0.
The size bits are not needed and should be zero.
The alignment bits are not needed and should be zero.
```

The third example uses an `IntruderAlloc` configured with `std::allocator` and bits for usage and weak counter, and for offset, size and alignment. The output of `check_intruder_efficiency()` reveals:
```
Checking the Intruder efficiency, example 3:
Padding detected. You can increase the control data value type to 8 bytes.
It would be more efficient to configure the offset bits as 'pntr::shared_bits'.
The size bits are not needed and should be zero.
The alignment bits are not needed and should be zero.
```

The fourth example uses an `IntruderAlloc` configured with `pntr::AllocatorMemoryResource` and bits for usage and weak counter, and for offset, size and alignment. The output of `check_intruder_efficiency()` reveals:
```
Checking the Intruder efficiency, example 4:
The offset bits of 12 can be reduced to 3 to store the pointer offset of 4.
The size bits of 12 can be reduced to 3 to store the size offset of 6.
The alignment bits of 12 can be reduced to 2 to store the alignment offset of 2.
```

And in the fifth example we use the output of the fourth example to efficiently configure the same control block. The output of `check_intruder_efficiency()` confirms:
```
Checking the Intruder efficiency, example 5:
Maximum efficiency confirmed.
```

If you have a complex class hierarchie you might have to check the efficiency of multiple derived class to find the maximum offset values required to support all classes in the hierarchie.

## More?

You can find more advanced topics in the [examples](https://github.com/john-plate/pntr/blob/main/examples/), including custom deleters and allocators.

I might extend the tutorial on request, or if I have more ideas based on the feedback.
