#include <catch2/catch.hpp>

#include <configuration/serialization.hpp>
#include <configuration/test_aid.hpp>
#include <iostream>

#include <atoms/util.hpp>
#include <variant>


namespace {

using namespace rofi::configuration;
using namespace rofi::configuration::roficom;
using namespace rofi::configuration::serialization;
using namespace rofi::configuration::matrices;


TEST_CASE( "UniversalModule - Demo" ) {
    ModuleId idCounter = 0;
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( idCounter++, 0_deg, 90_deg,   0_deg ) );
    auto& m2 = world.insert( UniversalModule( idCounter++, 0_deg,  0_deg, 180_deg ) );
    auto con = m2.connectors()[ 1 ];
    connect( m1.connectors()[ 0 ], con, Orientation::South );
    connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );
    REQUIRE( world.prepare() );

    auto j = serialization::toJSON( world );
    std::string js_str = j.dump( 4 );

    RofiWorld worldj = serialization::fromJSON( j );
    auto j_cpy   = serialization::toJSON( worldj );
    std::string js_str_cpy = j_cpy.dump( 4 );

    CHECK( j == j_cpy );
    CHECK( js_str == js_str_cpy );

    REQUIRE( world.modules().size() == worldj.modules().size() );
    REQUIRE( world.roficomConnections().size() == worldj.roficomConnections().size() );
    REQUIRE( world.referencePoints().size() == worldj.referencePoints().size() );
    REQUIRE( worldj.prepare() );

    for ( auto& m : world.modules() ) {
        ModuleId id = m.module->getId();
        CHECK( equals( world.getModulePosition( id ), worldj.getModulePosition( id ) ) );
    }
}

TEST_CASE( "Empty" ) {
    SECTION( "toJSON" ) {
        RofiWorld world;
        auto js = toJSON( world );

        CHECK( js[ "modules" ].size() == 0 );
        CHECK( js[ "spaceJoints" ].size()  == 0 );
        CHECK( js[ "moduleJoints" ].size() == 0 );
    }

    SECTION( "fromJSON" ) {
        auto js = R"({ "modules" : [], "spaceJoints" : [], "moduleJoints" : [] })"_json;

        RofiWorld world = fromJSON( js );

        CHECK( world.modules().size() == 0 );
        CHECK( world.roficomConnections().size() == 0 );
        CHECK( world.referencePoints().size() == 0 );
        CHECK( world.validate( SimpleCollision() ) );
    }
}

TEST_CASE( "Pad" ) {
    ModuleId idCounter = 0;
    RofiWorld world;

    SECTION( "Square pad" ) {
        auto& m1 = world.insert( Pad( idCounter++, 20 ) );
        connect< RigidJoint >( m1.components()[ 0 ], { 0, 0, 0 }, identity );
    }

    SECTION( "Rectangle pad" ) {
        auto& m1 = world.insert( Pad( idCounter++, 20, 10 ) );
        connect< RigidJoint >( m1.components()[ 0 ], { 0, 0, 0 }, identity );
    }

    SECTION( "Multiple pads" ) {
        auto& m1 = world.insert( Pad( idCounter++, 23, 11 ) );
        auto& m2 = world.insert( Pad( idCounter++, 5 ) );
        auto& m3 = world.insert( Pad( idCounter++, 1, 10 ) );
        connect< RigidJoint >( m1.components()[ 0 ], { 0, 0, 0 }, identity );
        connect< RigidJoint >( m2.components()[ 0 ], { 0, 0, 2 }, identity );
        connect< RigidJoint >( m3.components()[ 0 ], { 0, 0, 4 }, identity );
    }

    auto j = toJSON( world );

    RofiWorld cpy = fromJSON( j );

    REQUIRE( world.prepare() );
    REQUIRE( cpy.prepare() );

    CHECK( world.roficomConnections().size() == cpy.roficomConnections().size() );
    CHECK( ModuleId(world.modules().size())  == idCounter ); // idCounter is equal to number of modules within the world
    CHECK( world.modules().size()  == cpy.modules().size()  );
    CHECK( world.referencePoints().size() == cpy.referencePoints().size() );
    CHECK( j == toJSON( cpy ) );
}

