#pragma once

#include <cassert>
#include <type_traits>
#include <atoms/traits.hpp>
#include <functional>
#include <limits>

namespace atoms::detail {

template < typename T >
using CountKeyT = decltype( std::declval< T >().count( std::declval< T::key_type > ) );

template < typename T >
using CountValT = decltype( std::declval< T >().count( std::declval< T::value_type > ) );

template < typename T >
using HasCount = std::disjunction<
    atoms::detect< T, CountKeyT >,
    atoms::detect< T, CountValT > >;

} // namespace atoms::detail


template < std::size_t... >
struct sum: std::integral_constant< std::size_t, 0 > {};

template <std::size_t X, std::size_t... Xs >
struct sum< X, Xs... > :
    std::integral_constant< std::size_t, X + sum< Xs... >::value > {};

template< class... Ts >
struct overload : Ts... { using Ts::operator()...; };
template < class... Ts >
overload( Ts... ) -> overload< Ts... >;

template < typename T >
T& as( void* p ) {
    return *reinterpret_cast< T * >( p );
}

template < typename T >
const T& as( const void* p ) {
    return *reinterpret_cast< const T * >( p );
}

template < typename T >
std::make_unsigned_t< T > to_unsigned( T value ) {
    static_assert( std::is_arithmetic_v< T > );
    if constexpr ( std::is_signed_v< T > ) {
        assert( value >= T( 0 ) );
    }
    return static_cast< std::make_unsigned_t< T > >( value );
}

template < typename T >
std::make_signed_t< T > to_signed( T value )
{
    static_assert( std::is_arithmetic_v< T > );
    if constexpr ( std::is_unsigned_v< T > ) {
        assert( value <= std::numeric_limits< std::make_signed_t< T > >::max() );
    }
    return static_cast< std::make_signed_t< T > >( value );
}

template < typename Container, typename T >
bool contains( const Container& c, const T& value )
{
    if constexpr( atoms::detail::HasCount< Container >::value ) {
        return c.count( value );
    }

    for ( const auto& x : c )
        if ( x == value )
            return true;
    return false;
}

/**
 * Given an iterable list of items, return index of it. If element is not found,
 * -1 is returned.
 */
template < typename T, typename Ts >
int indexOf( T t, const Ts& source ) {
    int i = 0;
    for ( const auto& x : source ) {
        if ( t == x )
            return i;
        i++;
    }
    return -1;
}

template < typename T >
T clamp( T value, T min, T max ) {
    return std::max( min, std::min( value, max ) );
}

/***
 * Defer execution of a job until end of scope;
 */
class Defer {
private:
    std::function< void() > _job;
public:
    template < typename F >
    Defer( F&& f ): _job( f ) {}
    ~Defer() { _job(); }
};


#define CONCAT_( a, b ) a ## b
#define CONCAT( a, b ) CONCAT_( a, b )
#define ATOMS_DEFER( fn ) Defer CONCAT( __defer__, __LINE__ )( fn )
