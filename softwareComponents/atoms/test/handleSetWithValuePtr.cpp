#include <catch2/catch.hpp>

#include <atoms/containers.hpp>
#include <atoms/patterns.hpp>
#include <iostream>
#include <memory>

using atoms::HandleSet;
using atoms::ValuePtr;

struct Elem {
    int* num_ptr = nullptr;
    int handle;
    Elem( int* p, int handle ) : num_ptr( p ), handle( handle ) {}
    Elem( const Elem& m ) : num_ptr( m.num_ptr ), handle( m.handle ) {
        UNSCOPED_INFO("  Copy constructor " << num_ptr << ", " << static_cast< int >( handle ) );
    }
    Elem* clone() const {
        UNSCOPED_INFO("Cloning " << num_ptr << ", " << handle );
        return new Elem( *this );
    }
};

struct Span {
    Span( int x, int* ptr ) : handle( x ), ptr( { ptr, x } ) {}
    int handle = -1;
    ValuePtr< Elem > ptr;
};


TEST_CASE( "HandleSet + ValuePtr" ) {
    int x = 42;
    HandleSet< Span > HandleSet;

    CAPTURE( &x );
    INFO( "Inserting first element" );
    auto handle = HandleSet.insert( { 0, &x } );
    INFO( "Inserted first element: " << static_cast< int >( handle ) );
    Elem& m = *( HandleSet[ handle ].ptr );
    CAPTURE( m.num_ptr );
    CHECK( m.num_ptr == &x );
    CHECK( *m.num_ptr == 42 );

    INFO( "Inserting second element" );
    handle = HandleSet.insert( { 1, &x } );
    INFO( "Inserted second element: " << static_cast< int >( handle ) );
    Elem& m2 = *( HandleSet[ handle ].ptr );
    CHECK( m.num_ptr == &x );
    CHECK( *m.num_ptr == 42 );
    CHECK( m2.num_ptr == &x );
    CHECK( *m2.num_ptr == 42 );

    handle = HandleSet.insert( { 2, &x } );
    CAPTURE( handle );
    Elem& m3 = *( HandleSet[ handle ].ptr );
    CHECK( m3.num_ptr == &x );
    CHECK( m2.num_ptr == &x );
    CHECK( m.num_ptr == &x );
}

