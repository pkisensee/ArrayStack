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
  int64_t count = static_cast<int64_t>( std::ranges::size( rng ) );
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
// * std::stack methods
// * constexpr enabled
// * non-throwing
// * full()
// * capacity()
// * clear()
// * direct indexing using operator[] (not part of std::stack, but sometimes useful)
// * spaceship comparison operator
// * swap
// * ranges
// 
// Doesn't support:
// * custom allocators
// * constructing from std::array; use the range or input iterator ctors instead
// * exceptions from push(), pop(), top()
// 
///////////////////////////////////////////////////////////////////////////////

template<typename T, size_t N>
class ArrayStack
{
public:

  static constexpr size_t kEmptyStackDesignation = size_t( -1 );

  using Array = std::array<T, N>;
  using value_type = typename Array::value_type;
  using reference = typename Array::reference;
  using const_reference = typename Array::const_reference;
  using size_type = typename Array::size_type;
  using container_type = Array;

  ArrayStack() = default;

  constexpr explicit ArrayStack( const Array& c ) :
    c_( c ),
    top_( c.empty() ? kEmptyStackDesignation : (c.size() - 1) )
  {
  }

  template <typename InIt>
  constexpr ArrayStack( InIt first, InIt last ) noexcept( std::is_nothrow_constructible_v<Array, InIt, InIt> ) :
    c_{ MakeArray<InIt, N>( first, last ) },
    top_( first == last ? kEmptyStackDesignation : (std::distance( first, last ) - 1 ))
  {
  }

  template <typename Range>
  constexpr ArrayStack( std::from_range_t, Range&& rng ) :
    c_( MakeArray<Range, N>( rng ) ),
    top_( rng.empty() ? kEmptyStackDesignation : (rng.size() - 1) )
  {
  }

  constexpr bool empty() const noexcept
  {
    return top_ == kEmptyStackDesignation;
  }

  constexpr bool full() const noexcept
  {
    return top_ == (N - 1);
  }

  constexpr size_type size() const noexcept
  {
    return top_ + 1;
  }

  constexpr size_type capacity() const noexcept
  {
    return N;
  }

  constexpr void clear() noexcept
  {
    top_ = kEmptyStackDesignation;
  }

  constexpr reference top() noexcept
  {
    //if( empty() )
    //  throw std::out_of_range( "empty stack" );
    assert( !empty() );
    return c_[top_];
  }

  constexpr const_reference top() const noexcept
  {
    //if( empty() )
    //  throw std::out_of_range( "empty stack" );
    assert( !empty() );
    return c_[top_];
  }

  constexpr void push( const value_type& v ) noexcept
  {
    //if( full() )
    //  throw std::out_of_range( "stack overflow" );
    assert( !full() );
    c_[++top_] = v;
  }

  constexpr void push( value_type&& v ) noexcept
  {
    //if( full() )
    //  throw std::out_of_range( "stack overflow" );
    assert( !full() );
    c_[++top_] = std::move( v );
  }

  template <typename Range>
  constexpr void push_range( Range&& rng )
  {
    //if( ( size() + std::size( rng ) ) > capacity() )
    //  throw std::out_of_range( "stack overflow" );
    assert( ( size() + std::size( rng ) ) <= capacity() );
    auto dest = &( c_[top_ + 1] );
    std::ranges::copy( rng, dest );
    top_ += std::size( rng );
  }

  template <class... Types>
  constexpr decltype(auto) emplace( Types&&... values )
  {
    //if( full() )
    //  throw std::out_of_range( "stack overflow" );
    auto dest = &( c_[++top_] );
    std::construct_at( dest, std::forward<Types>( values )... );
  }

  constexpr void pop() noexcept
  {
    //if( empty() )
    //  throw std::out_of_range( "stack underflow" );
    assert( !empty() );
    --top_;
  }

  constexpr void swap( ArrayStack& rhs ) noexcept( std::is_nothrow_swappable<Array>::value )
  {
    // Swap the minimum portion necessary; +1 avoids dealing with kEmptyStackDesignation
    auto maxTop = std::max( top_+1, rhs.top_+1 );
    assert( maxTop <= N );
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

  constexpr bool operator==( const ArrayStack& rhs ) const noexcept
  {
    if( top_ != rhs.top_ ) // size mismatch
      return false;
    if( top_ == ArrayStack<T, N>::kEmptyStackDesignation ) // both empty
      return true;
    const auto start = std::begin( c_ );
    const auto end = start + static_cast<ptrdiff_t>( top_+1 );
    return std::equal( start, end, std::begin( rhs.c_ ) );
  }

  struct SynthThreeWay
  {
    template <typename T, std::totally_ordered_with<T> U>
    constexpr auto operator()( const T& lhs, const U& rhs ) const noexcept
    {
      if constexpr( std::three_way_comparable_with<T, U> )
        return lhs <=> rhs;
      else
      {
        if( lhs < rhs )
          return std::strong_ordering::less;
        else if( lhs > rhs )
          return std::strong_ordering::greater;
        return std::strong_ordering::equal;
      }
    }
  };

  constexpr auto operator<=>( const ArrayStack& rhs ) const noexcept
  {
    // Same number of elements
    if( top_ == rhs.top_ )
    {
      if( top_ == ArrayStack<T, N>::kEmptyStackDesignation ) // both empty
        return std::strong_ordering::equal;

      const auto lstart = std::begin( c_ );
      const auto lend = lstart + static_cast<ptrdiff_t>( top_+1 );
      const auto rstart = std::begin( rhs.c_ );
      const auto rend = rstart + static_cast<ptrdiff_t>( top_+1 );

      return std::lexicographical_compare_three_way( lstart, lend, rstart, rend,
                                                     SynthThreeWay{} );
    }

    // Different number of elements
    if( top_ == kEmptyStackDesignation ) // left empty, right has 1+ elements
      return std::strong_ordering::less;

    // Both stacks have 1 or more elements
    assert( top_ != kEmptyStackDesignation );
    assert( rhs.top_ != kEmptyStackDesignation );
    return top_ <=> rhs.top_;
  }

private:

  Array c_;
  size_t top_ = kEmptyStackDesignation; // TODO ptrdiff_t; explore different indexing options

}; // class ArrayStack

template <typename T, size_t N>
void constexpr swap( ArrayStack<T, N>& lhs, ArrayStack<T, N>& rhs ) noexcept( noexcept( lhs.swap( rhs ) ) )
{
  lhs.swap( rhs );
}

} // namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