TEST_CASE( "UniversalModule" ) {
    ModuleId idCounter = 0;
    RofiWorld world;

    SECTION( "Single Module" ) {
        auto& m1 = world.insert( UniversalModule( idCounter++, 90_deg, 90_deg, 180_deg ) );
        connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );
        auto js = toJSON( world );

        CHECK( js[ "modules" ][ 0 ][ "alpha" ] == 90 );
        CHECK( js[ "modules" ][ 0 ][ "beta" ]  == 90 );
        CHECK( js[ "modules" ][ 0 ][ "gamma" ] == 180 );
    }

    SECTION( "Signle With rotational joint" ) {
        auto& m1 = world.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 0_rad ) );
        connect< RotationJoint >( m1.bodies()[ 0 ], Vector{ 0, 0, 0 }
                                , identity, Vector{ 1, 0, 0 }, translate( { 0, 0, 1 } )
                                , Angle::deg( 90 ), Angle::deg( 90 ) );
    }

    SECTION( "Multiple Modules" ) {
        auto& m1 = world.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 90_deg ) );
        auto& m2 = world.insert( UniversalModule( idCounter++, 90_deg, 0_deg, 180_deg ) );
        auto& m3 = world.insert( UniversalModule( idCounter++, 35_deg, 0_deg, 90_deg ) );

        connect( m1.components()[ 2 ], m2.components()[ 2 ], Orientation::North );
        connect( m3.components()[ 2 ], m2.components()[ 5 ], Orientation::South );
        connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );

        auto js = toJSON( world );
        CHECK( js[ "modules" ][ 0 ][ "alpha" ] == 0  );
        CHECK( js[ "modules" ][ 0 ][ "beta" ]  == 0  );
        CHECK( js[ "modules" ][ 0 ][ "gamma" ] == 90 );

        CHECK( js[ "modules" ][ 1 ][ "alpha" ] == 90  );
        CHECK( js[ "modules" ][ 1 ][ "beta" ]  == 0   );
        CHECK( js[ "modules" ][ 1 ][ "gamma" ] == 180 );

        CHECK( js[ "modules" ][ 2 ][ "alpha" ] == 35 );
        CHECK( js[ "modules" ][ 2 ][ "beta" ]  == 0  );
        CHECK( js[ "modules" ][ 2 ][ "gamma" ] == 90 );
    }

    SECTION( "The other way" ) {
        auto js = R"({
                        "modules" : [
                                        {
                                            "id" : 66,
                                            "type"  : "universal",
                                            "alpha" : 90,
                                            "beta"  : 0,
                                            "gamma" : 180
                                        },
                                        {
                                            "id" : 42,
                                            "type" : "universal",
                                            "alpha" : 0,
                                            "beta"  : 90,
                                            "gamma" : 90
                                        }
                        ],
                        "moduleJoints" : [
                                            {
                                                "orientation" : "East",
                                                "from" : {
                                                            "id" : 66,
                                                            "connector" : "A-Z"
                                                    },
                                                "to" : {
                                                            "id" : 42,
                                                            "connector" : 5
                                                    }
                                            }
                        ],
                        "spaceJoints" : [
                                            {
                                                "point" : [0, 0, 0],
                                                "to" : {
                                                            "id" : 66,
                                                            "component" : 7
                                                },
                                                "joint" : {
                                                            "type" : "rigid",
                                                            "sourceToDestination" : "identity"
                                                            }
                                                }
                        ]
        })"_json;

        world = fromJSON( js );

        CHECK( world.modules().size() == 2 );
        idCounter++; // to count inserted worlds
        auto* m = dynamic_cast< UniversalModule* >( world.getModule( 66 ) );
        REQUIRE( m );
        CHECK( m->getAlpha().deg() == 90  );
        CHECK( m->getBeta().deg()  == 0   );
        CHECK( m->getGamma().deg() == 180 );

        idCounter++;
        m = dynamic_cast< UniversalModule* >( world.getModule( 42 ) );
        REQUIRE( m );
        CHECK( m->getAlpha().deg() == 0  );
        CHECK( m->getBeta().deg()  == 90 );
        CHECK( m->getGamma().deg() == 90 );
    }

    auto j = toJSON( world );

    RofiWorld cpy = fromJSON( j );

    REQUIRE( world.prepare() );
    REQUIRE( cpy.prepare() );

    CHECK( world.roficomConnections().size() == cpy.roficomConnections().size() );
    CHECK( ModuleId(world.modules().size())  == idCounter ); // idCounter is equal to number of modules within the world
    CHECK( world.modules().size()  == cpy.modules().size()  );
    CHECK( world.referencePoints().size() == 1 );
    CHECK( world.referencePoints().size() == cpy.referencePoints().size() );
    CHECK( j == toJSON( cpy ) );
}

