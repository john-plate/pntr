# `pntr` Examples

## Object Pool

The [object pool example](https://github.com/john-plate/pntr/blob/main/examples/pntr-object-pool.cpp) demonstrates how a simple custom deleter is able to return objects to a pool when their lifetime has expired.
## Memory Pool

The [memory pool example](https://github.com/john-plate/pntr/blob/main/examples/pntr-memory-pool.cpp) demonstrates how to use a `pntr::SharedPtr` with a `std::pmr::memory_resource` pool.

## Custom Memory Pool

The [custom memory pool example](https://github.com/john-plate/pntr/blob/main/examples/pntr-custom-memory-pool.cpp) demonstrates how to use a custom allocator with a `std::pmr::memory_resource` pool to achieve maximum efficiency.

## More examples?

Please feel free to send me ideas for more examples.
