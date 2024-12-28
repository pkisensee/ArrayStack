///////////////////////////////////////////////////////////////////////////////
//
//  ArrayStack.h
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
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <cstdint>
#include <ranges>

namespace // anonymous
{

// Define requirements for object comparisons; see SynthThreeWay
template <typename T>
concept BooleanTestableImpl = std::convertible_to<T, bool>;

template <typename T>
concept BooleanTestable = BooleanTestableImpl<T>
&& requires( T&& a ) {
  { !static_cast<T&&>( a ) } -> BooleanTestableImpl;
};

// Helper converts input iterators into an array
// Usage: std::array<int, N> arr = MakeArray<InIt, N>( vec.begin(), vec.end() );
template<typename InIt, size_t N>
auto MakeArray( InIt first, InIt last )
{
  auto count = std::distance( first, last );
  assert( count <= N );
  using ValType = typename std::iterator_traits<InIt>::value_type;
  std::array<ValType, N> arr;
  std::copy_n( first, count, std::begin( arr ) );
  return arr;
}

// Helper converts range into an array
// Usage: std::array<int, N> arr = MakeArray<Range, N>( rng );
template<typename Range, size_t N>
constexpr auto MakeArray( Range&& rng )
  requires std::ranges::sized_range<decltype( rng )>
{
  auto count = static_cast<int64_t>( std::ranges::size( rng ) );
  using ValType = std::iter_value_t<decltype( rng )>;
  std::array<ValType, N> arr;
  std::ranges::copy_n( std::begin( rng ), count, std::begin( arr ) );
  return arr;
}

}; // namespace anonymous

namespace PKIsensee
{

///////////////////////////////////////////////////////////////////////////////
//
// Ever needed/wanted std::stack<T, std::array<T,N>>?
// Then ArrayStack<T, N> is your friend
// 
// Features:
// * all std::stack methods
// * constexpr enabled
// * non-throwing
// * full(), capacity(), and clear()
// * direct indexing using operator[] (not part of std::stack, but often useful)
// * comparison operations
// * swap
// * range support
// 
// Doesn't support:
// * custom allocators
// * constructing from std::array; use the range or input iterator ctors instead
// * exceptions from push(), pop(), top()
// 
// Requirements:
// * C++20 and up
// 
///////////////////////////////////////////////////////////////////////////////

template <typename T, size_t N>
class ArrayStack
{
public:

  using Array           = std::array<T, N>;
  using container_type  = Array;
  using value_type      = typename Array::value_type;
  using reference       = typename Array::reference;
  using const_reference = typename Array::const_reference;
  using size_type       = typename Array::size_type;

  ArrayStack() = default;

  constexpr explicit ArrayStack( const Array& c ) :
    c_( c ),
    top_( c.size() )
  {
  }

  template <typename InIt>
  constexpr ArrayStack( InIt first, InIt last ) noexcept( std::is_nothrow_constructible_v<Array, InIt, InIt> ) :
    c_{ MakeArray<InIt, N>( first, last ) },
    top_( static_cast<size_t>( std::distance( first, last ) ) )
  {
  }

  template <typename Range>
  constexpr ArrayStack( std::from_range_t, Range&& rng ) :
    c_( MakeArray<Range, N>( rng ) ),
    top_( rng.size() )
  {
  }

  constexpr bool empty() const noexcept
  {
    return top_ == 0;
  }

  constexpr bool full() const noexcept
  {
    return top_ == N;
  }

  constexpr size_type size() const noexcept
  {
    return top_;
  }

  constexpr size_type capacity() const noexcept
  {
    return N;
  }

  constexpr void clear() noexcept
  {
    top_ = 0;
  }

  constexpr reference top() noexcept
  {
    // if( empty() ) throw std::out_of_range( "empty stack" );
    assert( !empty() );
    return c_[top_-1];
  }

  constexpr const_reference top() const noexcept
  {
    // if( empty() ) throw std::out_of_range( "empty stack" );
    assert( !empty() );
    return c_[top_-1];
  }

  constexpr void push( const value_type& v ) noexcept
  {
    // if( full() ) throw std::out_of_range( "stack overflow" );
    assert( !full() );
    c_[top_] = v;
    ++top_;
  }

  constexpr void push( value_type&& v ) noexcept
  {
    // if( full() ) throw std::out_of_range( "stack overflow" );
    assert( !full() );
    c_[top_] = std::move( v );
    ++top_;
  }

  template <typename Range>
  constexpr void push_range( Range&& rng )
  {
    // if( ( size() + std::size( rng ) ) > capacity() ) throw std::out_of_range( "stack overflow" );
    assert( ( size() + std::size( rng ) ) <= capacity() );
    auto dest = c_.data() + top_;
    std::ranges::copy( rng, dest );
    top_ += std::size( rng );
  }

  template <class... Types>
  constexpr decltype(auto) emplace( Types&&... values )
  {
    // if( full() ) throw std::out_of_range( "stack overflow" );
    auto dest = c_.data() + top_;
    std::construct_at( dest, std::forward<Types>( values )... );
    ++top_;
  }

  constexpr void pop() noexcept
  {
    // if( empty() ) throw std::out_of_range( "stack underflow" );
    assert( !empty() );
    --top_;
  }

  constexpr void swap( ArrayStack& rhs ) noexcept( std::is_nothrow_swappable<Array>::value )
  {
    // Swap only what is necessary, not the entire arrays
    auto maxTop = std::max( top_, rhs.top_ );
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

  // Synthesize a comparison operation even for those objects that don't support <=> operator.
  // Used in operator<=> above
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

  constexpr bool operator==( const ArrayStack& rhs ) const noexcept
  {
    if( top_ != rhs.top_ ) // different sized stacks are not equal
      return false;
    const auto start = c_.data();
    const auto end = start + top_;
    return std::equal( start, end, rhs.c_.data() );
  }

  constexpr auto operator<=>( const ArrayStack& rhs ) const noexcept
  {
    // Can't use std::array::operator<=> because must only compare a subset of elements
    const auto lstart = c_.data();
    const auto lend = lstart + top_;
    const auto rstart = rhs.c_.data();
    const auto rend = rstart + rhs.top_;
    return std::lexicographical_compare_three_way( lstart, lend, rstart, rend, SynthThreeWay{} );
  }

private:

  // For efficiency, top_ points to where the *next* element will be pushed
  // push(x) -> c_[top_] = x; ++top_;
  // pop()   -> --top_;
  // top()   -> return c_[top_-1];
  // empty() -> return top_ == 0;

  Array c_;
  size_t top_ = 0;

}; // class ArrayStack

template <typename T, size_t N>
void constexpr swap( ArrayStack<T, N>& lhs, ArrayStack<T, N>& rhs ) noexcept( noexcept( lhs.swap( rhs ) ) )
{
  lhs.swap( rhs );
}

} // namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
