#include <catch2/catch.hpp>

#include <atoms/containers.hpp>
#include <iostream>

using atoms::HandleSet;
using handle_int = atoms::HandleSet< int, int >::handle_type;


void testErasing( HandleSet< int, int >& hset, handle_int eraseHandle ) {
    INFO( "Erasing handle " << eraseHandle );
    hset.erase( eraseHandle );

    REQUIRE( hset.size() == 9 );
    for ( int i = 0; i != 10; i++ ) {
        if ( i == eraseHandle )
            continue;
        REQUIRE( hset[ i ] == i );
    }

    int counter = 0;
    for ( int i : hset ) {
        if ( counter == eraseHandle )
            counter = eraseHandle + 1;
        REQUIRE( i == counter );
        counter++;
    }

    // Insert
    REQUIRE( hset.insert( 42 ) == eraseHandle );
    for ( int i = 0; i != 10; i++ ) {
        if ( i == eraseHandle )
            REQUIRE( hset[ i ] == 42 );
        else
            REQUIRE( hset[ i ] == i );
    }
    counter = 0;
    for ( int i : hset ) {
        if ( counter == eraseHandle )
            CHECK( i == 42 );
        else
            CHECK( i == counter );
        counter++;
    }
}

TEST_CASE( "Basic usage of HandleSet" ) {
    HandleSet< int, int > hset;
    for ( int i = 0; i != 10; i++ )
        hset.insert( i );
    REQUIRE( hset.size() == 10 );

    SECTION( "HandleSet can be copied" ) {
        HandleSet< int, int > otherSet1 = hset;
        for ( int i = 0; i != 10; i++ ) {
            auto index = i;
            REQUIRE( hset[ index ] == otherSet1[ index ] );
        }

        const HandleSet< int, int > constHandleSet = hset;
        HandleSet< int, int > otherSet2 = constHandleSet;
        for ( int i = 0; i != 10; i++ ) {
            auto index = i;
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
        const HandleSet< int, int > constHandleSet = hset;
        for ( int h : constHandleSet ) {
            REQUIRE( h == counter );
            counter++;
        }
    }

    SECTION( "We can delete and reinsert items in the middle" ) {
        testErasing( hset, 5 );
    }

    SECTION( "We can delete and reinsert items at the end" ) {
        testErasing( hset, 9 );
    }

    SECTION( "We can delete and reinsert items at the beginning" ) {
        testErasing( hset, 0 );
    }
};
