# array_stack

If you've ever needed or wished for `std::stack<T, std::array<T, Capacity>>`, then `array_stack<T, Capacity>` is your friend.

`array_stack<T, Capacity>` implements a stack of objects `T`, stored on the actual 
stack using [`std::array`](https://en.cppreference.com/w/cpp/container/array), of maximum size `Capacity`, compatible with [`std::stack`](https://en.cppreference.com/w/cpp/container/stack).

Features:
* Header-only implementation; no dependencies other than standard C++ headers
* supports all `std::stack` methods through C++23
* constexpr enabled
* `full()`, `capacity()`, and `clear()`
* `operator[]` (not part of `std::stack`, but often useful)
* `begin()/end()` and friends for algorithm and range operations
* comparison operations
* swap
* range support
* non-throwing by default, with optional support for throwing exceptions from `push()`, `pop()`, and `top()`

Doesn't support:
* custom allocators
* constructing from `std::array`; use the range or input iterators instead
 
Requirements:
* C++23 and up

Notes:
* On Windows, the default stack size is typically 1MB, but can be changed in linker settings
* On Linux, the default is typically 8MB, also adjustable in various ways
* On MacOS, the default is typically 8MB for the app, and 512KB per thread


