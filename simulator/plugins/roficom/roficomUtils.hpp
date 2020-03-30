#pragma once

#include <gazebo/gazebo.hh>
#include <gazebo/common/Events.hh>
#include <gazebo/physics/physics.hh>

#include <connectorCmd.pb.h>
#include <connectorResp.pb.h>

#include <string>
#include <utility>
#include <optional>
#include <vector>

#include "utils.hpp"


namespace gazebo
{

inline ignition::math::Angle
        getAngle( const ignition::math::Vector3d & lhs, const ignition::math::Vector3d & rhs )
{
    return { std::acos( lhs.Dot( rhs ) / ( lhs.Length() * rhs.Length() ) ) };
}

ignition::math::Quaterniond getThisToOtherRotation( rofi::messages::ConnectorState::Orientation orientation );
std::optional< rofi::messages::ConnectorState::Orientation >
        getConnectorOrientation( const ignition::math::Pose3d & lhs, const ignition::math::Pose3d & rhs );

} // namespace gazebo
