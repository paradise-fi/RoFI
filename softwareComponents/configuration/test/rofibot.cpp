#include <catch2/catch.hpp>

#include <rofibot.h>
#include <universalModule.h>

namespace {

void printMatrix( Matrix m ) {
    std::cout << "-------------------\n";
    std::cout << m << "\n";
    std::cout << "-------------------\n";
    /*
    for ( int i = 0; i < m.n_rows; i++ ) {
        for ( int j = 0; j < m.n_cols; j++ ) {
            std::cout << round( m(i, j) ) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
    */
}

TEST_CASE( "Base Module Test" ) {}

TEST_CASE( "Universal Module Test" ) {
    SECTION( "Creation" ) {
        rofi::Module um = rofi::buildUniversalModule( 0, 0, 0 );
        CHECK( um.connectors().size() - 1 == 6 );
        um.setJointParams( 0, { 0, 0, 0 } );
    }

    SECTION( "Position - default" ) {
        rofi::Module um = rofi::buildUniversalModule( 0, 0, 0 );
        CHECK( equals( um.getComponentPosition( 6 ), identity ) );
        CHECK( equals( um.getComponentPosition( 9 ), { { -1, 0,  0, 0 }
                                                     , {  0, 1,  0, 0 }
                                                     , {  0, 0, -1, 1 }
                                                     , {  0, 0,  0, 1 } } ) );
        printMatrix( um.getComponentPosition( 9 ) );
    }

    SECTION( "Position - rotated" ) {
        rofi::Module um = rofi::buildUniversalModule( 0, -M_PI_2, M_PI_2 );
        CHECK( equals( um.getComponentPosition( 6 ), identity ) ); // shoeA
        CHECK( equals( um.getComponentPosition( 9 ), { { 0 , 0 , -1, 0 } // shoeB
                                                     , { -1, 0 , 0 , 0 }
                                                     , { 0 , 1 , 0 , 1 }
                                                     , { 0 , 0 , 0 , 1 } } ) );
        printMatrix( um.getComponentPosition( 9 ) );
    }
}

TEST_CASE( "Two modules" ) {
    rofi::Rofibot bot;
    auto& m1 = bot.insert( rofi::buildUniversalModule( 0, 0, 0 ) );
    auto& m2 = bot.insert( rofi::buildUniversalModule( 0, 0, 0 ) );
    connect( m1.connector( 4 ), m2.connector( 1 ), rofi::Orientation::North );
    // ToDo: fix below
    // rofi::connect< rofi::RigidJoint >( m1.connector( 1 ), { 0, 0, 0 }, identity );
    CHECK_NOTHROW( bot.prepare() );

    SECTION( "ShoesA " ) {
        CHECK( equals( bot.getModuleOrientation( m1.id ), identity ) );
        CHECK( equals( bot.getModuleOrientation( m2.id ), identity * translate( { 0, 0, 1 } ) ) );
    }

    SECTION( "ShoesB" ) {
    Matrix m2shoeB = bot.getModuleOrientation( m2.id ) * m2.getComponentPosition( 9 );
    Matrix m1shoeB = bot.getModuleOrientation( m1.id ) * m1.getComponentPosition( 9 );
    printMatrix( bot.getModuleOrientation( m2.id ) );
    CHECK( equals( m1shoeB, { { 0 , 0 , -1, 0 }
                            , { -1, 0 , 0 , 0 }
                            , { 0 , 1 , 0 , 1 }
                            , { 0 , 0 , 0 , 1 } } ) );
    CHECK( equals( m2shoeB, m1shoeB * translate( { 0, 0, 1 } ) ) );
    }
}

TEST_CASE( "Basic rofibot manipulation" ) {
    using namespace rofi;
    Rofibot bot;
    auto& m1 = bot.insert( buildUniversalModule( 0, 0, 0 ) );
    CHECK( m1.id == 0 );
    CHECK( bot.getModule( 0 )->id == 0 );
    REQUIRE( m1.id == 0 );
    REQUIRE( m1.parent == &bot );
    CHECK( &m1 == bot.getModule( 0 ) );
    auto& m2 = bot.insert( buildUniversalModule( 0, 0, 0 ) );
    REQUIRE( m2.id == 1 );
    CHECK( m1.id == 0 );
    CHECK( bot.getModule( 0 )->id == 0 );
    REQUIRE( &m1 == bot.getModule( 0 ) );
    auto& m3 = bot.insert( buildUniversalModule( 0, 0, 0 ) );
    REQUIRE( m3.id == 2 );
    REQUIRE( m1.parent == &bot );
    auto& m4 = bot.insert( buildUniversalModule( 0, 0, 0 ) );
    REQUIRE( m4.id == 3 );
    auto& m5 = bot.insert( buildUniversalModule( 0, 0, 0 ) );
    REQUIRE( m5.id == 4 );

    connect( m1.connector( 3 ), m3.connector( 2 ), rofi::Orientation::North );
    m1.setJointParams( 0, { 0, 0, 0 } );
    auto [ b, str ] = bot.isValid( SimpleColision() );
    if ( !b )
        std::cout << "Error: " << str << "\n";
}

} // namespace
