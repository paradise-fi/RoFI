#include <catch2/catch.hpp>
#include <geometry/aabb.hpp>
//#include "../include/aabb.hpp"

using namespace rofi::geometry;

TEST_CASE( "Insert objects without collision" ){
    AABB< Sphere > aabb;
    REQUIRE( aabb.size() == 0 );

    Vector center = { 0, 0, 0, 1 };
    for( size_t i = 0; i < 100; i++ ){
        REQUIRE( aabb.insert( Sphere( center ) ) );
        REQUIRE( aabb.size() == i + 1 );
        center[ 0 ] += 1.00;
    }
}

TEST_CASE( "Basic erase" ){
    AABB< Sphere > aabb;
    REQUIRE( aabb.size() == 0 );
    REQUIRE( aabb.insert( Sphere( { 0, 0, 0, 1 } ) ) );
    REQUIRE( aabb.size() == 1 );
    REQUIRE( aabb.erase( Sphere( { 0, 0, 0, 1 } ) ) );
    REQUIRE( aabb.size() == 0 );
    REQUIRE( !aabb.erase( Sphere( { 0, 0, 0, 1 } ) ) );
}

TEST_CASE( "Colliding objects" ){
    AABB< Sphere > aabb;
    REQUIRE( aabb.size() == 0 );
    REQUIRE( aabb.insert( Sphere( { 0, 0, 0, 1 } ) ) );
    REQUIRE( aabb.collides( Sphere( { 0, 0.1, 0, 1 } ) ) );
    REQUIRE( !aabb.insert( Sphere( { 0, 0.1, 0, 1 } ) ) );
    REQUIRE( !aabb.collides( Sphere( { 0, 1, 0, 1 } ) ) );
}

TEST_CASE( "Pseudorandom insert" ){
    AABB< Sphere > aabb;
    std::srand( 42 );
    while( aabb.size() < 1000 ){
        Sphere sphere( { double( std::rand() % 100 ),
                         double( std::rand() % 100 ),
                         double( std::rand() % 100 ),
                         1.0 } );
        aabb.insert( sphere );
    }
    REQUIRE( aabb.depth() == 86 );
}