TEST_CASE( "UnknownModule" ) {
    ModuleId idCounter = 0;
    RofiWorld world;

    SECTION( "Basic tests" ) {
        world.insert( UnknownModule( { Component{ ComponentType::Roficom, {}, {}, nullptr }
                                   , Component{ ComponentType::Roficom, {}, {}, nullptr } }
                                   , 2
                                   , { makeComponentJoint< RigidJoint >( 0, 1, identity ) }
                                   , idCounter++ ) );

        auto js = toJSON( world );

        CHECK( js[ "modules" ].size() == 1 );
        CHECK( js[ "spaceJoints" ].size()  == 0 );
        CHECK( js[ "moduleJoints" ].size() == 0 );

        CHECK( js[ "modules" ][ 0 ][ "components" ].size() == 2 );
        CHECK( js[ "modules" ][ 0 ][ "components" ][ 0 ][ "type" ] == "roficom" );
        CHECK( js[ "modules" ][ 0 ][ "components" ][ 1 ][ "type" ] == "roficom" );
        CHECK( js[ "modules" ][ 0 ][ "joints" ].size() == 1 );
    }
}

TEST_CASE( "Mixin'" ) {
    RofiWorld world;

    SECTION( "pad + UMs" ) {
        auto& pad = world.insert( Pad( 42, 10, 8 ) );
        auto& um1 = world.insert( UniversalModule( 66, 0_deg, 0_deg, 180_deg ) );
        auto& um2 = world.insert( UniversalModule(  0, 0_deg, 0_deg, 0_deg   ) );

        connect( pad.components()[ 0 ], um1.getConnector( "A-Z" ), Orientation::North );
        connect( um1.getConnector( "B-Z" ), um2.getConnector( "A+X" ), Orientation::West );

        auto js = toJSON( world );
        CHECK( js[ "modules" ].size() == 3 );
        CHECK( js[ "moduleJoints" ].size() == 2 );
        CHECK( js == toJSON( fromJSON( js ) ) );
    }
}

