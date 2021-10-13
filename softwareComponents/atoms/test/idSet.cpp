#include <catch2/catch.hpp>

#include <atoms/containers.hpp>
#include <iostream>

using atoms::HandleSet;

void testErasing( HandleSet< int >& hset, int eraseIdx ) {
    INFO( "Erasing idx " << eraseIdx );
    hset.erase( eraseIdx );

    REQUIRE( hset.size() == 9 );
    for ( int i = 0; i != 10; i++ ) {
        if ( i == eraseIdx )
            continue;
        REQUIRE( hset[ i ] == i );
    }

    int counter = 0;
    for ( int i : hset ) {
        if ( counter == eraseIdx )
            counter = eraseIdx + 1;
        REQUIRE( i == counter );
        counter++;
    }

    // Insert
    REQUIRE( hset.insert( 42 ) == eraseIdx );
    for ( int i = 0; i != 10; i++ ) {
        if ( i == eraseIdx )
            REQUIRE( hset[ i ] == 42 );
        else
            REQUIRE( hset[ i ] == i );
    }
    counter = 0;
    for ( int i : hset ) {
        if ( counter == eraseIdx )
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
            REQUIRE( hset[ i ] == otherSet1[ i ] );
        }

        const HandleSet< int > constHandleSet = hset;
        HandleSet< int > otherSet2 = constHandleSet;
        for ( int i = 0; i != 10; i++ ) {
            REQUIRE( hset[ i ] == otherSet2[ i ] );
        }
    }

    SECTION( "Id set can be iterated" ) {
        int counter = 0;
        for ( int i : hset ) {
            REQUIRE( i == counter );
            counter++;
        }

        counter = 0;
        const HandleSet< int > constHandleSet = hset;
        for ( int i : constHandleSet ) {
            REQUIRE( i == counter );
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
