#include "stringifiers.hpp"

#define CATCH_CONFIG_FALLBACK_STRINGIFIER fallback_stringifier
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
        function( lhs, rhs * rotation );
    }
}

#define CHECK_EQUAL( lhs, rhs )                         \
        do                                              \
        {                                               \
            INFO( #lhs << ":  " << to_string( lhs ) );  \
            INFO( #rhs << ": " << to_string( rhs ) );   \
            CHECK( lhs == rhs );                        \
        } while( false )

void checkMutualOrientationImpl( const ignition::math::Quaterniond & lhs,
                             const ignition::math::Quaterniond & rhs,
                             std::optional< ConnectorState::Orientation > desiredOrientation )
{
    INFO( "lhs rotation: " << lhs );
    INFO( "rhs rotation: " << rhs );
    auto mutualOrientation = getMutualOrientation( lhs, rhs );
    CHECK_EQUAL( mutualOrientation, desiredOrientation );
}

#define checkMutualOrientation( ... )                                             \
        do                                                              \
        {                                                               \
            INFO( "line: checkMutualOrientation( " << #__VA_ARGS__ << " );" );    \
            checkMutualOrientationImpl( __VA_ARGS__ );                            \
        } while ( false )

void checkAllGoodImpl( const Pose3d & lhs, const Pose3d & rhs )
{
    INFO( "lhs: " << lhs );
    INFO( "rhs: " << rhs );

    CHECK( checkCenterDistance( lhs.Pos(), rhs.Pos() ) );
    CHECK( checkShift( lhs, rhs ) );
    CHECK( checkTilt( lhs.Rot(), rhs.Rot() ) );
    auto mutualOrientation = getMutualOrientation( lhs.Rot(), rhs.Rot() );
    CHECK( mutualOrientation );

    auto result = canRoficomBeConnected( lhs, rhs );
    REQUIRE( result );

    CHECK_EQUAL( mutualOrientation, result );
}

#define checkAllGood( ... )                                             \
        do                                                              \
        {                                                               \
            INFO( "line: checkAllGood( " << #__VA_ARGS__ << " );" );    \
            forAllOrientations( checkAllGoodImpl, __VA_ARGS__ );                            \
        } while ( false )

#define checkAllGoodAtSamePosition( ... )                                           \
        do                                                                          \
        {                                                                           \
            INFO( "line: checkAllGoodAtSamePosition( " << #__VA_ARGS__ << " );" );  \
            checkAllGoodAtSamePositionImpl( __VA_ARGS__ );                          \
        } while ( false )

void checkAllGoodAtSamePositionImpl( const Quaterniond & lhs, const Quaterniond & rhs )
{
    auto pos = Vector3d(); // TODO make random
    INFO( "position: " << pos );
    forAllOrientations( checkAllGoodImpl, { pos, lhs }, { pos, rhs } );
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
            checkAllGoodAtSamePosition( { 0, 0, 0 }, { Angle::Pi(), 0, 0} ); // ok
            checkAllGoodAtSamePosition( { Angle::Pi(), 0, 0 }, { 0, 0, 0} );
            checkAllGoodAtSamePosition( { 0, Angle::Pi(), 0 }, { 0, 0, Angle::Pi()} );
            checkAllGoodAtSamePosition( { 0, 0, Angle::Pi() }, { 0, Angle::Pi(), 0} );
            checkAllGoodAtSamePosition( { Angle::HalfPi(), 0, 0 }, { -Angle::HalfPi(), 0, 0} );
            checkAllGoodAtSamePosition( { 0, 0, Angle::HalfPi() }, { 0, Angle::Pi(), -Angle::HalfPi()} );
            checkAllGoodAtSamePosition( { Angle::Pi(), Angle::HalfPi(), 0 }, { 0, Angle::HalfPi(), 0} );
        }

        SECTION( "South" )
        {
            checkAllGoodAtSamePosition( { 0, 0, 0 }, { 0, Angle::Pi(), 0} ); // ok
            checkAllGoodAtSamePosition( { Angle::Pi(), 0, 0 }, { 0, 0, Angle::Pi()} );
            checkAllGoodAtSamePosition( { 0, Angle::Pi(), 0 }, { 0, 0, 0} );
            checkAllGoodAtSamePosition( { 0, 0, Angle::Pi() }, { Angle::Pi(), 0, 0} );
            checkAllGoodAtSamePosition( { Angle::HalfPi(), 0, 0 }, { Angle::HalfPi(), 0, Angle::Pi()} );
            checkAllGoodAtSamePosition( { 0, Angle::HalfPi(), 0 }, { 0, -Angle::HalfPi(), 0} );
            checkAllGoodAtSamePosition( { 0, 0, Angle::HalfPi() }, { 0, Angle::Pi(), Angle::HalfPi()} );
        }

        SECTION( "East" )
        {
            checkAllGoodAtSamePosition( { Angle::Pi(), 0, 0 }, { 0, 0, Angle::HalfPi()} );
            checkAllGoodAtSamePosition( { 0, 0, 0 }, { Angle::Pi(), 0, -Angle::HalfPi()} );
            checkAllGoodAtSamePosition( { 0, Angle::Pi(), 0 }, { 0, 0, -Angle::HalfPi()} );
            checkAllGoodAtSamePosition( { 0, 0, Angle::Pi() }, { Angle::Pi(), 0, Angle::HalfPi()} );
            checkAllGoodAtSamePosition( { Angle::HalfPi(), 0, 0 }, { 0, Angle::HalfPi(), Angle::HalfPi()} );
            checkAllGoodAtSamePosition( { 0, Angle::HalfPi(), 0 }, { -Angle::HalfPi(), Angle::Pi(), Angle::HalfPi() } );
            checkAllGoodAtSamePosition( { 0, 0, Angle::HalfPi() }, { 0, Angle::Pi(), Angle::Pi() } );
        }

        SECTION( "West" )
        {
            checkAllGoodAtSamePosition( { Angle::Pi(), 0, 0 }, { 0, 0, -Angle::HalfPi()} ); // ok
            checkAllGoodAtSamePosition( { 0, 0, 0 }, { Angle::Pi(), 0, Angle::HalfPi()} ); // west
            checkAllGoodAtSamePosition( { 0, Angle::Pi(), 0 }, { 0, 0, Angle::HalfPi()} ); // ok
            checkAllGoodAtSamePosition( { 0, 0, Angle::Pi() }, { Angle::Pi(), 0, -Angle::HalfPi()} ); // west
            checkAllGoodAtSamePosition( { Angle::HalfPi(), 0, 0 }, { 0, -Angle::HalfPi(), -Angle::HalfPi()} ); // west
            checkAllGoodAtSamePosition( { 0, Angle::HalfPi(), 0 }, { Angle::HalfPi(), Angle::Pi(), -Angle::HalfPi() } );
            checkAllGoodAtSamePosition( { 0, 0, Angle::HalfPi() }, { 0, Angle::Pi(), 0 } );
        }
    }
}

TEST_CASE( "Check shift" )
{
    CHECK( checkShift( { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, Angle::Pi(), 0 } ) );
    CHECK( checkShift( { 234.003, -50, 10, Angle::Pi(), 0, 0 }, { 234, -50, 10, 0, 0, 0 } ) );
    CHECK( checkShift( { 0.001, 0.001, 0.001, Angle::Pi(), 0, 0 }, { 0, 0, 0, 0, 0, 0 } ) );
    CHECK( checkShift( { 0.002, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, Angle::Pi(), 0 } ) );
    CHECK( checkShift( { 0, 0.002, 0, 0, Angle::Pi(), 0 }, { 0, 0, 0, 0, 0, 0 } ) );

    CHECK_FALSE( checkShift( { 0, 0.008, 0, 0, 0, 0 }, { 0, 0, 0, Angle::Pi(), 0, 0 } ) );
    CHECK_FALSE( checkShift( { 0, 0, 10, 0, 0, 0 }, { 0, 0.02, 10, Angle::Pi(), 0, 0 } ) );
    CHECK_FALSE( checkShift( { 10, 0.04, 0, 0, 0, 0 }, { 10, 0, 0, Angle::Pi(), 0, 0 } ) );
    CHECK_FALSE( checkShift( { 0, 0, 10, 0, 0, 0 }, { 0.01, 0, 10, Angle::Pi(), 0, 0 } ) );
    CHECK_FALSE( checkShift( { 10, 0.005, 0, 0, 0, 0 }, { 10.003, 0, 0.005, Angle::Pi(), 0, 0 } ) );
}

TEST_CASE( "Check tilt" )
{
    REQUIRE( ( false && "TODO add tests" ) );
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
        checkMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), 0, 0}, ConnectorState::NORTH ); // ok
        checkMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 0, 0}, ConnectorState::NORTH );
        checkMutualOrientation( { 0, Angle::Pi(), 0 }, { 0, 0, Angle::Pi()}, ConnectorState::NORTH );
        checkMutualOrientation( { 0, 0, Angle::Pi() }, { 0, Angle::Pi(), 0}, ConnectorState::NORTH );
        checkMutualOrientation( { Angle::HalfPi(), 0, 0 }, { -Angle::HalfPi(), 0, 0}, ConnectorState::NORTH );
        checkMutualOrientation( { 0, 0, Angle::HalfPi() }, { 0, Angle::Pi(), -Angle::HalfPi()}, ConnectorState::NORTH );
        checkMutualOrientation( { Angle::Pi(), Angle::HalfPi(), 0 }, { 0, Angle::HalfPi(), 0}, ConnectorState::NORTH );
    }

    SECTION( "South" )
    {
        checkMutualOrientation( { 0, 0, 0 }, { 0, Angle::Pi(), 0}, ConnectorState::SOUTH ); // ok
        checkMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 0, Angle::Pi()}, ConnectorState::SOUTH );
        checkMutualOrientation( { 0, Angle::Pi(), 0 }, { 0, 0, 0}, ConnectorState::SOUTH );
        checkMutualOrientation( { 0, 0, Angle::Pi() }, { Angle::Pi(), 0, 0}, ConnectorState::SOUTH );
        checkMutualOrientation( { Angle::HalfPi(), 0, 0 }, { Angle::HalfPi(), 0, Angle::Pi()}, ConnectorState::SOUTH );
        checkMutualOrientation( { 0, Angle::HalfPi(), 0 }, { 0, -Angle::HalfPi(), 0}, ConnectorState::SOUTH );
        checkMutualOrientation( { 0, 0, Angle::HalfPi() }, { 0, Angle::Pi(), Angle::HalfPi()}, ConnectorState::SOUTH );
    }

    SECTION( "East" )
    {
        checkMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 0, Angle::HalfPi()}, ConnectorState::EAST );
        checkMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), 0, -Angle::HalfPi()}, ConnectorState::EAST );
        checkMutualOrientation( { 0, Angle::Pi(), 0 }, { 0, 0, -Angle::HalfPi()}, ConnectorState::EAST );
        checkMutualOrientation( { 0, 0, Angle::Pi() }, { Angle::Pi(), 0, Angle::HalfPi()}, ConnectorState::EAST );
        checkMutualOrientation( { Angle::HalfPi(), 0, 0 }, { 0, Angle::HalfPi(), Angle::HalfPi()}, ConnectorState::EAST );
        checkMutualOrientation( { 0, Angle::HalfPi(), 0 }, { -Angle::HalfPi(), Angle::Pi(), Angle::HalfPi() }, ConnectorState::EAST );
        checkMutualOrientation( { 0, 0, Angle::HalfPi() }, { 0, Angle::Pi(), Angle::Pi() }, ConnectorState::EAST );
    }

    SECTION( "West" )
    {
        checkMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 0, -Angle::HalfPi()}, ConnectorState::WEST ); // ok
        checkMutualOrientation( { 0, 0, 0 }, { Angle::Pi(), 0, Angle::HalfPi()}, ConnectorState::WEST ); // west
        checkMutualOrientation( { 0, Angle::Pi(), 0 }, { 0, 0, Angle::HalfPi()}, ConnectorState::WEST ); // ok
        checkMutualOrientation( { 0, 0, Angle::Pi() }, { Angle::Pi(), 0, -Angle::HalfPi()}, ConnectorState::WEST ); // west
        checkMutualOrientation( { Angle::HalfPi(), 0, 0 }, { 0, -Angle::HalfPi(), -Angle::HalfPi()}, ConnectorState::WEST ); // west
        checkMutualOrientation( { 0, Angle::HalfPi(), 0 }, { Angle::HalfPi(), Angle::Pi(), -Angle::HalfPi() }, ConnectorState::WEST );
        checkMutualOrientation( { 0, 0, Angle::HalfPi() }, { 0, Angle::Pi(), 0 }, ConnectorState::WEST );
    }

    SECTION( "Nothing" )
    {
        checkMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 0, Angle::HalfPi() / 2 }, std::nullopt );
        checkMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 0, Angle::HalfPi() + Angle::HalfPi() / 2 }, std::nullopt );
        checkMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 0, -Angle::HalfPi() - Angle::HalfPi() / 2 }, std::nullopt );
        checkMutualOrientation( { Angle::Pi(), 0, 0 }, { 0, 0, -Angle::HalfPi() / 2 }, std::nullopt );
        checkMutualOrientation( { 0, Angle::Pi(), 0, 0 }, { 0, 0, Angle::HalfPi() / 2 }, std::nullopt );
        checkMutualOrientation( { 0, Angle::Pi(), 0, 0 }, { 0, 0, Angle::HalfPi() + Angle::HalfPi() / 2 }, std::nullopt );
        checkMutualOrientation( { 0, Angle::Pi(), 0, 0 }, { 0, 0, -Angle::HalfPi() - Angle::HalfPi() / 2 }, std::nullopt );
        checkMutualOrientation( { 0, Angle::Pi(), 0, 0 }, { 0, 0, -Angle::HalfPi() / 2 }, std::nullopt );
        checkMutualOrientation( { Angle::HalfPi(), Angle::HalfPi() / 2, -Angle::HalfPi() }, { -Angle::HalfPi(), 0, -Angle::HalfPi() }, std::nullopt );
    }
}
