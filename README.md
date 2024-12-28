Ever needed or wished for std::stack<T, std::array<T,Capacity>>?

Then array_stack<T, Capacity> is your friend

array_stack<T, Capacity> implements a stack of objects T, stored on the actual 
stack using std::array, of maximum size Capacity, compatible with std::stack

Features:
* all std::stack methods
* constexpr enabled
* non-throwing by default
* full(), capacity(), and clear()
* direct indexing using operator[] (not part of std::stack, but often useful)
* comparison operations
* swap
* range support
* optional support for throwing exceptions from push(), pop(), top()

Doesn't support:
* custom allocators
* constructing from std::array; use the range or input iterator ctors instead
 
Requirements:
* C++23 and up
