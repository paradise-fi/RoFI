#include <catch2/catch.hpp>
#include <aabb.hpp>
//#include "../include/aabb.hpp"

using namespace rofi::geometry;

TEST_CASE( "Insert objects without collision" ){
    AABB< Sphere > aabb;
    REQUIRE( aabb.objects() == 0 );

    Vector center = { 0, 0, 0, 1 };
    for( size_t i = 0; i < 100; i++ ){
        REQUIRE( aabb.insert( Sphere( center ) ) );
        REQUIRE( aabb.objects() == i + 1 );
        center[ 0 ] += 1.00;
    }
}

TEST_CASE( "Basic erase" ){
    AABB< Sphere > aabb;
    REQUIRE( aabb.objects() == 0 );
    REQUIRE( aabb.insert( Sphere( { 0, 0, 0, 1 } ) ) );
    REQUIRE( aabb.objects() == 1 );
    REQUIRE( aabb.erase( Sphere( { 0, 0, 0, 1 } ) ) );
    REQUIRE( aabb.objects() == 0 );
    REQUIRE( !aabb.erase( Sphere( { 0, 0, 0, 1 } ) ) );
}
