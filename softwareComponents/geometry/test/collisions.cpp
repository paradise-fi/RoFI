#include <catch2/catch.hpp>
#include <geometry/shapes.hpp>

using namespace rofi::geometry;

TEST_CASE( "Line Sphere" ){
    Line z( { 0, 0, 0 }, { 0, 0, 1 } );
    REQUIRE( collide( z, Sphere( { 0, 0, 0 } ) ) );
    REQUIRE( collide( z, Sphere( { 0, 0, -1 } ) ) );
    REQUIRE( collide( z, Sphere( { 0, 0, 1 } ) ) );
    REQUIRE( collide( z, Sphere( { 0, 0, 100000 } ) ) );
    REQUIRE( collide( z, Sphere( { 0.1, 0.1, -0.5 } ) ) );
    REQUIRE( collide( z, Sphere( { 0.49, 0, 0 } ) ) );

    REQUIRE( !collide( z, Sphere( { 0, 0.51, 1 } ) ) );
    REQUIRE( !collide( z, Sphere( { 0.40, 0.40, 0 } ) ) );

    Line xy( { 10, 10, 0 }, { 1, 1, 0 } );
    REQUIRE( collide( xy, Sphere( { 0, 0, 0 } ) ) );
    REQUIRE( collide( xy, Sphere( { 20, 20, 0 } ) ) );
    REQUIRE( collide( xy, Sphere( { 5, 5, 0.2 } ) ) );
    REQUIRE( collide( xy, Sphere( { 0, 4.99, 0 }, 5 ) ) );

    REQUIRE( !collide( xy, Sphere( { 0, 0, 2 }, 0.99 ) ) );
}

TEST_CASE( "Segment Sphere" ){
    Segment z( { 0, 0, 0 }, { 0, 0, 10 } );
    REQUIRE( collide( z, Sphere( { 0, 0, 0 } ) ) );
    REQUIRE( collide( z, Sphere( { 0, 0, 10 } ) ) );
    REQUIRE( collide( z, Sphere( { 0, 0.4, 5 } ) ) );

    REQUIRE( !collide( z, Sphere( { 0, 0, -0.5 } ) ) );
    REQUIRE( !collide( z, Sphere( { 0, 0, 11 } ) ) );
    REQUIRE( !collide( z, Sphere( { 0, 0.5, 5 } ) ) );
}

TEST_CASE( "Line Box" ){
    Line z( { 0, 0, 0 }, { 0, 0, 1 } );
    REQUIRE( collide( z, Box( Vector{ 0, 0, 0 } ) ) );
    REQUIRE( collide( z, Box( Vector{ 0, 0, 10 } ) ) );
    REQUIRE( collide( z, Box( Vector{ 0, 0, -10 } ) ) );

    Line xy( { 10, 10, 0 }, { 1, 1, 0 } );
    REQUIRE( collide( xy, Box( Vector{ 0, 0, 5 }, 1, 1, 10.1 ) ) );
    REQUIRE( collide( xy, Box( Vector{ 5, 5, 0 }, 10, 10, 10 ) ) );
    REQUIRE( collide( xy, Box( Vector{ 100, 100, 0 }, 0.5, 0.5, 0.5 ) ) );

    REQUIRE( !collide( z, Box( Vector{ 1, 0, 0 } ) ) );

}

TEST_CASE( "Segment Box" ){
    Segment z( { 0, 0, 0 }, { 0, 0, 10 } );
    REQUIRE( collide( z, Box( Vector{ 0, 0, 0 } ) ) );
    REQUIRE( collide( z, Box( Vector{ 0, 0, 10 } ) ) );
    REQUIRE( collide( z, Box( Vector{ 0, 0, 0.1 } ) ) );
    REQUIRE( collide( z, Box( Vector{ 0, 0, -0.1 } ) ) );

    REQUIRE( !collide( z, Box( Vector{ 0, 0, -0.51 } ) ) );
    REQUIRE( !collide( z, Box( Vector{ 0, 0, -10 } ) ) );

    Segment xy( { 10, 10, 0 }, { 20, 20, 0 } );
    REQUIRE( collide( xy, Box( Vector{ 0, 15, 0 }, 30.1, 0.1, 0.1 ) ) );
    REQUIRE( collide( xy, Box( Vector{ -15, -15, 0 }, 52, 52, 1 ) ) );

    REQUIRE( !collide( xy, Box( Vector{ 0, 0, 0 }, 19, 19, 19 ) ) );
    REQUIRE( !collide( xy, Box( Vector{ 21, 21, 0 } ) ) );

    Segment small( { 0, 0, 0 }, { 1, 1, 1 } );
    REQUIRE( collide( small, Box( Vector{ 0, 0, 0 }, 20, 20, 20 ) ) );
}

TEST_CASE( "Sphere Plane" ){
    Plane z( { 0, 0, 0 }, { 0, 0, 1 } );
    REQUIRE( collide( z, Sphere( { 0, 0, 0 } ) ) );
    REQUIRE( collide( z, Sphere( { 1, 0, 0 } ) ) );
    REQUIRE( collide( z, Sphere( { 0, 1, 0 } ) ) );
    REQUIRE( collide( z, Sphere( { 0, 0, 1 }, 2 )));
    REQUIRE( collide( z, Sphere( { 0, 100000, 0 } )));
    
    REQUIRE( !collide( z, Sphere( { 0, 0, 1 } ) ) );
    REQUIRE( !collide( z, Sphere( { 0, 0, 0.5 } ) ) );
    REQUIRE( !collide( z, Sphere( { 100, 100, 0.5 })));

    Sphere s( { 0.5, 0.5, 0.5 } );
    REQUIRE( !collide( Plane( { 0, 0, 0 }, { 0, 1, 0 } ), s ) );
    REQUIRE( !collide( Plane( { 0, 0, 0 }, { 1, 0, 0 } ), s ) );
}

TEST_CASE( "Box Plane" ){
    Plane z( { 0, 0, 0 }, { 0, 0, 1 } );
    REQUIRE( collide( z, Box( Vector{ 0, 0, 0 } ) ) );
    REQUIRE( !collide( z, Box( Vector{ 0, 0, 0.51 } )));
}