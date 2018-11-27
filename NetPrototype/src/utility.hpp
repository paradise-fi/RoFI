#pragma once

#include <cstdint>

template < typename T >
T& as( uint8_t* p ) {
    return *reinterpret_cast< T * >( p );
}

template < typename T >
const T& as( const uint8_t* p ) {
    return *reinterpret_cast< const T * >( p );
}