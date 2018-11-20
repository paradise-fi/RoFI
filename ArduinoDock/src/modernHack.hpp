#pragma once

template< class T > struct remove_reference        { typedef T type; };
template< class T > struct remove_reference< T& >  { typedef T type; };
template< class T > struct remove_reference< T&& > { typedef T type; };

template < typename T >
typename remove_reference< T >::type&& move( T&& arg )
{
    return static_cast< typename remove_reference< T >::type&& >( arg );
}