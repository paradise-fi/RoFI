#pragma once

#include <utility>

template < typename T >
struct Peripheral {
    Peripheral( T *periph = nullptr ) : _periph( periph ) {}

    Peripheral( const Peripheral& ) = delete;
    Peripheral& operator=( const Peripheral& ) = delete;

    void swap( Peripheral& o ) {
        using std::swap;
        swap( _periph, o._periph );
    }
    Peripheral( Peripheral&& o ) { swap( o ); }
    Peripheral& operator=( Peripheral&& o ) { swap( o ); return *this; }

    T *periph() {
        return _periph;
    }


    T *_periph;
};
