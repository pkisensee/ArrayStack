///////////////////////////////////////////////////////////////////////////////
//
//  array_stack.h
//
//  Copyright © Pete Isensee (PKIsensee@msn.com).
//  All rights reserved worldwide.
//
//  Permission to copy, modify, reproduce or redistribute this source code is
//  granted provided the above copyright notice is retained in the resulting 
//  source code.
// 
//  This software is provided "as is" and without any express or implied
//  warranties.
// 
// -----------------------------------------------------------------------------
//
//  Ever needed or wished for std::stack<T, std::array<T,Capacity>>?
// 
//  Then array_stack<T, Capacity> is your friend
// 
//  array_stack<T, Capacity> implements a stack of objects T, stored on the actual 
//  stack using std::array, of maximum size Capacity, compatible with std::stack.
// 
//  Details: https://github.com/pkisensee/ArrayStack/blob/master/README.md
// 
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <cstdint>
#include <ranges>

// Uncomment the following line to enable exceptions, or define this symbol at build time
// #define PK_ENABLE_EXCEPTIONS 1

#if defined(PK_ENABLE_EXCEPTIONS)
  #include <stdexcept>
  #define PK_MAY_THROW noexcept(false) // indicates function may throw
#else
  #define PK_MAY_THROW noexcept(true) // function will not throw
#endif

namespace // anonymous
{

// Requirements for object comparisons; see SynthThreeWay
template <typename T>
concept BooleanTestableImpl = std::convertible_to<T, bool>;

template <typename T>
concept BooleanTestable = BooleanTestableImpl<T>
&& requires( T&& a ) {
  { !static_cast<T&&>( a ) } -> BooleanTestableImpl;
};

// Helper converts input iterators into an array
// Usage: std::array<int, Capacity> arr = MakeArray<InIt, Capacity>( vec.begin(), vec.end() );
template<typename InIt, size_t Capacity>
constexpr auto MakeArray( InIt first, InIt last )
{
  const auto count = std::distance( first, last );
  assert( count <= Capacity );
  using ValType = typename std::iterator_traits<InIt>::value_type;
  std::array<ValType, Capacity> arr;
  std::copy_n( first, count, std::begin( arr ) );
  return arr;
}

// Helper converts range into an array
// Usage: std::array<int, Capacity> arr = MakeArray<Range, Capacity>( rng );
template<typename Range, size_t Capacity>
constexpr auto MakeArray( Range&& rng )
  requires std::ranges::sized_range<decltype( rng )>
{
  const auto count = static_cast<int64_t>( std::ranges::size( rng ) );
  assert( count <= Capacity );
  using ValType = std::iter_value_t<decltype( rng )>;
  std::array<ValType, Capacity> arr;
  std::ranges::copy_n( std::begin( rng ), count, std::begin( arr ) );
  return arr;
}

}; // namespace anonymous

namespace PKIsensee
{

template <typename T, size_t Capacity> // stack of objects T; maximum size Capacity
class array_stack
{
public:

  using Array                   = std::array<T, Capacity>;
  using container_type          = Array;
  using value_type              = typename Array::value_type;
  using reference               = typename Array::reference;
  using const_reference         = typename Array::const_reference;
  using iterator                = typename Array::iterator;
  using const_iterator          = typename Array::const_iterator;
  using reverse_iterator        = typename Array::reverse_iterator;
  using const_reverse_iterator  = typename Array::const_reverse_iterator;
  using size_type               = typename Array::size_type;

  array_stack() = default;

  constexpr explicit array_stack( const Array& c ) :
    top_( c.size() ),
    c_( c )
  {
  }

  template <typename InIt>
  constexpr array_stack( InIt first, InIt last ) noexcept( std::is_nothrow_constructible_v<Array, InIt, InIt> ) :
    top_( static_cast<size_t>( std::distance( first, last ) ) ),
    c_{ MakeArray<InIt, Capacity>( first, last ) }
  {
  }

  template <typename Range>
  constexpr array_stack( std::from_range_t, Range&& rng ) :
    top_( std::size( rng ) ),
    c_( MakeArray<Range, Capacity>( rng ) )
  {
  }

  constexpr iterator begin() noexcept
  {
    return c_.begin();
  }

  constexpr const_iterator begin() const noexcept
  {
    return c_.begin();
  }

  constexpr const_iterator cbegin() const noexcept
  {
    return begin();
  }

  constexpr iterator end() noexcept
  {
    // end of stack is top element, not Capacity
    return begin() + static_cast<ptrdiff_t>( top_ );
  }

  constexpr const_iterator end() const noexcept
  {
    // end of stack is top element, not Capacity
    return cbegin() + static_cast<ptrdiff_t>( top_ );
  }

  constexpr const_iterator cend() const noexcept
  {
    return end();
  }

  constexpr reverse_iterator rbegin() noexcept
  {
    return c_.rbegin();
  }

  constexpr const_reverse_iterator rbegin() const noexcept
  {
    return c_.rbegin();
  }

  constexpr const_reverse_iterator crbegin() const noexcept
  {
    return rbegin();
  }

  constexpr reverse_iterator rend() noexcept
  {
    // end of stack is top element, not Capacity
    return rbegin() + static_cast<ptrdiff_t>( top_ );
  }

