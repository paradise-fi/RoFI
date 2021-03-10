#pragma once

namespace atoms {

/**
 * \brief Convert type to bool, default boolead predicate
 */
template < typename T >
bool booleanize( const T& t ) {
    return t;
}

/**
 * \brief Check if there is an element fulfilling given predicate in the
 * container.
 *
 * If no predicate is specified, the default predicate tries to convert element
 * to bool
 */
template < typename Container,
           typename Pred = decltype( booleanize< typename Container::value_type > ) >
bool any( const Container& container,
          Pred pred = booleanize )
{
    for ( const auto& x : container ) {
        if ( pred( x ) )
            return true;
    }
    return false;
}

/**
 * \brief Check all elements of the container fulfill given predicate
 *
 * If no predicate is specified, the default predicate tries to convert element
 * to bool
 */
template < typename Container,
           typename Pred = decltype( booleanize< typename Container::value_type > ) >
bool all( const Container& container,
          Pred pred = booleanize )
{
    for ( const auto& x : container ) {
        if ( !pred( x ) )
            return false;
    }
    return true;
}


} // namespace atoms