#include <catch2/catch.hpp>

#include <configuration/joints.hpp>
#include <configuration/test_aid.hpp>

namespace {

using namespace rofi::configuration;
using namespace rofi::configuration::matrices;


TEST_CASE( "Base RigidJoint" ) {
    auto j = RigidJoint( translate( { 42, 42, 42 } ) );
    CHECK( equals( j.sourceToDest(), translate( { 42, 42, 42 } ) ) );
    CHECK( equals( j.destToSource(), translate( { -42, -42, -42 } ) ) );

    j = RigidJoint( rotate( M_PI_2, { 1, 0, 0 } ) * translate( { 20, 0, 0 } ) );
    CHECK( equals( j.sourceToDest(), { { 1, 0,  0, 20 }
                                     , { 0, 0, -1,  0 }
                                     , { 0, 1,  0,  0 }
                                     , { 0, 0,  0,  1 } } ) );
    CHECK( equals( j.sourceToDest() * j.destToSource(), identity ) );
    CHECK( equals( j.sourceToDest(), rotate( M_PI_2, { 1, 0, 0 } ) * translate( { 20, 0, 0 } ) ) );
}

TEST_CASE( "Base RotationJoint" ) {
    std::vector< float > tmp{ 0 };
    SECTION( "basic creation at one point" ) {
        // Vector sourceOrigin, Vector sourceAxis, Vector destOrigin, Vector desAxis, double min, double max
        auto j = RotationJoint( { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, -90_deg, 90_deg );
        j.setPositions( tmp ); // set the angle of the rotation
        CHECK( equals( j.sourceToDest(), identity ) );
        CHECK( equals( j.destToSource(), identity ) );
    }

    SECTION( "rotation by 0" ) {
        auto j = RotationJoint( { 0, 0, 0 }, { 0, 0, 0 }, { 42, 42, 42 }, { 1, 0, 0 }, -180_deg, 180_deg );
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), translate( { 42, 42, 42 } ) ) );
        CHECK( equals( j.destToSource(), translate( { -42, -42, -42 } ) ) );
        CHECK( equals( j.sourceToDest() * j.destToSource(), identity ) );
        j = RotationJoint( identity, { 1, 0, 0 }, translate( { 42, 42, 42 } ), -90_deg, 90_deg );
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), translate( { 42, 42, 42 } ) ) );
        CHECK( equals( j.destToSource(), translate( { -42, -42, -42 } ) ) );
        CHECK( equals( j.sourceToDest() * j.destToSource(), identity ) );
    }

    SECTION( "sourceToDest on a ptr of type Joint" ) {
        auto j = RotationJoint( identity, { 1, 0, 0 }, translate( { 42, 42, 42 } ), -90_deg, 90_deg );
        REQUIRE( j.positions().size() == 1 );
        Joint& jj = j;
        tmp = { ( 90_deg ).rad() };
        jj.setPositions( tmp );
        jj.sourceToDest();
        REQUIRE( j.positions().size() == 1 );
        CHECK( j.positions()[ 0 ] == ( 90_deg ).rad() );
    }
}

TEST_CASE( "sourceToDest and destToSource" ) {
    std::vector< float > tmp{ 0 };
    SECTION( "Unit size" ) {
        auto j = RotationJoint( { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, -360_deg, 360_deg );
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), translate( {  1, 0, 0 } ) ) );
        CHECK( equals( j.destToSource(), translate( { -1, 0, 0 } ) ) );
        tmp = { 2 * M_PI };
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), translate( { 1, 0, 0 } ) ) );
        tmp = { ( 90_deg ).rad() };
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), rotate( M_PI_2, { 1, 0, 0 } ) * translate( { 1, 0, 0 } ) ) );
        CHECK( equals( j.sourceToDest() * j.destToSource(), identity ) );
        CHECK_FALSE( equals( j.destToSource(), j.sourceToDest() ) );
    }

    SECTION( "Bigger size" ) {
        auto j = RotationJoint( { 0, 0, 0 }, { 0, 0, 0 }, { 5, 0, 0 }, { 0, 1, 0 }, -180_deg, 180_deg );
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), translate( { 5, 0, 0 } ) ) );
        tmp = { ( 180_deg ).rad() };
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), rotate( M_PI, { 0, 1, 0 } ) * translate( { 5, 0, 0 } ) ) );
        CHECK( equals( center( j.sourceToDest() ), { -5, 0, 0, 1 } ) );
        tmp = { ( 90_deg ).rad() };
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), rotate( M_PI_2, { 0, 1, 0 } ) * translate( { 5, 0, 0 } ) ) );
        CHECK( equals( center( j.sourceToDest() ), { 0, 0, -5, 1 } ) );
        tmp = { ( -90_deg ).rad() };
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), rotate( -M_PI_2, { 0, 1, 0 } ) * translate( { 5, 0, 0 } ) ) );
        CHECK( equals( center( j.sourceToDest() ), { 0, 0, 5, 1 } ) );
        CHECK( equals( j.sourceToDest() * j.destToSource(), identity ) );
    }
}

TEST_CASE( "joint limits" ) {
    SECTION( "RigidJoint" ) {
        auto j = RigidJoint( identity );
        CHECK( j.positions().size() == 0 );
        CHECK( j.jointLimits().size() == 0 );
    }

    SECTION( "RotationJoint" ) {
        auto j = RotationJoint( identity, { 0, 2, 1 }, identity, -180_deg, 180_deg );
        CHECK( j.positions().size() == 1 );
        CHECK( j.jointLimits().size() == 1 );
    }
}

} // namespace
