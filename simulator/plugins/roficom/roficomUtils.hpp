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

enum class RoFICoMPosition : signed char
{
    Retracted = 0,
    Retracting = 1,
    Extending = 2,
    Extended = 3,
};

inline ignition::math::Angle
        getAngle( const ignition::math::Vector3d & lhs, const ignition::math::Vector3d & rhs )
{
    return { std::acos( lhs.Dot( rhs ) / ( lhs.Length() * rhs.Length() ) ) };
}

physics::LinkPtr getLinkByName( physics::ModelPtr roficom, const std::string & name );
physics::JointPtr getJointByName( physics::ModelPtr roficom, const std::string & jointName );

} // namespace gazebo
