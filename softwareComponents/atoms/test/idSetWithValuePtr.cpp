#include <catch2/catch.hpp>

#include <atoms/containers.hpp>
#include <atoms/patterns.hpp>
#include <iostream>
#include <memory>

using atoms::HandleSet;
using atoms::ValuePtr;

struct Elem {
    int* num_ptr = nullptr;
    int id;
    Elem( int* p, int id ) : num_ptr( p ), id( id ) {}
    Elem( const Elem& m ) : num_ptr( m.num_ptr ), id( m.id ) {
        UNSCOPED_INFO("  Copy constructor " << num_ptr << ", " << id );
    }
    Elem* clone() const {
        UNSCOPED_INFO("Cloning " << num_ptr << ", " << id );
        return new Elem( *this );
    }
};

struct Span {
    Span( int x, int* ptr ) : id( x ), ptr( { ptr, x } ) {}
    int id = -1;
    ValuePtr< Elem > ptr;
};


TEST_CASE( "HandleSet + ValuePtr" ) {
    int x = 42;
    HandleSet< Span > HandleSet;

    CAPTURE( &x );
    INFO( "Inserting first element" );
    auto id = HandleSet.insert( { 0, &x } );
    INFO( "Inserted first element: " << id );
    Elem& m = *( HandleSet[ id ].ptr );
    CAPTURE( m.num_ptr );
    CHECK( m.num_ptr == &x );
    CHECK( *m.num_ptr == 42 );

    INFO( "Inserting second element" );
    id = HandleSet.insert( { 1, &x } );
    INFO( "Inserted second element: " << id );
    Elem& m2 = *( HandleSet[ id ].ptr );
    CHECK( m.num_ptr == &x );
    CHECK( *m.num_ptr == 42 );
    CHECK( m2.num_ptr == &x );
    CHECK( *m2.num_ptr == 42 );

    id = HandleSet.insert( { 2, &x } );
    CAPTURE( id );
    Elem& m3 = *( HandleSet[ id ].ptr );
    CHECK( m3.num_ptr == &x );
    CHECK( m2.num_ptr == &x );
    CHECK( m.num_ptr == &x );
};

