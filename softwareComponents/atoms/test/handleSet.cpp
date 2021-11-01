#include <catch2/catch.hpp>

#include <atoms/containers.hpp>
#include <iostream>

using atoms::HandleSet;
using handle = atoms::HandleSet< int >::handle_type;

// the static casts are ok because we know the underlying
// implementation but don't use this in you code...

void testErasing( HandleSet< int >& hset, handle eraseHandle ) {
    INFO( "Erasing handle " << static_cast< int >( eraseHandle ) );
    hset.erase( eraseHandle );

    REQUIRE( hset.size() == 9 );
    for ( int i = 0; i != 10; i++ ) {
        if ( i == static_cast< int >( eraseHandle ) )
            continue;
        // this is not very nice
        REQUIRE( hset[ static_cast< handle >( i ) ] == i );
    }

    int counter = 0;
    for ( int i : hset ) {
        if ( counter == static_cast< int >( eraseHandle ) )
            counter = static_cast< int >( eraseHandle ) + 1;
        REQUIRE( i == counter );
        counter++;
    }

    // Insert
    REQUIRE( hset.insert( 42 ) == eraseHandle );
    for ( int i = 0; i != 10; i++ ) {
        if ( i == static_cast< int >( eraseHandle ) )
            REQUIRE( hset[ static_cast< handle >( i ) ] == 42 );
        else
            REQUIRE( hset[ static_cast< handle >( i ) ] == i );
    }
    counter = 0;
    for ( int i : hset ) {
        if ( counter == static_cast< int >( eraseHandle ) )
            CHECK( i == 42 );
        else
            CHECK( i == counter );
        counter++;
    }
}

TEST_CASE( "Basic usage of HandleSet" ) {
    HandleSet< int > hset;
    for ( int i = 0; i != 10; i++ )
        hset.insert( i );
    REQUIRE( hset.size() == 10 );

    SECTION( "HandleSet can be copied" ) {
        HandleSet< int > otherSet1 = hset;
        for ( int i = 0; i != 10; i++ ) {
            handle index = static_cast< handle >( i );
            REQUIRE( hset[ index ] == otherSet1[ index ] );
        }

        const HandleSet< int > constHandleSet = hset;
        HandleSet< int > otherSet2 = constHandleSet;
        for ( int i = 0; i != 10; i++ ) {
            handle index = static_cast< handle >( i );
            REQUIRE( hset[ index ] == otherSet2[ index ] );
        }
    }

    SECTION( "Handle set can be iterated" ) {
        int counter = 0;
        for ( int h : hset ) {
            REQUIRE( h == counter );
            counter++;
        }

        counter = 0;
        const HandleSet< int > constHandleSet = hset;
        for ( int h : constHandleSet ) {
            REQUIRE( h == counter );
            counter++;
        }
    }

    SECTION( "We can delete and reinsert items in the middle" ) {
        testErasing( hset, static_cast< handle >( 5 ) );
    }

    SECTION( "We can delete and reinsert items at the end" ) {
        testErasing( hset, static_cast< handle >( 9 ) );
    }

    SECTION( "We can delete and reinsert items at the beginning" ) {
        testErasing( hset, static_cast< handle >( 0 ) );
    }
};
