#include "roficomConnect.hpp"

#include <cassert>
#include <cmath>

#include <ignition/math/Angle.hh>


struct RoficomConnectConfig
{
    static constexpr double maxDistance = 5e-3; // [m]
    static constexpr double maxTilt = 0.1; // [rad] ( 6 deg )
    static constexpr double maxShift = 4e-3; // [m]
    static constexpr double maxRotation = IGN_PI / 4; // [rad]


    static_assert( 4 * maxRotation <= 2 * IGN_PI );
};


namespace detail
{

bool checkMaxAngle( const ignition::math::Vector3d & lhs,
                    const ignition::math::Vector3d & rhs,
                    double maxAngle )
{
    auto cosAngle = lhs.Dot( rhs ) / ( lhs.Length() * rhs.Length() );

    // return std::acos( cosAngle ) <= maxAngle;
    return cosAngle >= std::cos( maxAngle ); // cos is decreasing function
}


bool checkCenterDistance( const ignition::math::Vector3d & lhs,
                          const ignition::math::Vector3d & rhs )
{
    return lhs.Distance( rhs ) <= RoficomConnectConfig::maxDistance;
}

bool checkOneWayShift( const ignition::math::Pose3d & lhs,
                       const ignition::math::Pose3d & rhs )
{
    auto secondPoint = rhs.Pos() + rhs.Rot().RotateVector( { 0, 0, 1 } );
    auto distToLine = ignition::math::Vector3d( lhs.Pos() ).DistToLine( rhs.Pos(), secondPoint );
    return distToLine <= RoficomConnectConfig::maxShift;
}

bool checkShift( const ignition::math::Pose3d & lhs,
                 const ignition::math::Pose3d & rhs )
{
    return checkOneWayShift( lhs, rhs ) && checkOneWayShift( rhs, lhs );
}

bool checkTilt( const ignition::math::Quaterniond & lhs,
                const ignition::math::Quaterniond & rhs )
{
    auto left = lhs.RotateVector( { 0, 0, 1 } );
    auto right = rhs.RotateVector( { 0, 0, -1 } );
    return checkMaxAngle( left, right, RoficomConnectConfig::maxTilt );
}

std::optional< rofi::messages::ConnectorState::Orientation >
        getMutualOrientation( const ignition::math::Quaterniond & lhs,
                              const ignition::math::Quaterniond & rhs )
{
    using ignition::math::Quaterniond;
    for ( int i = 0; i < 4; i++ )
    {
        auto leftVector = Quaterniond( 0, 0, i * ignition::math::Angle::HalfPi() ).RotateVector( { 1, 0, 0 } );
        auto left = lhs.RotateVector( leftVector );
        auto right = rhs.RotateVector( { 1, 0, 0 } );

        if ( checkMaxAngle( left, right, RoficomConnectConfig::maxRotation ) )
        {
            switch ( i )
            {
            case 0:
                return rofi::messages::ConnectorState::NORTH;
            case 1:
                return rofi::messages::ConnectorState::WEST;
            case 2:
                return rofi::messages::ConnectorState::SOUTH;
            case 3:
                return rofi::messages::ConnectorState::EAST;
            default:
                assert( false );
            }
        }

        return {};
    }

    return {};
}

} // namespace detail


std::optional< rofi::messages::ConnectorState::Orientation >
        canBeConnected( const ignition::math::Pose3d & lhs,
                        const ignition::math::Pose3d & rhs )
{
    if ( !detail::checkCenterDistance( lhs.Pos(), rhs.Pos() ) )
    {
        return {};
    }
    if ( !detail::checkShift( lhs, rhs ) )
    {
        return {};
    }
    if ( !detail::checkTilt( lhs.Rot(), rhs.Rot() ) )
    {
        return {};
    }
    return detail::getMutualOrientation( lhs.Rot(), rhs.Rot() );
}
