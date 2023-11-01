#include <catch2/catch.hpp>
#include <geometry/aabb.hpp>

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

TEST_CASE( "Erase empties the tree" ){
    AABB< Sphere > aabb;
    std::vector< Sphere > generated;
    for( int i = 0; i < 10; i++ ){
        Sphere sphere( { double( std::rand() % 100 ),
                         double( std::rand() % 100 ),
                         double( std::rand() % 100 ),
                         1.0 } );
        generated.push_back( sphere );
    }
    for( const auto& s : generated ){
        aabb.insert( s );
    }
    for( const auto& s : generated ){
        aabb.erase( s );
    }
    REQUIRE( aabb.size() == 0 );
    REQUIRE( aabb.depth() == 0 );
}

TEST_CASE( "Simple iterator" ){
    AABB< Sphere > aabb;
    aabb.insert( Sphere( Vector{ 10, 10, 10, 1 } ) );
    for( const auto& s : aabb ){
        REQUIRE( s == Sphere( Vector{ 10, 10, 10, 1 } ) );
    }
}

TEST_CASE( "Colliding leaves" ){
    AABB< Sphere > aabb;
    auto s1 = Sphere( Vector{ 0, 0, 0, 1 } );
    auto s2 = Sphere( Vector{ 0, 0, 1, 1 } );
    aabb.insert( s1 );
    aabb.insert( s2 );
    auto colliding_all = aabb.colliding_leaves( Sphere( Vector{ 0, 0, 0.5, 1 } ) );
    REQUIRE( colliding_all.size() == 2 );
    bool equal = colliding_all[ 0 ] == s1 && colliding_all[ 1 ] == s2 ||
                 colliding_all[ 0 ] == s2 && colliding_all[ 1 ] == s1;
    REQUIRE( equal );

    auto colliding_one = aabb.colliding_leaves( Sphere( Vector{ 0.5, 0, 0, 1 } ) );
    REQUIRE( colliding_one.size() == 1 );
    REQUIRE( colliding_one[ 0 ] == s1 );
}

TEST_CASE( "Raycasting" ){
    AABB< Sphere > aabb;
    auto s1 = Sphere( Vector{ 0, 0, 0, 1 } );
    auto s2 = Sphere( Vector{ 0, 0, 1, 1 } );
    aabb.insert( s1 );
    aabb.insert( s2 );
    REQUIRE( aabb.colliding_leaves( Segment( { 0, 0, 0 }, { 0, 0, 1 } ) ).size() == 2 );
    REQUIRE( aabb.colliding_leaves( Segment( { 0, 0, 0 }, { 0, 0, -1 } ) ).size() == 1 );
    REQUIRE( aabb.colliding_leaves( Segment( { 0, 0, 2 }, { 0, 0, 1 } ) ).size() == 1 );
    REQUIRE( aabb.colliding_leaves( Segment( { 0, -1, 0 }, { 0, 0, -1 } ) ).size() == 0 );
}
