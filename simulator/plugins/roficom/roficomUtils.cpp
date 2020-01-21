#include "roficomUtils.hpp"


namespace gazebo
{

ignition::math::Quaterniond getThisToOtherRotation( rofi::messages::ConnectorState::Orientation orientation )
{
    using namespace ignition::math;
    using rofi::messages::ConnectorState;

    // TODO check connect orientation ( east / west )
    switch ( orientation )
    {
        case ConnectorState::NORTH:
            return Quaterniond::Identity;
        case ConnectorState::EAST:
            return Quaterniond( { 0, 0, 1 }, -Angle::HalfPi.Radian() );
        case ConnectorState::SOUTH:
            return Quaterniond( { 0, 0, 1 }, Angle::Pi.Radian() );
        case ConnectorState::WEST:
            return Quaterniond( { 0, 0, 1 }, Angle::HalfPi.Radian() );
        default:
            throw std::runtime_error( "Orientation out of range" );
    }

    assert( false && "unexpected connector state value" );
    return Quaterniond::Identity;
}

std::optional< rofi::messages::ConnectorState::Orientation >
        getConnectorOrientation( const ignition::math::Pose3d & lhs, const ignition::math::Pose3d & rhs )
{
    using namespace ignition::math;
    using rofi::messages::ConnectorState;
    Angle maxOrientationAngle;
    maxOrientationAngle.Degree( 20 );


    auto lhsRotationVector = lhs.Rot().RotateVector( { 1, 0, 0 } );
    auto rhsRotationVector = rhs.Rot().RotateVector( { 1, 0, 0 } );

    for ( auto orientation : { ConnectorState::NORTH, ConnectorState::EAST,
                               ConnectorState::SOUTH, ConnectorState::WEST } )
    {
        auto orientationRotation = getThisToOtherRotation( orientation );
        auto rotationAngle = getAngle( orientationRotation.RotateVector( lhsRotationVector ), rhsRotationVector );
        if ( rotationAngle < maxOrientationAngle )
        {
            return orientation;
        }
    }

    return {};
}

std::optional< rofi::messages::ConnectorState::Orientation >
        canRoficomBeConnected( const ignition::math::Pose3d & lhs, const ignition::math::Pose3d & rhs )
{
    using namespace ignition::math;

    constexpr double maxConnectionCenterDistance = 0.004;
    Angle maxConnectionAngle;
    maxConnectionAngle.Degree( 4 );


    if ( lhs.Pos().Distance( rhs.Pos() ) > maxConnectionCenterDistance )
    {
        return {};
    }


    auto lhsAxis = lhs.Rot().RotateVector( { 0, 0, 1 } );
    auto rhsAxis = rhs.Rot().RotateVector( { 0, 0, 1 } );

    auto axisAngle = getAngle( lhsAxis, rhsAxis );
    if ( axisAngle < Angle::Pi - maxConnectionAngle )
    {
        return {};
    }

    return getConnectorOrientation( lhs, rhs );
}

} // namespace gazebo
