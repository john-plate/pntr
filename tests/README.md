# `pntr` - Unit Tests

The unit tests include:
- The regular and the atomic counters, each with 8 different integer types.
- The control block data with both counter types, each tested with 34 bit size combinations.
- The control block for deleters, tested with 5 deleter types, each tested with 12 object types.
  - Including tests of proper construction and destruction for both the objects and deleters.
- The control block for allocators, tested with 6 allocator types, each tested with 12 object types.
  - Including tests of proper construction and destruction for both the objects and allocators.
- The object types include classes that are polymorphic, non-polymorphic, virtually derived, constant, and with non-standard alignment.
- All functions of the shared pointer with both control block types.
- All functions of the weak pointer with the allocator control block.

All unit tests are executed twice, once using the regular headers, and once with the single header.

Please let me know if you find any functionality which is not covered yet.
