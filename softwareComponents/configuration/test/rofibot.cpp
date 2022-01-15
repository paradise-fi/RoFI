#include <catch2/catch.hpp>

#include <configuration/rofibot.hpp>
#include <configuration/universalModule.hpp>
#include <configuration/unknownModule.hpp>

namespace {

using namespace rofi::configuration;
using namespace rofi::configuration::roficom;
using namespace rofi::configuration::matrices;

TEST_CASE( "UnknownModule (base Module) Test" ) {
    auto m = UnknownModule( { Component{ ComponentType::Roficom } }, 1, {}, 42 );
    CHECK( m.bodies().size() == 0 );
    CHECK( m.components().size() == 1 );
    CHECK( m.connectors().size() == 1 );
    CHECK( m.getId() == 42 );
    CHECK( m.setId( 66 ) );
    CHECK( m.getId() == 66 );
}

TEST_CASE( "Universal Module Test" ) {
    SECTION( "Creation" ) {
        auto um = UniversalModule( 0, 0_deg, 0_deg, 0_deg );
        CHECK( um.components().size() == 10 );
        um.prepare();
        REQUIRE( um.getOccupiedPositions().size() == 2 );
        CHECK( equals( um.getOccupiedPositions()[ 0 ], identity ) );
        CHECK( equals( center( um.getOccupiedPositions()[ 1 ] ), { 0, 0, 1, 1 } ) );
    }

    SECTION( "Roficoms" ) {
        auto um = UniversalModule( 0, 0_deg, 0_deg, 0_deg );
        CHECK( um.connectors().size() == 6 );

        for ( int i = 0; i < um.connectors().size(); i++ ) {
            INFO( "Connector number: " << i );
            CHECK( um.connectors()[ i ].type == ComponentType::Roficom );
        }

        REQUIRE( um.components().size() >= 6 );
        for ( int i = 0; i < um.components().size(); i++ ) {
            INFO( "Number of connectors: " << um.connectors().size() );
            INFO( "Component number: " << i );
            if ( i < um.connectors().size() ) {
                CHECK( um.components()[ i ].type == ComponentType::Roficom );
            } else {
                CHECK( um.components()[ i ].type != ComponentType::Roficom );
            }
        }
    }

    SECTION( "Position - default" ) {
        auto um = UniversalModule( 0, 0_deg, 0_deg, 0_deg );
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
        SECTION( "degrees" ) {
            auto um = UniversalModule( 0, 0_deg, 0_deg, 90_deg );
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
    }

    SECTION( "radians" ) {
        auto um = UniversalModule( 0, 0_rad, 0_rad, 1.57079632679489661923_rad ); // 0, 0, M_PI_2
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
        SECTION( "degrees" ) {
            auto um = UniversalModule( 0, 0_deg, Angle::deg( -90 ), 90_deg );
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

        SECTION( "radians" ) {
            auto um = UniversalModule( 0, 0_rad, Angle::rad( -1.57079632679489661923 ), 1.57079632679489661923_rad );
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

    SECTION( "Connector translations" ) {
        CHECK( UniversalModule::translateComponent( "A-X" ) == 0 );
        CHECK( UniversalModule::translateComponent( "A+X" ) == 1 );
        CHECK( UniversalModule::translateComponent( "A-Z" ) == 2 );
        CHECK( UniversalModule::translateComponent( "B-X" ) == 3 );
        CHECK( UniversalModule::translateComponent( "B+X" ) == 4 );
        CHECK( UniversalModule::translateComponent( "B-Z" ) == 5 );
    }
}

TEST_CASE( "Two modules next to each other" ) {
    ModuleId idCounter = 0;
    Rofibot bot;
    auto& m1 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    auto con = m2.connectors()[ 1 ];
    connect( m1.connectors()[ 0 ], con, Orientation::South );
    connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );
    REQUIRE_NOTHROW( bot.prepare() );

    SECTION( "The second is just moved to left by one" ) {
        Matrix new_origin = identity * translate( { -1, 0, 0 } );
        REQUIRE( !equals( identity, new_origin ) );
        Matrix mat = identity;
        CHECK( equals( center( m2.getComponentPosition( 0, bot.getModulePosition( m2.getId() ) ) )
                     , center( new_origin ) ) );

        mat = new_origin * rotate( M_PI, { 0, 0, 1 } );
        CHECK( equals( center( m2.getComponentPosition( 1, bot.getModulePosition( m2.getId() ) ) ), center( mat ) ) );

        mat = new_origin * rotate( - M_PI_2, { 0, 1, 0 } );
        CHECK( equals( center( m2.getComponentPosition( 2, bot.getModulePosition( m2.getId() ) ) ), center( mat ) ) );
        CHECK( equals( center( m2.getComponentPosition( 6, bot.getModulePosition( m2.getId() ) ) ), center( new_origin ) ) );
        CHECK( equals( center( m2.getComponentPosition( 7, bot.getModulePosition( m2.getId() ) ) ), center( new_origin ) ) );

        mat =  new_origin * m1.getComponentPosition( 3 );
        CHECK( equals( center( m2.getComponentPosition( 3, bot.getModulePosition( m2.getId() ) ) ), center( mat ) ) );

        mat = new_origin * m1.getComponentPosition( 4 );
        CHECK( equals( center( m2.getComponentPosition( 4, bot.getModulePosition( m2.getId() ) ) ), center( mat ) ) );

        mat = new_origin * m1.getComponentPosition( 5 );
        CHECK( equals( center( m2.getComponentPosition( 5, bot.getModulePosition( m2.getId() ) ) ), center( mat ) ) );

        mat = new_origin * m1.getComponentPosition( 8 );
        CHECK( equals( center( m2.getComponentPosition( 8, bot.getModulePosition( m2.getId() ) ) ), center( mat ) ) );

        mat = { { -1, 0, 0, -1 }, { 0, 1, 0, 0 }, { 0, 0, -1, 1 }, { 0, 0, 0, 1 } };
        CHECK( equals( center( m2.getComponentPosition( 9, bot.getModulePosition( m2.getId() ) ) ), center( mat ) ) );

        mat = new_origin * m1.getComponentPosition( 9 );
        CHECK( equals( center( m2.getComponentPosition( 9, bot.getModulePosition( m2.getId() ) ) ), center( mat ) ) );
    }
}

TEST_CASE( "Two modules - different angles" ) {
    int idCounter = 0;
    Rofibot bot;
    auto& m1 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    connect( m1.connectors()[ 3 ], m2.connectors()[ 0 ], Orientation::South );
    connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );
    REQUIRE_NOTHROW( bot.prepare() );

    SECTION( "BodyA " ) {
        CHECK( equals( bot.getModulePosition( m1.getId() ), identity ) );
        CHECK( equals( center( bot.getModulePosition( m2.getId() ) ), center( identity * translate( { 1, 0, 1 } ) ) ) );
    }

    SECTION( "BodyB" ) {
    Matrix m1shoeB = m1.getComponentPosition( 9 );
    Matrix m2shoeB = m2.getComponentPosition( 9, bot.getModulePosition( m2.getId() ) );
    CHECK( equals( m1shoeB, { { -1, 0,  0, 0 }
                            , {  0, 1,  0, 0 }
                            , {  0, 0, -1, 1 }
                            , {  0, 0,  0, 1 } } ) );
    CHECK( equals( center( m2shoeB ), center( translate( { 1, 0, 2 } ) ) ) );
    }
}

TEST_CASE( "Three modules -- connect docks 3 to 0s " ) {
    int idCounter = 0;
    Rofibot bot;
    auto& m1 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    auto& m3 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    connect( m1.connectors()[ 3 ], m2.connectors()[ 0 ], Orientation::South );
    connect( m2.connectors()[ 3 ], m3.connectors()[ 0 ], Orientation::South );
    connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );
    REQUIRE_NOTHROW( bot.prepare() );

    SECTION( "Modules are well placed" ) {
        CHECK( equals( center( bot.getModulePosition( m1.getId() ) ), center( identity ) ) );
        CHECK( equals( center( bot.getModulePosition( m2.getId() ) ), center( translate( { 1, 0, 1 } ) ) ) );
        CHECK( equals( center( bot.getModulePosition( m3.getId() ) ), center( translate( { 2, 0, 2 } ) ) ) );
    }

    SECTION( "Shoes A" ) {
        CHECK( equals( m1.getComponentPosition( 6, bot.getModulePosition( m1.getId() ) ), identity ) );
        CHECK( equals( m2.getComponentPosition( 6, bot.getModulePosition( m2.getId() ) ), translate( { 1, 0, 1 } ) ) );
        CHECK( equals( m3.getComponentPosition( 6, bot.getModulePosition( m3.getId() ) ), translate( { 2, 0, 2 } ) ) );
    }

    SECTION( "Shoes B" ) {
        CHECK( equals( m1.getComponentPosition( 9, bot.getModulePosition( m1.getId() ) )
                     , translate( { 0, 0, 1 } ) * rotate( M_PI, { 0, 1, 0 } ) ) );
        CHECK( equals( m2.getComponentPosition( 9, bot.getModulePosition( m2.getId() ) )
                     , translate( { 1, 0, 2 } ) * rotate( M_PI, { 0, 1, 0 } ) ) );
        CHECK( equals( m3.getComponentPosition( 9, bot.getModulePosition( m3.getId() ) )
                     , translate( { 2, 0, 3 } ) * rotate( M_PI, { 0, 1, 0 } ) ) );
    }
}

TEST_CASE( "Basic rofibot manipulation" ) {
    using namespace rofi;
    int idCounter = 0;
    Rofibot bot;
    auto& m1 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    CHECK( m1.getId() == 0 );
    CHECK( bot.getModule( 0 )->getId() == 0 );
    REQUIRE( m1.getId() == 0 );
    REQUIRE( m1.parent == &bot );
    CHECK( &m1 == bot.getModule( 0 ) );
    auto& m2 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    REQUIRE( m2.getId() == 1 );
    CHECK( m1.getId() == 0 );
    CHECK( bot.getModule( 0 )->getId() == 0 );
    REQUIRE( &m1 == bot.getModule( 0 ) );
    auto& m3 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    REQUIRE( m3.getId() == 2 );
    REQUIRE( m1.parent == &bot );
    auto& m4 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    REQUIRE( m4.getId() == 3 );
    auto& m5 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    REQUIRE( m5.getId() == 4 );
    CHECK( bot.modules().size() == 5 );

    CHECK( bot.roficoms().size() == 0 );
    connect( m1.connectors()[ 5 ], m2.connectors()[ 2 ], Orientation::North );
    connect( m2.connectors()[ 5 ], m3.connectors()[ 2 ], Orientation::North );
    connect( m3.connectors()[ 5 ], m4.connectors()[ 2 ], Orientation::North );
    connect( m4.connectors()[ 5 ], m5.connectors()[ 2 ], Orientation::North );
    CHECK( bot.roficoms().size() == 4 );
    connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );
    static_cast< UniversalModule& >( m1 ).setGamma( Angle::rad( M_PI_2 ) );
    auto [ b, str ] = bot.isValid( SimpleColision() );
    CHECK( !b ); // because the configuration is not prepared
    auto [ b2, str2 ] = bot.validate( SimpleColision() );
    CHECK( b2 );
    if ( !b2 )
        std::cout << "Error: " << str2 << "\n";
}

TEST_CASE( "Colliding configuration" ) {
    using namespace rofi;
    int idCounter = 0;
    Rofibot bot;
    auto& m1 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    auto& m3 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    auto& m4 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    auto& m5 = bot.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_deg ) );
    CHECK( bot.modules().size() == 5 );

    connect( m1.connectors()[ 1 ], m2.connectors()[ 2 ], Orientation::North );
    connect( m2.connectors()[ 1 ], m3.connectors()[ 2 ], Orientation::North );
    connect( m3.connectors()[ 1 ], m4.connectors()[ 2 ], Orientation::North );
    connect( m4.connectors()[ 1 ], m5.connectors()[ 2 ], Orientation::North );
    CHECK( bot.roficoms().size() == 4 );
    connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );
    auto [ b, str ] = bot.validate( SimpleColision() );
    CHECK( !b );
}

