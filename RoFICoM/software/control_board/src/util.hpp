#pragma once

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