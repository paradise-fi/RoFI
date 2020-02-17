#pragma once

#include <cstdint>
#include <iostream>
#include <string>

template < typename T >
T& as( void* p ) {
    return *reinterpret_cast< T * >( p );
}

template < typename T >
const T& as( const void* p ) {
    return *reinterpret_cast< const T * >( p );
}
