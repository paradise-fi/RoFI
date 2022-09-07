#pragma once

#include <type_traits>
#include <functional>

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