TEST_CASE( "Attributes" ) {
    RofiWorld world;

    SECTION( "Single Universal Module" ) {
        auto& m1 = world.insert( UniversalModule( 0, 90_deg, 90_deg, 180_deg ) );
        connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );
        auto js = toJSON( world );

        CHECK( js[ "modules" ][ 0 ][ "alpha" ] == 90 );
        CHECK( js[ "modules" ][ 0 ][ "beta" ]  == 90 );
        CHECK( js[ "modules" ][ 0 ][ "gamma" ] == 180 );

        CHECK( !js[ "modules" ][ 0 ].contains( "attributes" ) );

        auto testAttrCallback = overload{
            []( const UniversalModule& m ) {
                return nlohmann::json::object( { { "UniversalModule", m.getId() } } );
            },
            []( auto&& ... ) { return nlohmann::json{}; }
        };

        js = toJSON( world, testAttrCallback );
        CHECK( js[ "modules" ][ 0 ].contains( "attributes" ) );
    }

    SECTION( "Different modules - different messages" ) {
        auto testAttrCallback = overload{
            []( const Module& m ) {
                return nlohmann::json::object( { { "Not an UniversalModule", m.getId() } } );
            },
            []( const UniversalModule& m ) { // This will get prioritized in case of the UM before the above
                return nlohmann::json::object( { { "UniversalModule", m.getId() } } );
            },
            []( const ComponentJoint&, int ) { return nlohmann::json{}; },
            []( const Component&, int )      { return nlohmann::json{}; },
            []( const RoficomJoint& ) { return nlohmann::json{}; },
            []( const SpaceJoint&   ) { return nlohmann::json{}; }
        };

        world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        world.insert( UniversalModule( 42, 0_deg, 0_deg, 0_deg ) );
        world.insert( Pad( 66, 2, 5 ) );

        auto js = toJSON( world, testAttrCallback );

        REQUIRE( js[ "modules" ][ 0 ].contains( "attributes" ) );
        REQUIRE( js[ "modules" ][ 1 ].contains( "attributes" ) );
        REQUIRE( js[ "modules" ][ 2 ].contains( "attributes" ) );
        // Todo: This is really ugly. Is there a better way?
        CHECK( js[ "modules" ][ 0 ][ "attributes" ].items().begin().key() == "UniversalModule" );
        CHECK( js[ "modules" ][ 0 ][ "attributes" ].items().begin().value() == 0 );
        CHECK( js[ "modules" ][ 1 ][ "attributes" ].items().begin().key() == "UniversalModule" );
        CHECK( js[ "modules" ][ 1 ][ "attributes" ].items().begin().value() == 42 );
        CHECK( js[ "modules" ][ 2 ][ "attributes" ].items().begin().key() == "Not an UniversalModule" );
        CHECK( js[ "modules" ][ 2 ][ "attributes" ].items().begin().value() == 66 );

        for ( auto j : js[ "moduleJoints" ] )
            CHECK( !j.contains( "attributes" ) );

        for ( auto s : js[ "spaceJoints" ] )
            CHECK( !s.contains( "attributes" ) );
    }

    SECTION( "Everything gets an attribute" ) {
        auto testAttrCallback = overload{
            // empty objects or arrays should not be discarded as null should
            []( const ComponentJoint& ) {
                return nlohmann::json::array();
            },
            []( const SpaceJoint& ) {
                return nlohmann::json::object();
            },
            []( auto&& ... ) {
                return "test-attr";
            }
        };

        auto& pad = world.insert( Pad( 42, 10, 8 ) );
        auto& um1 = world.insert( UniversalModule( 66, 0_deg, 0_deg, 180_deg ) );
        auto& um2 = world.insert( UniversalModule(  0, 0_deg, 0_deg, 0_deg   ) );

        connect( pad.components()[ 0 ], um1.getConnector( "A-Z" ), Orientation::North );
        connect( um1.getConnector( "B-Z" ), um2.getConnector( "A+X" ), Orientation::West );

        auto js = toJSON( world, testAttrCallback );

        for ( auto m : js[ "modules" ] ) {
            CHECK( m.contains( "attributes" ) );
        }

        for ( auto j : js[ "moduleJoints" ] )
            CHECK( j.contains( "attributes" ) );

        for ( auto s : js[ "spaceJoints" ] )
            CHECK( s.contains( "attributes" ) );
    }

    SECTION( "Nothing gets an attribute" ) {
        auto testAttrCallback = overload{
            []( auto&& ... ) {
                return nlohmann::json{};
            }
        };

        auto& pad = world.insert( Pad( 42, 10, 8 ) );
        auto& um1 = world.insert( UniversalModule( 66, 0_deg, 0_deg, 180_deg ) );
        auto& um2 = world.insert( UniversalModule(  0, 0_deg, 0_deg, 0_deg   ) );

        connect( pad.components()[ 0 ], um1.getConnector( "A-Z" ), Orientation::North );
        connect( um1.getConnector( "B-Z" ), um2.getConnector( "A+X" ), Orientation::West );

        auto js = toJSON( world, testAttrCallback );

        for ( auto m : js[ "modules" ] ) {
            CHECK( !m.items().begin().value().contains( "attributes" ) );
        }

        for ( auto j : js[ "moduleJoints" ] )
            CHECK( !j.contains( "attributes" ) );

        for ( auto s : js[ "spaceJoints" ] )
            CHECK( !s.contains( "attributes" ) );
    }
}

