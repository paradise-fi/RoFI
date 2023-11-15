#pragma once

#include <stm32g0xx_hal.h>
#include <function2/function2.hpp>
#include <cstring>

// ARM can silently round-down unaligned acess to memory Therefore, naive
// implementation of viewAs using reinterpret cast does not work. We have to
// memcpy the types. This is done by the proxy.
template < typename T >
class AsProxy {
public:
    AsProxy( void *target ): _target( target ) {}
    operator T() const {
        T ret;
        memcpy( &ret, _target, sizeof( T ) );
        return ret;
    }

    AsProxy& operator=( const T& val ) {
        memcpy( _target, &val, sizeof ( T ) );
        return *this;
    }
private:
    void *_target;
};

template < typename T >
AsProxy< T > viewAs( void *ptr ) {
    return ptr;
}

class Every {
public:
    using Job =  fu2::unique_function< void() >;

    Every( uint32_t interval, Job j )
        : _interval( interval ),
          _next( HAL_GetTick() + _interval ),
          _do( std::move( j ) )
    {}

    void operator()() {
        if ( HAL_GetTick() < _next )
            return;
        _next += _interval;
        _do();
    }

    void reset() {
        _next = HAL_GetTick() + _interval;
    }

private:
    uint32_t _interval;
    uint32_t _next;
    Job _do;
};
