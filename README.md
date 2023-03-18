# `pntr` - Intrusive Shared Pointer for C++17

The project title `pntr` is simply pronounced "pointer".

## Motivation

The [C++ Standard Library](https://en.cppreference.com/w/cpp/standard_library) already offers a great shared smart pointer with [`std::shared_ptr`](https://en.cppreference.com/w/cpp/memory/shared_ptr), so why would anyone consider an alternative? In my opinion, the main reason is its usual size of two raw pointers, which is caused by the additional pointer to the control block. The obvious solution is to integrate the control block into the managed object, for example as used by [Boost's](https://www.boost.org/) [`intrusive_ptr`](https://www.boost.org/doc/libs/1_81_0/libs/smart_ptr/doc/html/smart_ptr.html#intrusive_ptr). But destroying the object will also destroy the control block, which is not compatible with the usual weak pointer implementation. There are solutions for weak pointers with intrusive control blocks using separate objects similar to [`observer_ptr`](https://github.com/vsg-dev/VulkanSceneGraph/blob/master/include/vsg/core/observer_ptr.h) from [VulkanSceneGraph](https://github.com/vsg-dev/VulkanSceneGraph), but this comes with an overhead cost.

I searched for a shared pointer with a size of only one raw pointer that supports weak pointers without additional overhead. I wasn't able to find it, so I decided to implement it myself.

## Features

The `pntr` project offers a shared smart pointer for C++17 with the following features:
- **A size of only one raw pointer**
- **Supports weak pointers without additional overhead**
- A highly configurable intrusive control block with a minimal size of **only one byte** for a combined usage and weak reference counter, which supports:
  - Atomic counters for safe multi-threading
  - Regular counters for performant single-threading
  - Smaller integer types for cases where every byte counts
- Supports custom deleters compatible to [`std::default_delete`](https://en.cppreference.com/w/cpp/memory/default_delete)
- Supports custom allocators compatible to [`std::allocator`](https://en.cppreference.com/w/cpp/memory/allocator)
- Supports custom allocators compatible to [`std::pmr::memory_resource`](https://en.cppreference.com/w/cpp/memory/memory_resource)
- **Header-only library** with CMake integration
- Available as automatically generated [**single header**](single-header/pntr/pntr.hpp) library with embedded license

## Who should use it?

Short answer: If you are not sure that you want it, you don't need it.

For most applications, the existing shared pointers are good enough. The performance and efficiency advantages of my shared pointers are small, but can become significant in specific use cases:
- Projects that use a huge number of shared pointers
  - They can save half of the memory required to store the pointers.
- Projects that use a huge number of shared objects
  - They can save a lot of memory with smaller control blocks.
- Performance-critical projects
  - They can use faster non-atomic counters for thread-local objects.
- Projects which use memory pools
  - My shared pointers are optimized to be integrated with memory pools in several ways.
- Perfectionists
  - It might just feel good to know that your project is optimized as much as possible.

## Project Status

The project is feature-complete for the first release and includes:
- [Single header and its generator](single-header/)
- [Tutorial](tutorial/)
- [Examples](examples/)
- Hundreds of [unit tests](tests/)
- And a small benchmark

`pntr` compiles without any warnings when using the highest warning level, and it successfully passes all unit tests with the following compilers and architectures:
- Windows 10
  - Microsoft Visual C++ 2022 v17.5.2, Compiler v19.35, x86 & x64
  - Clang CL v15.0.7, x86 & x64
  - GCC v12.2.0 (mingw) ucrt64
- Ubuntu 22.04
  - GCC v12.1.0
  - Clang v15.0.6

## Installation

Either simply copy the [**single header**](single-header/pntr/pntr.hpp) into your favorite include directory, or install the header-only library with the following commands (requires git and CMake v3.20 or later):
- `git clone https://github.com/john-plate/pntr.git`
- `cd pntr`
- `cmake -B build [--install-prefix <install-path>]`
- `cmake --install build`

After installation just use `find_package(pntr)` and link to the target `pntr`.

[Examples for using the CMake integration](https://github.com/john-plate/pntr-cmake-example/) are available in a separate repository.

## Building

To build the tutorial, the examples, and the unit tests, I recommend using the provided CMake presets, which require the [Ninja build tool](https://ninja-build.org/) and the environment variables `USERBUILD` and `USERINSTALL` for the build and install base directories. For mingw GCC, you also need to set `MSYS2_ROOT`. The unit tests will fetch [Catch2](https://github.com/catchorg/Catch2), and require CMake v3.24 or later. Then you can configure, build, and test everything with presets, e.g.:
- `cd pntr`
- `cmake --preset msvc-x64`
- `cmake --build --preset msvc-x64-release`
- `ctest --preset msvc-x64`

The presets are available for all platforms listed under [Project Status](#project-status).

## The Formal Stuff

### License

The `pntr` project is released under the terms of the [MIT license](LICENSE).

### Contributing

[Appreciated!](CONTRIBUTING.md)

### Code of Conduct

[Behave!](CODE_OF_CONDUCT.md)

## Feedback

It would be great if everybody who uses `pntr` sends me an email with feedback, for example:
- For what kind of project are you using it? Private or professional?
- Are you able to experience or even measure an improvement? Performance and/or happiness?
- May I publish your or your company's name in a list of users of `pntr`?

## Frequently Asked Questions

Nothing yet.

## Acknowledgements

Thanks to my friend Gwen Pech for proofreading!
