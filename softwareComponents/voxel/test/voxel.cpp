#include "voxel.hpp"

#include <sstream>
#include <utility>

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "configuration/serialization.hpp"
#include "configuration/test_aid.hpp"
#include "voxel/test_aid.hpp"


auto rofiWorldsEquivalent( const rofi::configuration::RofiWorld & actual,
                           const rofi::configuration::RofiWorld & expected )
        -> atoms::Result< std::monostate >
{
    REQUIRE( expected.isValid() );
    REQUIRE( actual.isValid() );

    if ( actual.modules().size() != expected.modules().size() ) {
        return atoms::result_error(
                fmt::format( "Worlds have different number of modules (actual: {}, expected: {})",
                             actual.modules().size(),
                             expected.modules().size() ) );
    }

    auto expectedVoxel = rofi::voxel::VoxelWorld::fromRofiWorld( expected );
    REQUIRE( expectedVoxel );
    auto actualVoxel = rofi::voxel::VoxelWorld::fromRofiWorld( actual );
    REQUIRE( actualVoxel );

    REQUIRE( actualVoxel->bodies.size() == expectedVoxel->bodies.size() );

    for ( const auto & expectedBody : expectedVoxel->bodies ) {
        auto actualBodies = actualVoxel->bodies;
        auto actualBody = std::ranges::find( actualBodies,
                                             expectedBody.pos,
                                             []( const rofi::voxel::Voxel & actualBody ) {
                                                 return actualBody.pos;
                                             } );
        if ( actualBody == actualBodies.end() ) {
            return atoms::result_error(
                    fmt::format( "Expected body at {}, but found none (expected: {})",
                                 nlohmann::json( expectedBody.pos ).dump(),
                                 nlohmann::json( expectedBody ).dump() ) );
        }

        if ( *actualBody != expectedBody ) {
            return atoms::result_error(
                    fmt::format( "Bodies at {} differ, (actual: {}, expected: {})",
                                 nlohmann::json( expectedBody.pos ).dump(),
                                 nlohmann::json( *actualBody ).dump(),
                                 nlohmann::json( expectedBody ).dump() ) );
        }
    }

    return atoms::result_value( std::monostate() );
}

namespace
{
using namespace std::string_literals;

using namespace rofi::configuration;
using rofi::voxel::VoxelWorld;


TEST_CASE( "Empty" )
{
    CHECK_FALSE( VoxelWorld().toRofiWorld() );
    CHECK_FALSE( VoxelWorld::fromRofiWorld( RofiWorld() ) );
}

TEST_CASE( "Default position - 1 module" )
{
    auto rofiWorld = serialization::fromJSON( R"({
    "modules" : [
        {
            "id" : 1,
            "type" : "universal",
            "alpha" : 0,
            "beta"  : 0,
            "gamma" : 0
        }
    ],
    "moduleJoints" : [],
    "spaceJoints" : [
        {
            "point" : [0, 0, 0],
            "to" : {
                "id" : 1,
                "component" : 0
            },
            "joint" : {
                "type" : "rigid",
                "sourceToDestination" : "identity"
            }
        }
    ]
})"_json );

    REQUIRE( rofiWorld.validate() );

    auto voxelWorld = VoxelWorld( R"({
    "bodies" : [
        {
            "pos" : [0, 0, 0],
            "other_body_dir" : {
                "axis" : "Z",
                "is_positive" : true
            },
            "is_shoe_rotated" : false,
            "joint_pos" : 0
        },
        {
            "pos" : [0, 0, 1],
            "other_body_dir" : {
                "axis" : "Z",
                "is_positive" : false
            },
            "is_shoe_rotated" : false,
            "joint_pos" : 0
        }
    ]
})"_json );

    SECTION( "Rofi World -> Voxel World" )
    {
        auto convertedVoxelWorld = VoxelWorld::fromRofiWorld( rofiWorld );
        REQUIRE( convertedVoxelWorld );

        CHECK( *convertedVoxelWorld == voxelWorld );
    }

    SECTION( "Voxel World -> Rofi World" )
    {
        auto convertedRofiWorld = voxelWorld.toRofiWorld();
        REQUIRE( convertedRofiWorld );
        CAPTURE( *convertedRofiWorld );
        REQUIRE( convertedRofiWorld->prepare() );
        REQUIRE( convertedRofiWorld->validate() );

        REQUIRE( rofiWorldsEquivalent( *convertedRofiWorld, rofiWorld ) );

        INFO( "This is not mandatory, but expected" );
        CHECK( serialization::toJSON( *convertedRofiWorld ) == serialization::toJSON( rofiWorld ) );
    }
}

TEST_CASE( "Orientation" )
{
    auto rofiWorld = RofiWorld();

    auto & um1 = rofiWorld.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
    auto & um2 = rofiWorld.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );

    connect< RigidJoint >( um1.connectors()[ 0 ], Vector{ 0, 0, 0 }, matrices::identity );

    auto orientation = GENERATE( roficom::Orientation::North,
                                 roficom::Orientation::East,
                                 roficom::Orientation::South,
                                 roficom::Orientation::West );

    REQUIRE( um1.connectors().size() == 6 );
    auto connector1 = um1.connectors()[ GENERATE( range( 0, 6 ) ) ];
    REQUIRE( um2.connectors().size() == 6 );
    auto connector2 = um2.connectors()[ GENERATE( range( 0, 6 ) ) ];

    connect( connector1, connector2, orientation );

    REQUIRE( rofiWorld.validate() );

    auto voxelWorld = VoxelWorld::fromRofiWorld( rofiWorld );
    REQUIRE( voxelWorld );

    auto convertedRofiWorld = voxelWorld->toRofiWorld();
    REQUIRE( convertedRofiWorld );
    CAPTURE( *convertedRofiWorld );
    REQUIRE( convertedRofiWorld->prepare() );
    REQUIRE( convertedRofiWorld->validate() );

    CHECK( rofiWorldsEquivalent( *convertedRofiWorld, rofiWorld ) );
}