TEST_CASE( "Working with attributes" ) {
    RofiWorld world;

    SECTION( "Save and load IDs from attributes" ) {
        world.insert( UniversalModule( 42, 0_deg, 0_deg, 0_deg ) );
        world.insert( UniversalModule( 66, 0_deg, 0_deg, 0_deg ) );
        world.insert( UniversalModule( 0,  0_deg, 0_deg, 0_deg ) );
        world.insert( UniversalModule( 78, 0_deg, 0_deg, 0_deg ) );

        auto js = toJSON( world, overload{
                        []( const Module& m ) {
                            return nlohmann::json( m.getId() );
                        },
                        []( const ComponentJoint&, int ) { return nlohmann::json{}; },
                        []( const Component&, int )      { return nlohmann::json{}; },
                        []( const RoficomJoint& ) { return nlohmann::json{}; },
                        []( const SpaceJoint&   ) { return nlohmann::json{}; }
        } );

        std::vector< ModuleId > ids;

        auto copy = fromJSON( js, overload{
                            [ &ids ]( const nlohmann::json& j, const Module& m ) {
                                CHECK( j.get< ModuleId >() == m.getId() );
                                ids.push_back( j );
                            },
                            []( const nlohmann::json&, const ComponentJoint&, int )  { return; },
                            []( const nlohmann::json&, const Component&, int )       { return; },
                            []( const nlohmann::json&, RofiWorld::RoficomJointHandle ) { return; },
                            []( const nlohmann::json&, RofiWorld::SpaceJointHandle )   { return; },
        } );

        CHECK( ids.size() == 4 );
        std::sort( ids.begin(), ids.end() );
        CHECK( ids[ 0 ] == 0  );
        CHECK( ids[ 1 ] == 42 );
        CHECK( ids[ 2 ] == 66 );
        CHECK( ids[ 3 ] == 78 );
    }

    SECTION( "Sum all attributes" ) {
        auto js = R"({ "modules" : [ { "id" : 66,
                                       "type"  : "universal",
                                       "alpha" : 90, "beta"  : 0, "gamma" : 180,
                                       "attributes" : 1
                                     },
                                     { "id" : 42,
                                       "type" : "universal",
                                       "alpha" : 0, "beta"  : 90, "gamma" : 90,
                                       "attributes" : 2
                                     }
                                ],
                     "moduleJoints" : [ {
                                            "orientation" : "East",
                                            "from" : { "id" : 66, "connector" : "A-Z" },
                                            "to" :   { "id" : 42, "connector" : "B-Z" },
                                            "attributes" : 3
                                          }
                                        ],
                     "spaceJoints" : [ {
                                        "point" : [ 0, 0, 0 ],
                                        "to" : { "id" : 66, "component" : 7 },
                                        "attributes" : 4,
                                        "joint" : {
                                                        "type" : "rigid",
                                                        "sourceToDestination" : [
                                                                           [ 1, 0, 0, 0 ]
                                                                           , [ 0, 1, 0, 0 ]
                                                                           , [ 0, 0, 1, 0 ]
                                                                           , [ 0, 0, 0, 1 ]
                                                        ]
                                                    }
                                        }
                                    ]
                    })"_json;

        int sum = 0;
        auto copy = fromJSON( js, overload{
                            [ &sum ]( const nlohmann::json& j, const Module& )    { sum += j.get< int >(); },
                            [ &sum ]( const nlohmann::json& j, const ComponentJoint&, int )  { sum += j.get< int >(); },
                            [ &sum ]( const nlohmann::json& j, const Component&, int )       { sum += j.get< int >(); },
                            [ &sum ]( const nlohmann::json& j, RofiWorld::RoficomJointHandle ) { sum += j.get< int >(); },
                            [ &sum ]( const nlohmann::json& j, RofiWorld::SpaceJointHandle )   { sum += j.get< int >(); },
        } );

        CHECK( sum == 10 );
    }
}

TEST_CASE( "Tutorial configuration" )
{
    auto world = fromJSON( R"""({
    "modules": [
        {
            "id": 12,
            "type": "universal",
            "alpha": 90,
            "beta": 90,
            "gamma": 0
        },
        {
            "id": 42,
            "type": "pad",
            "width": 6,
            "height": 3
        }
    ],
    "moduleJoints": [
        {
            "orientation": "North",
            "from": {
                "id": 12,
                "connector": "A-Z"
            },
            "to": {
                "id": 42,
                "connector": 1
            }
        }
    ],
    "spaceJoints": [
        {
            "point": [ 0, 0, 0 ],
            "joint": {
                "type": "rigid",
                "sourceToDestination": "identity"
            },
            "to": {
                "id": 42,
                "component": 0
            }
        }
    ]
})"""_json );

    REQUIRE( world.prepare() );
    REQUIRE( world.isValid() );

    auto um12 = dynamic_cast< UniversalModule * >( world.getModule( 12 ) );
    REQUIRE( um12 );
    auto connectorB = um12->getConnector( "B-Z" );
    auto nearConnector = connectorB.getNearConnector();
    REQUIRE( nearConnector );
    CHECK( nearConnector->first.parent->getId() == 42 );
    CHECK( nearConnector->first.getIndexInParent() == 4 );
    CHECK( nearConnector->second == roficom::Orientation::South );
}

TEST_CASE( "Serialize deserialized" )
{
    SECTION( "Single module" )
    {
        auto world = fromJSON( R"""({
        "modules": [
            {
                "id": 1,
                "type": "universal",
                "alpha": 0,
                "beta": 0,
                "gamma": 0
            }
        ],
        "moduleJoints": [],
        "spaceJoints": [
            {
                "point": [ 0, 0, 0 ],
                "joint": {
                    "type": "rigid",
                    "sourceToDestination": "identity"
                },
                "to": {
                    "id": 1,
                    "component": 0
                }
            }
        ]
    })"""_json );

        REQUIRE( world.prepare() );
        REQUIRE( world.isValid() );

        auto worldToJson = toJSON( world );
        auto worldToJsonFromJson = fromJSON( worldToJson );
        auto worldToJsonFromJsonToJson = toJSON( worldToJsonFromJson );

        CHECK( worldToJson == worldToJsonFromJsonToJson );
    }
}

} // namespace
