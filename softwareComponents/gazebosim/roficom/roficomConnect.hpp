#pragma once

#include <optional>

#include <gz/math/Pose3.hh>

#include <connectorResp.pb.h>


namespace detail
{
bool checkCenterDistance( const gz::math::Vector3d & lhs, const gz::math::Vector3d & rhs );
bool checkShift( const gz::math::Pose3d & lhs, const gz::math::Pose3d & rhs );
bool checkTilt( const gz::math::Quaterniond & lhs, const gz::math::Quaterniond & rhs );
std::optional< rofi::messages::ConnectorState::Orientation > getMutualOrientation(
        const gz::math::Quaterniond & lhs,
        const gz::math::Quaterniond & rhs );

} // namespace detail


std::optional< rofi::messages::ConnectorState::Orientation > canRoficomBeConnected(
        const gz::math::Pose3d & lhs,
        const gz::math::Pose3d & rhs );
