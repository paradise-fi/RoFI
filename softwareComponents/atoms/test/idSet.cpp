#include <catch2/catch.hpp>

#include <atoms/containers.hpp>
#include <iostream>

using atoms::IdSet;

void testErasing( IdSet< int >& idSet, int eraseIdx ) {
    INFO( "Erasing idx " << eraseIdx );
    idSet.erase( eraseIdx );

    REQUIRE( idSet.size() == 9 );
    for ( int i = 0; i != 10; i++ ) {
        if ( i == eraseIdx )
            continue;
        REQUIRE( idSet[ i ] == i );
    }

    int counter = 0;
    for ( int i : idSet ) {
        if ( counter == eraseIdx )
            counter = eraseIdx + 1;
        REQUIRE( i == counter );
        counter++;
    }

    // Insert
    REQUIRE( idSet.insert( 42 ) == eraseIdx );
    for ( int i = 0; i != 10; i++ ) {
        if ( i == eraseIdx )
            REQUIRE( idSet[ i ] == 42 );
        else
            REQUIRE( idSet[ i ] == i );
    }
    counter = 0;
    for ( int i : idSet ) {
        if ( counter == eraseIdx )
            CHECK( i == 42 );
        else
            CHECK( i == counter );
        counter++;
    }
}

TEST_CASE( "Basic usage of IdSet" ) {
    IdSet< int > idSet;
    for ( int i = 0; i != 10; i++ )
        idSet.insert( i );
    REQUIRE( idSet.size() == 10 );

    SECTION( "IdSet can be copied" ) {
        IdSet< int > otherSet1 = idSet;
        for ( int i = 0; i != 10; i++ ) {
            REQUIRE( idSet[ i ] == otherSet1[ i ] );
        }

        const IdSet< int > constIdSet = idSet;
        IdSet< int > otherSet2 = constIdSet;
        for ( int i = 0; i != 10; i++ ) {
            REQUIRE( idSet[ i ] == otherSet2[ i ] );
        }
    }

    SECTION( "Id set can be iterated" ) {
        int counter = 0;
        for ( int i : idSet ) {
            REQUIRE( i == counter );
            counter++;
        }

        counter = 0;
        const IdSet< int > constIdSet = idSet;
        for ( int i : constIdSet ) {
            REQUIRE( i == counter );
            counter++;
        }
    }

    SECTION( "We can delete and reinsert items in the middle" ) {
        testErasing( idSet, 5 );
    }

    SECTION( "We can delete and reinsert items at the end" ) {
        testErasing( idSet, 9 );
    }

    SECTION( "We can delete and reinsert items at the beginning" ) {
        testErasing( idSet, 0 );
    }
};