  constexpr const_reverse_iterator rend() const noexcept
  {
    // end of stack is top element, not Capacity
    return crbegin() + static_cast<ptrdiff_t>( top_ );
  }

  constexpr const_reverse_iterator crend() const noexcept
  {
    return rend();
  }

  constexpr bool empty() const noexcept
  {
    return top_ == 0;
  }

  constexpr bool full() const noexcept
  {
    return top_ == Capacity;
  }

  constexpr size_type size() const noexcept
  {
    return top_;
  }

  constexpr size_type capacity() const noexcept
  {
    return Capacity;
  }

  constexpr void clear() noexcept
  {
    top_ = 0;
  }

  constexpr reference top() PK_MAY_THROW
  {
    CheckForEmptyStack();
    return c_[top_-1];
  }

  constexpr const_reference top() const PK_MAY_THROW
  {
    CheckForEmptyStack();
    return c_[top_-1];
  }

  constexpr void push( const value_type& v ) PK_MAY_THROW
  {
    CheckForFullStack(1);
    c_[top_] = v;
    ++top_;
  }

  constexpr void push( value_type&& v ) PK_MAY_THROW
  {
    CheckForFullStack(1);
    c_[top_] = std::move( v );
    ++top_;
  }

  template <typename Range>
  constexpr void push_range( Range&& rng ) PK_MAY_THROW
  {
    CheckForFullStack( std::size( rng ) );
    const auto dest = c_.data() + top_;
    std::ranges::copy( rng, dest );
    top_ += std::size( rng );
  }

  template <class... Types>
  constexpr decltype(auto) emplace( Types&&... values ) PK_MAY_THROW
  {
    CheckForFullStack(1);
    const auto dest = c_.data() + top_;
    std::construct_at( dest, std::forward<Types>( values )... );
    ++top_;
  }

  constexpr void pop() PK_MAY_THROW
  {
    CheckForEmptyStack();
    --top_;
  }

  constexpr void swap( array_stack& rhs ) noexcept( std::is_nothrow_swappable<Array>::value )
  {
    // Swap only what is necessary, not the entire arrays
    const auto maxTop = std::max( top_, rhs.top_ );
    for( size_t i = 0; i < maxTop; ++i )
      std::swap( c_[i], rhs.c_[i] );
    std::swap( top_, rhs.top_ );
  }

  constexpr const_reference operator[]( size_type i ) const noexcept
  {
    assert( i < size() );
    return c_[i];
  }

  constexpr reference operator[]( size_type i ) noexcept
  {
    assert( i < size() );
    return c_[i];
  }

private:

  // Synthesize a comparison operation even for objects that don't support <=> operator.
  // Used in operator<=> below
  struct SynthThreeWay
  {
    template <typename U, typename V>
    constexpr auto operator()( const U& lhs, const V& rhs ) const noexcept
      requires requires
      {
        { lhs < rhs } -> BooleanTestable;
        { lhs > rhs } -> BooleanTestable;
      }
    {
      if constexpr( std::three_way_comparable_with<U, V> )
        return lhs <=> rhs; // U supports operator <=>
      else
      { // implement <=> equivalent using existing < and > operators
        if( lhs < rhs )
          return std::strong_ordering::less;
        if( lhs > rhs )
          return std::strong_ordering::greater;
        return std::strong_ordering::equal;
      }
    }
  };

public:

  constexpr bool operator==( const array_stack& rhs ) const noexcept
  {
    if( top_ != rhs.top_ ) // different sized stacks are not equal
      return false;
    const auto start = c_.data();
    const auto end = start + top_;
    return std::equal( start, end, rhs.c_.data() );
  }

  constexpr auto operator<=>( const array_stack& rhs ) const noexcept
  {
    // Can't use std::array::operator<=> because must only compare a subset of elements
    const auto lhsStart = c_.data();
    const auto lhsEnd = lhsStart + top_;
    const auto rhsStart = rhs.c_.data();
    const auto rhsEnd = rhsStart + rhs.top_;
    return std::lexicographical_compare_three_way( lhsStart, lhsEnd, 
                                                   rhsStart, rhsEnd, SynthThreeWay{} );
  }

private:

  void CheckForEmptyStack() const
  {
#if defined(PK_ENABLE_EXCEPTIONS)
    if( empty() )
      throw std::out_of_range( "empty stack" );
#else
    assert( !empty() );
#endif
  }

  void CheckForFullStack( [[maybe_unused]] size_t elementsToAdd ) const
  {
#if defined(PK_ENABLE_EXCEPTIONS)
    if( ( size() + elementsToAdd ) > capacity() )
      throw std::out_of_range( "stack overflow" );
#else
    assert( ( size() + elementsToAdd ) <= capacity() );
#endif
  }

private:

  // top_ points to where the *next* element will be pushed
  // push(x) -> c_[top_] = x; ++top_;
  // pop()   -> --top_;
  // top()   -> return c_[top_-1];
  // empty() -> return top_ == 0;
  // end()   -> return c_ + top_;

  size_t top_ = 0;
  Array c_;

}; // class array_stack

template <typename T, size_t Capacity>
void constexpr swap( array_stack<T, Capacity>& lhs, 
                     array_stack<T, Capacity>& rhs ) noexcept( noexcept( lhs.swap( rhs ) ) )
{
  lhs.swap( rhs );
}

} // namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