TEST_CASE( "All possible configurations - 1 module" )
{
    auto rofiWorld = RofiWorld();

    auto alpha = GENERATE( 0_deg, 90_deg, -90_deg );
    auto beta = GENERATE( 0_deg, 90_deg, -90_deg );
    auto gamma = GENERATE( 0_deg, 90_deg, 180_deg, -90_deg );

    CAPTURE( alpha, beta, gamma );

    auto & um = rofiWorld.insert( UniversalModule( 42, alpha, beta, gamma ) );

    auto refPoint = GENERATE( Vector{ 0, 0, 0 }, Vector{ 2, 4, 6 } );

    auto zRotate = GENERATE( 0_deg, 90_deg, 180_deg, 270_deg );

    auto rotateZFaceIdx = GENERATE( range( 0, 6 ) );
    auto rotateZFace = std::array{
            matrices::identity,
            matrices::rotate( ( 180_deg ).rad(), { 1, 0, 0 } ),
            matrices::rotate( ( 90_deg ).rad(), { 1, 0, 0 } ),
            matrices::rotate( ( -90_deg ).rad(), { 1, 0, 0 } ),
            matrices::rotate( ( 90_deg ).rad(), { 0, 1, 0 } ),
            matrices::rotate( ( -90_deg ).rad(), { 0, 1, 0 } ),
    }[ rotateZFaceIdx ];

    CAPTURE( refPoint, zRotate, rotateZFaceIdx );

    auto transform = Matrix( matrices::rotate( zRotate.rad(), { 0, 0, 1 } ) * rotateZFace );
    REQUIRE_NOTHROW( connect< RigidJoint >( um.connectors()[ 0 ], refPoint, transform ) );

    CAPTURE( rofiWorld );
    REQUIRE( rofiWorld.validate() );

    auto voxelWorld = VoxelWorld::fromRofiWorld( rofiWorld );
    REQUIRE( voxelWorld );
    CAPTURE( voxelWorld );

    auto convertedRofiWorld = voxelWorld->toRofiWorld();
    REQUIRE( convertedRofiWorld );
    CAPTURE( *convertedRofiWorld );
    REQUIRE( convertedRofiWorld->prepare() );
    REQUIRE( convertedRofiWorld->validate() );
    CAPTURE( VoxelWorld::fromRofiWorld( *convertedRofiWorld ) );

    CHECK( rofiWorldsEquivalent( *convertedRofiWorld, rofiWorld ) );
}

TEST_CASE( "All possible configurations - 2 modules", "[!hide]" )
{
    auto rofiWorld = RofiWorld();

    auto alpha1 = GENERATE( 0_deg, 90_deg, -90_deg );
    auto alpha2 = GENERATE( 0_deg, 90_deg, -90_deg );
    auto beta1 = GENERATE( 0_deg, 90_deg, -90_deg );
    auto beta2 = GENERATE( 0_deg, 90_deg, -90_deg );
    auto gamma1 = GENERATE( 0_deg, 90_deg );
    auto gamma2 = GENERATE( 180_deg, 270_deg );

    auto & um1 = rofiWorld.insert( UniversalModule( 1, alpha1, beta1, gamma1 ) );
    auto & um2 = rofiWorld.insert( UniversalModule( 2, alpha2, beta2, gamma2 ) );

    auto refPoint = Vector{ 0, 0, 0 };
    auto transform = matrices::identity;
    connect< RigidJoint >( um1.connectors()[ 0 ], refPoint, transform );

    auto orientation = GENERATE( roficom::Orientation::North,
                                 roficom::Orientation::East,
                                 roficom::Orientation::South,
                                 roficom::Orientation::West );

    REQUIRE( um1.connectors().size() == 6 );
    auto connector1 = um1.connectors()[ GENERATE( range( 0, 6 ) ) ];
    REQUIRE( um2.connectors().size() == 6 );
    auto connector2 = um2.connectors()[ GENERATE( range( 0, 6 ) ) ];

    connect( connector1, connector2, orientation );

    CAPTURE( rofiWorld );
    REQUIRE( rofiWorld.prepare() );
    REQUIRE( rofiWorld.validate() );

    auto voxelWorld = VoxelWorld::fromRofiWorld( rofiWorld );
    REQUIRE( voxelWorld );

    auto convertedRofiWorld = voxelWorld->toRofiWorld();
    REQUIRE( convertedRofiWorld );
    CAPTURE( *convertedRofiWorld );
    REQUIRE( convertedRofiWorld->prepare() );
    REQUIRE( convertedRofiWorld->validate() );


    CHECK( rofiWorldsEquivalent( *convertedRofiWorld, rofiWorld ) );
}

} // namespace
