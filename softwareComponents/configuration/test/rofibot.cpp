#include <catch2/catch.hpp>

#include <rofibot.h>
#include <universalModule.h>

namespace {

TEST_CASE( "Base Module Test" ) {}

TEST_CASE( "Universal Module Test" ) {
    SECTION( "Creation" ) {
        rofi::Module um = rofi::buildUniversalModule( 0, 0, 0 );
        CHECK( um.connectors().size() - 1 == 6 );
        um.setJointParams( 0, { 0, 0, 0 } );
        CHECK( um.components().size() -1 == 9 );
    }

    SECTION( "Position - default" ) {
        rofi::Module um = rofi::buildUniversalModule( 0, 0, 0 );
        // A part
        CHECK( equals( um.getComponentPosition( 0 ), identity ) );
        CHECK( equals( um.getComponentPosition( 1 ), rotate( M_PI, { 0, 0, 1 } ) * rotate( M_PI, { 1, 0, 0 } ) ) );
        CHECK( equals( um.getComponentPosition( 2 ), rotate( - M_PI_2, { 0, 1, 0 } ) * rotate( M_PI, { 1, 0, 0 } ) ) );
        CHECK( equals( um.getComponentPosition( 6 ), identity ) );
        CHECK( equals( um.getComponentPosition( 7 ), identity ) );
        // B part
        // -X
        CHECK( equals( um.getComponentPosition( 3 ), translate( { 0, 0, 1 } ) * rotate( M_PI, { 0, 0, 1 } )
                                                   * rotate( M_PI, { 1, 0, 0 } ) ) );
        // +X
        CHECK( equals( um.getComponentPosition( 4 ), translate( { 0, 0, 1 } ) * rotate( M_PI, { 0, 1, 0 } )
                                                   * rotate( M_PI, { 0, 0, 1 } ) * rotate( M_PI, { 1, 0, 0 } ) ) );
        // -Z
        CHECK( equals( um.getComponentPosition( 5 ), translate( { 0, 0, 1 } ) * rotate( M_PI, { 0, 1, 0 } )
                                                   * rotate( - M_PI_2, { 0, 1, 0 } ) * rotate( M_PI, { 1, 0, 0 } ) ) );
        // Body
        CHECK( equals( um.getComponentPosition( 8 ), translate( { 0, 0, 1 } ) * rotate( M_PI, { 0, 1, 0 } ) ) );
        // Shoe
        CHECK( equals( um.getComponentPosition( 9 ), { { -1, 0,  0, 0 }
                                                     , {  0, 1,  0, 0 }
                                                     , {  0, 0, -1, 1 }
                                                     , {  0, 0,  0, 1 } } ) );
    }

    SECTION( "Position - rotated gamma" ) {
        rofi::Module um = rofi::buildUniversalModule( 0, 0, 90 );
        // A part
        CHECK( equals( um.getComponentPosition( 0 ), identity ) );
        CHECK( equals( um.getComponentPosition( 1 ), rotate( M_PI, { 0, 0, 1 } ) * rotate( M_PI, { 1, 0, 0 } ) ) );
        CHECK( equals( um.getComponentPosition( 2 ), rotate( - M_PI_2, { 0, 1, 0 } ) * rotate( M_PI, { 1, 0, 0 } ) ) );
        CHECK( equals( um.getComponentPosition( 6 ), identity ) );
        CHECK( equals( um.getComponentPosition( 7 ), identity ) );
        // B part
        // -X
        CHECK( equals( um.getComponentPosition( 3 ), rotate( M_PI_2, { 0, 0, 1 } ) * translate( { 0, 0, 1 } )
                                                    * rotate( M_PI, { 0, 0, 1 } ) * rotate( M_PI, { 1, 0, 0 } ) ) );
        // +X
        CHECK( equals( um.getComponentPosition( 4 ), rotate( M_PI_2, { 0, 0, 1 } ) * translate( { 0, 0, 1 } )
                                                   * rotate( M_PI, { 0, 1, 0 } ) * rotate( M_PI, { 0, 0, 1 } )
                                                   * rotate( M_PI, { 1, 0, 0 } ) ) );
        // -Z
        CHECK( equals( um.getComponentPosition( 5 ), rotate( M_PI_2, { 0, 0, 1 } ) * translate( { 0, 0, 1 } )
                                                    * rotate( M_PI, { 0, 1, 0 } ) * rotate( - M_PI_2, { 0, 1, 0 } )
                                                    * rotate( M_PI, { 1, 0, 0 } ) ) );
        // Body
        CHECK( equals( um.getComponentPosition( 8 ), rotate( M_PI_2, { 0, 0, 1 } ) * translate( { 0, 0, 1 } )
                                                   * rotate( M_PI, { 0, 1, 0 } ) ) );
        // Shoe
        CHECK( equals( um.getComponentPosition( 9 ), { {  0, -1,  0, 0 } // shoeB
                                                     , { -1,  0,  0, 0 }
                                                     , {  0,  0, -1, 1 }
                                                     , {  0,  0,  0, 1 } } ) );
    }

    SECTION( "Position - rotated beta + gamma" ) {
        rofi::Module um = rofi::buildUniversalModule( 0, -90, 90 );
        // A part
        CHECK( equals( um.getComponentPosition( 0 ), identity ) );
        CHECK( equals( um.getComponentPosition( 1 ), rotate( M_PI, { 0, 0, 1 } ) * rotate( M_PI, { 1, 0, 0 } ) ) );
        CHECK( equals( um.getComponentPosition( 2 ), rotate( - M_PI_2, { 0, 1, 0 } ) * rotate( M_PI, { 1, 0, 0 } ) ) );
        CHECK( equals( um.getComponentPosition( 6 ), identity ) );
        CHECK( equals( um.getComponentPosition( 7 ), identity ) );
        // B part
        // -X
        CHECK( equals( um.getComponentPosition( 3 ), rotate( M_PI_2, { 0, 0, 1 } )
                                                    * translate( { 0, 0, 1 } )
                                                    * rotate( M_PI, { 0, 1, 0 } )
                                                    * rotate( - M_PI_2, { 1, 0, 0 } ) ) );
        // +X
        CHECK( equals( um.getComponentPosition( 4 ), rotate( M_PI_2, { 0, 0, 1 } ) * translate( { 0, 0, 1 } )
                                                    * rotate( M_PI, { 0, 1, 0 } )  * rotate( - M_PI_2, { 1, 0, 0 } )
                                                    * rotate( M_PI, { 0, 0, 1 } ) * rotate( M_PI, { 1, 0, 0 } ) ) );
        // -Z
        CHECK( equals( um.getComponentPosition( 5 ), rotate( M_PI_2, { 0, 0, 1 } ) * translate( { 0, 0, 1 } )
                                                    * rotate( M_PI, { 0, 1, 0 } )  * rotate( - M_PI_2, { 1, 0, 0 } )
                                                    * rotate( - M_PI_2, { 0, 1, 0 } ) * rotate( M_PI, { 1, 0, 0 } ) ) );
        // Body
        CHECK( equals( um.getComponentPosition( 8 ), rotate( M_PI_2, { 0, 0, 1 } ) * translate( { 0, 0, 1 } )
                                                    * rotate( M_PI, { 0, 1, 0 } ) ) );
        // Shoe
        CHECK( equals( um.getComponentPosition( 9 ), { {  0, 0, -1, 0 }
                                                     , { -1, 0,  0, 0 }
                                                     , {  0, 1,  0, 1 }
                                                     , {  0, 0,  0, 1 } } ) );
    }
}

TEST_CASE( "Two modules next to each other" ) {
    rofi::Rofibot bot;
    auto& m1 = bot.insert( rofi::buildUniversalModule( 0, 0, 0 ) );
    auto& m2 = bot.insert( rofi::buildUniversalModule( 0, 0, 0 ) );
    connect( m1.connector( 0 ), m2.connector( 2 ), rofi::Orientation::South );
    rofi::connect< rofi::RigidJoint >( m1.body( 0 ), { 0, 0, 0 }, identity );
    REQUIRE_NOTHROW( bot.prepare() );

    SECTION( "The second is just moved to left by one" ) {
        Matrix new_origin = identity * translate( { -1, 0, 0 } );
        REQUIRE( !equals( identity, new_origin ) );
        Matrix mat = identity;
        CHECK( equals( center( m2.getComponentPosition( 0, bot.getModulePosition( m2.id ) ) )
                     , center( new_origin ) ) );

        mat = new_origin * rotate( M_PI, { 0, 0, 1 } );
        CHECK( equals( center( m2.getComponentPosition( 1, bot.getModulePosition( m2.id ) ) ), center( mat ) ) );

        mat = new_origin * rotate( - M_PI_2, { 0, 1, 0 } );
        CHECK( equals( center( m2.getComponentPosition( 2, bot.getModulePosition( m2.id ) ) ), center( mat ) ) );
        CHECK( equals( center( m2.getComponentPosition( 6, bot.getModulePosition( m2.id ) ) ), center( new_origin ) ) );
        CHECK( equals( center( m2.getComponentPosition( 7, bot.getModulePosition( m2.id ) ) ), center( new_origin ) ) );

        mat =  new_origin * m1.getComponentPosition( 3 );
        CHECK( equals( center( m2.getComponentPosition( 3, bot.getModulePosition( m2.id ) ) ), center( mat ) ) );

        mat = new_origin * m1.getComponentPosition( 4 );
        CHECK( equals( center( m2.getComponentPosition( 4, bot.getModulePosition( m2.id ) ) ), center( mat ) ) );

        mat = new_origin * m1.getComponentPosition( 5 );
        CHECK( equals( center( m2.getComponentPosition( 5, bot.getModulePosition( m2.id ) ) ), center( mat ) ) );

        mat = new_origin * m1.getComponentPosition( 8 );
        CHECK( equals( center( m2.getComponentPosition( 8, bot.getModulePosition( m2.id ) ) ), center( mat ) ) );

        mat = { { -1, 0, 0, -1 }, { 0, 1, 0, 0 }, { 0, 0, -1, 1 }, { 0, 0, 0, 1 } };
        CHECK( equals( center( m2.getComponentPosition( 9, bot.getModulePosition( m2.id ) ) ), center( mat ) ) );

        mat = new_origin * m1.getComponentPosition( 9 );
        CHECK( equals( center( m2.getComponentPosition( 9, bot.getModulePosition( m2.id ) ) ), center( mat ) ) );
    }
}

TEST_CASE( "Two modules - different angles" ) {
    rofi::Rofibot bot;
    auto& m1 = bot.insert( rofi::buildUniversalModule( 0, 0, 0 ) );
    auto& m2 = bot.insert( rofi::buildUniversalModule( 0, 0, 0 ) );
    connect( m1.connector( 3 ), m2.connector( 0 ), rofi::Orientation::South );
    rofi::connect< rofi::RigidJoint >( m1.body( 0 ), { 0, 0, 0 }, identity );
    REQUIRE_NOTHROW( bot.prepare() );

    SECTION( "BodyA " ) {
        CHECK( equals( bot.getModulePosition( m1.id ), identity ) );
        CHECK( equals( center( bot.getModulePosition( m2.id ) ), center( identity * translate( { 1, 0, 1 } ) ) ) );
    }

    SECTION( "BodyB" ) {
    Matrix m1shoeB = m1.getComponentPosition( 9 );
    Matrix m2shoeB = m2.getComponentPosition( 9, bot.getModulePosition( m2.id ) );
    CHECK( equals( m1shoeB, { { -1, 0,  0, 0 }
                            , {  0, 1,  0, 0 }
                            , {  0, 0, -1, 1 }
                            , {  0, 0,  0, 1 } } ) );
    CHECK( equals( center( m2shoeB ), center( translate( { 1, 0, 2 } ) ) ) );
    }
}

TEST_CASE( "Three modules -- connect docks 3 to 0s " ) {
    rofi::Rofibot bot;
    auto& m1 = bot.insert( rofi::buildUniversalModule( 0, 0, 0 ) );
    auto& m2 = bot.insert( rofi::buildUniversalModule( 0, 0, 0 ) );
    auto& m3 = bot.insert( rofi::buildUniversalModule( 0, 0, 0 ) );
    connect( m1.connector( 3 ), m2.connector( 0 ), rofi::Orientation::South );
    connect( m2.connector( 3 ), m3.connector( 0 ), rofi::Orientation::South );
    rofi::connect< rofi::RigidJoint >( m1.body( 0 ), { 0, 0, 0 }, identity );
    REQUIRE_NOTHROW( bot.prepare() );

    SECTION( "Modules are well placed" ) {
        CHECK( equals( center( bot.getModulePosition( m1.id ) ), center( identity ) ) );
        CHECK( equals( center( bot.getModulePosition( m2.id ) ), center( translate( { 1, 0, 1 } ) ) ) );
        CHECK( equals( center( bot.getModulePosition( m3.id ) ), center( translate( { 2, 0, 2 } ) ) ) );
    }

    SECTION( "Shoes A" ) {
        CHECK( equals( m1.getComponentPosition( 6, bot.getModulePosition( m1.id ) ), identity ) );
        CHECK( equals( m2.getComponentPosition( 6, bot.getModulePosition( m2.id ) ), translate( { 1, 0, 1 } ) ) );
        CHECK( equals( m3.getComponentPosition( 6, bot.getModulePosition( m3.id ) ), translate( { 2, 0, 2 } ) ) );
    }

    SECTION( "Shoes B" ) {
        CHECK( equals( m1.getComponentPosition( 9, bot.getModulePosition( m1.id ) )
                     , translate( { 0, 0, 1 } ) * rotate( M_PI, { 0, 1, 0 } ) ) );
        CHECK( equals( m2.getComponentPosition( 9, bot.getModulePosition( m2.id ) )
                     , translate( { 1, 0, 2 } ) * rotate( M_PI, { 0, 1, 0 } ) ) );
        CHECK( equals( m3.getComponentPosition( 9, bot.getModulePosition( m3.id ) )
                     , translate( { 2, 0, 3 } ) * rotate( M_PI, { 0, 1, 0 } ) ) );
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
    CHECK( bot.modules().size() == 5 );

    CHECK( bot.roficoms().size() == 0 );
    connect( m1.connector( 3 ), m2.connector( 2 ), rofi::Orientation::North );
    connect( m2.connector( 3 ), m3.connector( 2 ), rofi::Orientation::North );
    connect( m3.connector( 3 ), m4.connector( 2 ), rofi::Orientation::North );
    connect( m4.connector( 3 ), m5.connector( 2 ), rofi::Orientation::North );
    CHECK( bot.roficoms().size() == 4 );
    rofi::connect< rofi::RigidJoint >( m1.body( 0 ), { 0, 0, 0 }, identity );
    m1.setJointParams( 2, { 90 } );
    auto [ b, str ] = bot.isValid( SimpleColision() );
    CHECK( b );
    if ( !b )
        std::cout << "Error: " << str << "\n";
}

} // namespace
