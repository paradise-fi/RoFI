#pragma once

#include <optional>

#include <ignition/math/Pose3.hh>

#include <connectorResp.pb.h>


namespace detail
{
bool checkCenterDistance( const ignition::math::Vector3d & lhs,
                          const ignition::math::Vector3d & rhs );
bool checkShift( const ignition::math::Pose3d & lhs, const ignition::math::Pose3d & rhs );
bool checkTilt( const ignition::math::Quaterniond & lhs, const ignition::math::Quaterniond & rhs );
std::optional< rofi::messages::ConnectorState::Orientation > getMutualOrientation(
        const ignition::math::Quaterniond & lhs,
        const ignition::math::Quaterniond & rhs );

} // namespace detail


std::optional< rofi::messages::ConnectorState::Orientation > canRoficomBeConnected(
        const ignition::math::Pose3d & lhs,
        const ignition::math::Pose3d & rhs );
