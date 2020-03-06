#include <catch2/catch.hpp>

#include "roficomConnect.hpp"

#include <ignition/math/Angle.hh>


using ConnectorState = rofi::messages::ConnectorState;
using namespace ignition::math;

using detail::checkCenterDistance;
using detail::checkTilt;
using detail::checkShift;
using detail::getMutualOrientation;


template< typename Function >
void forAllOrientations( Function function, const Pose3d & lhs, const Pose3d & rhs )
{
    for ( const auto & rotation : {
            Pose3d( 0, 0, 0, 0, 0, 0 ),
            Pose3d( 0, 0, 0, 0, 0, Angle::HalfPi() ),
            Pose3d( 0, 0, 0, 0, 0, Angle::Pi() ),
            Pose3d( 0, 0, 0, 0, 0, -Angle::HalfPi() ),
        } )
    {
        function( lhs, rotation * rhs );
    }
}

void checkAllGoodImpl( const Pose3d & lhs, const Pose3d & rhs )
{
    INFO( "lhs: " << lhs << ", rhs: " << rhs );

    CHECK( checkCenterDistance( lhs.Pos(), rhs.Pos() ) );
    CHECK( checkShift( lhs, rhs ) );
    CHECK( checkTilt( lhs.Rot(), rhs.Rot() ) );
    auto mutualOrientation = getMutualOrientation( lhs.Rot(), rhs.Rot() );
    CHECK( mutualOrientation );

    auto result = canBeConnected( lhs, rhs );
    CHECK( result );

    CHECK( ( !result || result == mutualOrientation ) );
}

void checkAllGood( const Pose3d & lhs, const Pose3d & rhs )
{
    forAllOrientations( checkAllGoodImpl, lhs, rhs );
}

void checkAllGoodAtSamePosition( const Quaterniond & lhs, const Quaterniond & rhs )
{
    auto pos = Vector3d(); // TODO make random
    return checkAllGood( { pos, lhs }, { pos, rhs } );
}

