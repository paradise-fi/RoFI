#include <catch2/catch.hpp>

#include <rofibot.h>
#include <universalModule.h>

TEST_CASE( "Base Module Test" ) {
    // ToDo: Write the test
}

TEST_CASE( "Basic rofibot manipulation" ) {
    using namespace rofi;
    Rofibot bot;
    auto& m1 = bot.insert( buildUniversalModule( 0, 0, 0 ) );
    REQUIRE( m1.id == 0 );
    auto& m2 = bot.insert( buildUniversalModule( 0, 0, 0 ) );
    REQUIRE( m2.id == 1 );

    connect( m1.connector( 3 ), m2.connector( 2 ), Orientation::North );

    // ToDo: Write the test
}