TEST_CASE( "Changing modules ID" ) {
    using namespace rofi;
    Rofibot bot;

    auto& m1 = bot.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = bot.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
    auto& m3 = bot.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );
    connect( m1.connectors()[ 5 ], m2.connectors()[ 2 ], Orientation::North );
    connect( m2.connectors()[ 5 ], m3.connectors()[ 2 ], Orientation::North );
    connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );

    CHECK( bot.validate( SimpleColision() ).first );

    CHECK( m1.getId() == 0 );
    CHECK( m2.getId() == 1 );
    CHECK( m3.getId() == 2 );

    CHECK( m2.setId( 42 ) );
    CHECK( bot.validate( SimpleColision() ).first );
    CHECK( m1.getId() == 0 );
    CHECK( m2.getId() != 1 );
    CHECK( m2.getId() == 42 );
    CHECK( m3.getId() == 2 );

    CHECK( !m2.setId( 0 ) );
    CHECK( m1.getId() == 0 );
    CHECK( m2.getId() == 42 );
    CHECK( m3.getId() == 2 );

    CHECK( m1.setId( 66 ) );
    CHECK( m3.setId( 78 ) );

    CHECK( m1.getId() == 66 );
    CHECK( m2.getId() == 42 );
    CHECK( m3.getId() == 78 );
}

TEST_CASE( "Configurable joints" ) {
    SECTION( "default" ) {
        auto m = UniversalModule( 42, 0_deg, 0_rad, 0_deg );
        int i = 0;
        for ( auto& j : m.configurableJoints() ) {
            static_assert( std::is_same_v< decltype( j ), Joint & > );
            i++;
        }
        REQUIRE( i == 3 );
    }
    SECTION( "const" ) {
        auto m = UniversalModule( 42, 0_deg, 0_rad, 0_deg );
        int i = 0;
        for ( auto& j : std::as_const( m ).configurableJoints() ) {
            static_assert( std::is_same_v< decltype( j ), const Joint & > );
            i++;
        }
        CHECK( i == 3 );
    }
    SECTION( "equality" ) {
        auto m = UniversalModule( 42, 0_deg, 0_rad, 0_deg );

        auto joints = m.configurableJoints();
        auto cjoints = std::as_const( m ).configurableJoints();

        auto it = joints.begin();
        auto cit = cjoints.begin();
        while ( it != joints.end() && cit != cjoints.end() ) {
            CHECK( &*it == &*cit ); // Check address
            ++it;
            ++cit;
        }
        CHECK( it == joints.end() );
        CHECK( cit == cjoints.end() );
    }
}

} // namespace