TEST_CASE( "Check all" )
{
    SECTION( "Perfect" )
    {
        checkAllGood( { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, Angle::Pi(), 0, 0 } );
        checkAllGood( { 0, 0, 0, Angle::Pi(), 0, 0 }, { 0, 0, 0, 0, 0, 0 } );
        checkAllGood( { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, Angle::Pi(), 0 } );
        checkAllGood( { 0, 0, 0, 0, Angle::Pi(), 0 }, { 0, 0, 0, 0, 0, 0 } );
        checkAllGood( { 0, 0, 0, 0, 0, Angle::Pi() }, { 0, 0, 0, Angle::Pi(), 0, 0 } );
        checkAllGood( { 0, 0, 0, Angle::Pi(), 0, Angle::Pi() }, { 0, 0, 0, 0, 0, 0 } );
        checkAllGood( { 0, 0, 0, 0, 0, Angle::HalfPi() }, { 0, 0, 0, 0, Angle::Pi(), 0 } );
        checkAllGood( { 0, 0, 0, 0, Angle::Pi(), 0 }, { 0, 0, 0, 0, 0, -Angle::HalfPi() } );
        checkAllGood( { 0, 0, 0, 0, Angle::Pi(), 0 }, { 0, 0, 0, 0, 0, Angle::HalfPi() } );
    }

    SECTION( "Small shift" )
    {
        checkAllGood( { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0.002, Angle::Pi(), 0, 0 } );
        checkAllGood( { 0, 0, 10, 0, 0, 0  }, { 0, 0, 10.002, Angle::Pi(), 0, 0 } );
        checkAllGood( { 0.001, 0.001, 0.001, 0, 0, 0 }, { 0, 0, 0, Angle::Pi(), 0, 0 } );
        checkAllGood( { 0.002, 0, 0, 0, 0, 0 }, { 0, 0, 0, Angle::Pi(), 0, 0 } );
        checkAllGood( { 0, 0.002, 0, 0, 0, 0 }, { 0, 0, 0, Angle::Pi(), 0, 0 } );
    }

    SECTION( "Different orientations" )
    {
        SECTION( "North" )
        {
            checkAllGoodAtSamePosition( { 0, 0, 0 }, { Angle::Pi(), 0, 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), 0, 0 }, { 0, 0, 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), Angle::Pi(), 0 }, { 0, Angle::Pi(), 0 } );
        }

        SECTION( "South" )
        {
            checkAllGoodAtSamePosition( { 0, 0, 0 }, { Angle::Pi(), Angle::Pi(), 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), Angle::Pi(), 0 }, { 0, 0, 0 } );
            checkAllGoodAtSamePosition( { 0, Angle::Pi(), 0 }, { Angle::Pi(), 0, 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), 0, 0 }, { 0, Angle::Pi(), 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), Angle::HalfPi(), 0 }, { 0, Angle::HalfPi(), 0 } );
        }

        SECTION( "East" )
        {
            checkAllGoodAtSamePosition( { 0, 0, 0 }, { Angle::Pi(), Angle::HalfPi(), 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), Angle::HalfPi(), 0 }, { 0, 0, 0 } );
            checkAllGoodAtSamePosition( { 0, Angle::HalfPi(), 0 }, { Angle::Pi(), 0, 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), 0, 0 }, { 0, Angle::HalfPi(), 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), Angle::HalfPi() / 2, 0 }, { 0, Angle::HalfPi() / 2, 0 } );
        }

        SECTION( "West" )
        {
            checkAllGoodAtSamePosition( { 0, 0, 0 }, { Angle::Pi(), -Angle::HalfPi(), 0 } );
            checkAllGoodAtSamePosition( { 0, 0, 0 }, { Angle::Pi(), 3 * Angle::HalfPi(), 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), -Angle::HalfPi(), 0 }, { 0, 0, 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), 3 * Angle::HalfPi(), 0 }, { 0, 0, 0 } );
            checkAllGoodAtSamePosition( { 0, -Angle::HalfPi(), 0 }, { Angle::Pi(), 0, 0 } );
            checkAllGoodAtSamePosition( { Angle::HalfPi(), 3 * Angle::HalfPi(), 0 }, { -Angle::HalfPi(), 0, 0 } );
            checkAllGoodAtSamePosition( { 3 * Angle::HalfPi(), -Angle::HalfPi(), 0 }, { Angle::HalfPi(), 0, 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), 0, 0 }, { 0, 3 * Angle::HalfPi(), 0 } );
            checkAllGoodAtSamePosition( { Angle::Pi(), -Angle::HalfPi() / 2, 0 }, { 0, -Angle::HalfPi() / 2, 0 } );
        }
    }
}

TEST_CASE( "Check shift" )
{
    CHECK( checkShift( { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } ) );
    CHECK( checkShift( { 234.003, -50, 10, 0, 0, 0 }, { 234, -50, 10, 0, 0, 0 } ) );
    CHECK( checkShift( { 0.001, 0.001, 0.001, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } ) );
    CHECK( checkShift( { 0.002, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } ) );
    CHECK( checkShift( { 0, 0.002, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } ) );

    CHECK_FALSE( checkShift( { 0, 0.008, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } ) );
    CHECK_FALSE( checkShift( { 0, 0, 10, 0, 0, 0 }, { 0, 0.02, 10, 0, 0, 0 } ) );

    CHECK_FALSE( checkShift( { 10, 0.04, 0, 0, 0, 0 }, { 10, 0, 0, 0, 0, 0 } ) );
    CHECK_FALSE( checkShift( { 0, 0, 10, 0, 0, 0 }, { 0.01, 0, 10, 0, 0, 0 } ) );
    CHECK_FALSE( checkShift( { 10, 0.005, 0, 0, 0, 0 }, { 10.003, 0, 0.005, 0, 0, 0 } ) );
}

TEST_CASE( "Check tilt" )
{
    CHECK( ( false && "TODO add tests" ) );
}

