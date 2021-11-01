#include <catch2/catch.hpp>

#include <configuration/joints.hpp>

namespace {

using namespace rofi::configuration;

const Angle A_PI_2     = Angle::rad( pi / 2 );
const Angle A_PI       = Angle::rad( pi );
const Angle A_PI_2_neg = Angle::rad( - pi / 2 );
const Angle A_PI_neg   = Angle::rad( - pi );

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
        auto j = RotationJoint( { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, A_PI_2_neg, A_PI_2 );
        j.setPositions( tmp ); // set the angle of the rotation
        CHECK( equals( j.sourceToDest(), identity ) );
        CHECK( equals( j.destToSource(), identity ) );
    }

    SECTION( "rotation by 0" ) {
        auto j = RotationJoint( { 0, 0, 0 }, { 0, 0, 0 }, { 42, 42, 42 }, { 1, 0, 0 }, A_PI_neg, A_PI );
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), translate( { 42, 42, 42 } ) ) );
        CHECK( equals( j.destToSource(), translate( { -42, -42, -42 } ) ) );
        CHECK( equals( j.sourceToDest() * j.destToSource(), identity ) );
        j = RotationJoint( identity, { 1, 0, 0 }, translate( { 42, 42, 42 } ), A_PI_2_neg, A_PI_2 );
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), translate( { 42, 42, 42 } ) ) );
        CHECK( equals( j.destToSource(), translate( { -42, -42, -42 } ) ) );
        CHECK( equals( j.sourceToDest() * j.destToSource(), identity ) );
    }

    SECTION( "sourceToDest on a ptr of type Joint" ) {
        auto j = RotationJoint( identity, { 1, 0, 0 }, translate( { 42, 42, 42 } ), A_PI_2_neg, A_PI_2 );
        REQUIRE( j.getPositions().size() == 1 );
        auto* jj = static_cast< Joint* >( &j );
        tmp = { A_PI.rad() };
        jj->sourceToDest( tmp );
        REQUIRE( !j.getPositions().empty() );
        CHECK( j.getPositions()[ 0 ] == A_PI.rad() );
    }
}

TEST_CASE( "sourceToDest and destToSource" ) {
    std::vector< float > tmp{ 0 };
    SECTION( "Unit size" ) {
        auto j = RotationJoint( { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, A_PI_2_neg, A_PI_2 );
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), translate( {  1, 0, 0 } ) ) );
        CHECK( equals( j.destToSource(), translate( { -1, 0, 0 } ) ) );
        tmp = { 2 * M_PI };
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), translate( { 1, 0, 0 } ) ) );
        tmp = { A_PI_2.rad() };
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), rotate( M_PI_2, { 1, 0, 0 } ) * translate( { 1, 0, 0 } ) ) );
        CHECK( equals( j.sourceToDest() * j.destToSource(), identity ) );
        CHECK_FALSE( equals( j.destToSource(), j.sourceToDest() ) );
    }

    SECTION( "Bigger size" ) {
        auto j = RotationJoint( { 0, 0, 0 }, { 0, 0, 0 }, { 5, 0, 0 }, { 0, 1, 0 }, A_PI_2_neg, A_PI_2 );
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), translate( { 5, 0, 0 } ) ) );
        tmp = { A_PI.rad() };
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), rotate( M_PI, { 0, 1, 0 } ) * translate( { 5, 0, 0 } ) ) );
        CHECK( equals( center( j.sourceToDest() ), { -5, 0, 0, 1 } ) );
        tmp = { A_PI_2.rad() };
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), rotate( M_PI_2, { 0, 1, 0 } ) * translate( { 5, 0, 0 } ) ) );
        CHECK( equals( center( j.sourceToDest() ), { 0, 0, -5, 1 } ) );
        tmp = { A_PI_2_neg.rad() };
        j.setPositions( tmp );
        CHECK( equals( j.sourceToDest(), rotate( -M_PI_2, { 0, 1, 0 } ) * translate( { 5, 0, 0 } ) ) );
        CHECK( equals( center( j.sourceToDest() ), { 0, 0, 5, 1 } ) );
        CHECK( equals( j.sourceToDest() * j.destToSource(), identity ) );
    }
}

TEST_CASE( "joint limits" ) {
    SECTION( "RigidJoint" ) {
        auto j = RigidJoint( identity );
        CHECK( j.positionCount() == 0 );
        CHECK_THROWS( j.jointLimits( 10 ) );
    }

    SECTION( "RotationJoint" ) {
        auto j = RotationJoint( identity, { 0, 2, 1 }, identity, A_PI_neg, A_PI );
        CHECK( j.positionCount() == 1 );
        CHECK_THROWS( j.jointLimits( 1 ) );
        CHECK_NOTHROW( j.jointLimits( 0 ) );
    }
}

} // namespace
