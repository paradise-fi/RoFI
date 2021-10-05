#pragma once


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