TEST_CASE( "Check center distance" )
{
    CHECK( checkCenterDistance( { 0, 0, 0 }, { 0, 0, 0.002 } ) );
    CHECK( checkCenterDistance( { 0, 0, 10 }, { 0, 0, 10.002 } ) );
    CHECK( checkCenterDistance( { 0.001, 0.001, 0.001 }, { 0, 0, 0 } ) );
    CHECK( checkCenterDistance( { 0.002, 0, 0 }, { 0, 0, 0 } ) );
    CHECK( checkCenterDistance( { 0, 0.002, 0 }, { 0, 0, 0 } ) );

    CHECK_FALSE( checkCenterDistance( { 0, 0, 0 }, { 0, 0, 0.02 } ) );
    CHECK_FALSE( checkCenterDistance( { 0, 0, 10 }, { 0, 0, 10.02 } ) );

    CHECK_FALSE( checkCenterDistance( { 0, 0.04, 0 }, { 0, 0, 0 } ) );
    CHECK_FALSE( checkCenterDistance( { 0, 0, 10 }, { 0.01, 0, 10 } ) );
    CHECK_FALSE( checkCenterDistance( { 10, 0.005, 0 }, { 10, 0, 0.005 } ) );
}

TEST_CASE( "Orientation" )
{
    SECTION( "North" )
    {
        SECTION( "Has some value" )
        {
            CHECK( getMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), 0, 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 0, 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), Angle::Pi(), 0 }, { 0, Angle::Pi(), 0 } ) );
        }

        SECTION( "Has right value" )
        {
            CHECK( getMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), 0, 0 } ).value() == ConnectorState::NORTH );
            CHECK( getMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 0, 0 } ).value() == ConnectorState::NORTH );
            CHECK( getMutualOrientation( { Angle::Pi(), Angle::Pi(), 0 }, { 0, Angle::Pi(), 0 } ).value() == ConnectorState::NORTH );
        }
    }

    SECTION( "South" )
    {
        SECTION( "Has some value" )
        {
            CHECK( getMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), Angle::Pi(), 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), Angle::Pi(), 0 }, { 0, 0, 0 } ) );
            CHECK( getMutualOrientation( { 0, Angle::Pi(), 0 }, { Angle::Pi(), 0, 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, Angle::Pi(), 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), Angle::HalfPi(), 0 }, { 0, Angle::HalfPi(), 0 } ) );
        }
        SECTION( "Has right value" )
        {
            CHECK( getMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), Angle::Pi(), 0 } ).value() == ConnectorState::SOUTH );
            CHECK( getMutualOrientation( { Angle::Pi(), Angle::Pi(), 0 }, { 0, 0, 0 } ).value() == ConnectorState::SOUTH );
            CHECK( getMutualOrientation( { 0, Angle::Pi(), 0 }, { Angle::Pi(), 0, 0 } ).value() == ConnectorState::SOUTH );
            CHECK( getMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, Angle::Pi(), 0 } ).value() == ConnectorState::SOUTH );
            CHECK( getMutualOrientation( { Angle::Pi(), Angle::HalfPi(), 0 }, { 0, Angle::HalfPi(), 0 } ).value() == ConnectorState::SOUTH );
        }
    }

    SECTION( "East" )
    {
        SECTION( "Has some value" )
        {
            CHECK( getMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), Angle::HalfPi(), 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), Angle::HalfPi(), 0 }, { 0, 0, 0 } ) );
            CHECK( getMutualOrientation( { 0, Angle::HalfPi(), 0 }, { Angle::Pi(), 0, 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, Angle::HalfPi(), 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), Angle::HalfPi() / 2, 0 }, { 0, Angle::HalfPi() / 2, 0 } ) );
        }
        SECTION( "Has right value" )
        {
            CHECK( getMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), Angle::HalfPi(), 0 } ).value() == ConnectorState::EAST );
            CHECK( getMutualOrientation( { Angle::Pi(), Angle::HalfPi(), 0 }, { 0, 0, 0 } ).value() == ConnectorState::EAST );
            CHECK( getMutualOrientation( { 0, Angle::HalfPi(), 0 }, { Angle::Pi(), 0, 0 } ).value() == ConnectorState::EAST );
            CHECK( getMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, Angle::HalfPi(), 0 } ).value() == ConnectorState::EAST );
            CHECK( getMutualOrientation( { Angle::Pi(), Angle::HalfPi() / 2, 0 }, { 0, Angle::HalfPi() / 2, 0 } ).value() == ConnectorState::EAST );
        }
    }

    SECTION( "West" )
    {
        SECTION( "Has some value" )
        {
            CHECK( getMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), -Angle::HalfPi(), 0 } ) );
            CHECK( getMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), 3 * Angle::HalfPi(), 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), -Angle::HalfPi(), 0 }, { 0, 0, 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), 3 * Angle::HalfPi(), 0 }, { 0, 0, 0 } ) );
            CHECK( getMutualOrientation( { 0, -Angle::HalfPi(), 0 }, { Angle::Pi(), 0, 0 } ) );
            CHECK( getMutualOrientation( { Angle::HalfPi(), 3 * Angle::HalfPi(), 0 }, { -Angle::HalfPi(), 0, 0 } ) );
            CHECK( getMutualOrientation( { 3 * Angle::HalfPi(), -Angle::HalfPi(), 0 }, { Angle::HalfPi(), 0, 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 3 * Angle::HalfPi(), 0 } ) );
            CHECK( getMutualOrientation( { Angle::Pi(), -Angle::HalfPi() / 2, 0 }, { 0, -Angle::HalfPi() / 2, 0 } ) );
        }
        SECTION( "Has right value" )
        {
            CHECK( getMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), -Angle::HalfPi(), 0 } ).value() == ConnectorState::WEST );
            CHECK( getMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), 3 * Angle::HalfPi(), 0 } ).value() == ConnectorState::WEST );
            CHECK( getMutualOrientation( { Angle::Pi(), -Angle::HalfPi(), 0 }, { 0, 0, 0 } ).value() == ConnectorState::WEST );
            CHECK( getMutualOrientation( { Angle::Pi(), 3 * Angle::HalfPi(), 0 }, { 0, 0, 0 } ).value() == ConnectorState::WEST );
            CHECK( getMutualOrientation( { 0, -Angle::HalfPi(), 0 }, { Angle::Pi(), 0, 0 } ).value() == ConnectorState::WEST );
            CHECK( getMutualOrientation( { Angle::HalfPi(), 3 * Angle::HalfPi(), 0 }, { -Angle::HalfPi(), 0, 0 } ).value() == ConnectorState::WEST );
            CHECK( getMutualOrientation( { 3 * Angle::HalfPi(), -Angle::HalfPi(), 0 }, { Angle::HalfPi(), 0, 0 } ).value() == ConnectorState::WEST );
            CHECK( getMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 3 * Angle::HalfPi(), 0 } ).value() == ConnectorState::WEST );
            CHECK( getMutualOrientation( { Angle::Pi(), -Angle::HalfPi() / 2, 0 }, { 0, -Angle::HalfPi() / 2, 0 } ).value() == ConnectorState::WEST );
        }
    }

    SECTION( "Nothing" )
    {
        CHECK_FALSE( getMutualOrientation( { 0, Angle::HalfPi() / 2, 0 }, { Angle::Pi(), -Angle::HalfPi(), 0 } ) );
        CHECK_FALSE( getMutualOrientation( { 0, -Angle::HalfPi() / 2, 0 }, { Angle::Pi(), 3 * Angle::HalfPi(), 0 } ) );
        CHECK_FALSE( getMutualOrientation( { Angle::Pi(), -Angle::HalfPi(), 0 }, { 0, -Angle::HalfPi() / 2, 0 } ) );
        CHECK_FALSE( getMutualOrientation( { Angle::Pi(), 3.5 * Angle::HalfPi(), 0 }, { 0, 0, 0 } ) );
        CHECK_FALSE( getMutualOrientation( { 0, -Angle::HalfPi() / 2, 0 }, { Angle::Pi(), 0, 0 } ) );
        CHECK_FALSE( getMutualOrientation( { Angle::Pi(), -Angle::HalfPi() / 2, 0 }, { 0, 3 * Angle::HalfPi(), 0 } ) );
        CHECK_FALSE( getMutualOrientation( { Angle::Pi(), Angle::HalfPi() / 2, 0 }, { 0, -Angle::HalfPi() / 2, 0 } ) );
    }
}
