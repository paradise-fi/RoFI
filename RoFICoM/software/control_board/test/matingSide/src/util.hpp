#pragma once

#include <type_traits>
#include <cstring>

template< typename From, typename To >
using CopyConst = typename std::conditional< std::is_const< From >::value, const To, To >::type;


// ARM can silently round-down unaligned acess to memory Therefore, naive
// implementation of viewAs using reinterpret cast does not work. We have to
// memcpy the types. This is done by the proxy.
template < typename T >
class AsProxy {
public:
    using Ptr = CopyConst< T, void >;
    using Val = typename std::remove_const< T >::type;


    AsProxy( Ptr *target ): _target( target ) {}
    operator T() const {
        Val ret;
        memcpy( &ret, _target, sizeof( T ) );
        return ret;
    }

    AsProxy& operator=( const T& val ) {
        memcpy( _target, &val, sizeof ( T ) );
        return *this;
    }
private:
    Ptr *_target;
};

template < typename T >
AsProxy< T > viewAs( void *ptr ) {
    return ptr;
}

template < typename T >
AsProxy< const T > viewAs( const void *ptr ) {
    return ptr;
}