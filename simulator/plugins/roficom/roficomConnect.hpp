#pragma once

#include <optional>

#include <ignition/math/Pose3.hh>

#include <connectorResp.pb.h>


namespace detail
{

bool checkCenterDistance( const ignition::math::Vector3d & lhs,
                          const ignition::math::Vector3d & rhs );
bool checkShift( const ignition::math::Pose3d & lhs,
                 const ignition::math::Pose3d & rhs );
bool checkTilt( const ignition::math::Quaterniond & lhs,
                const ignition::math::Quaterniond & rhs );
std::optional< rofi::messages::ConnectorState::Orientation >
        getMutualOrientation( const ignition::math::Quaterniond & lhs,
                              const ignition::math::Quaterniond & rhs );

} // namespace detail


/**
 * @brief Check if two roficoms are in a position to be connected
 * and returns the connection orientation
 * 
 * @params
 *      lhs, rhs - poses of the roficoms
 *                 vector (0, 0, 1) is the mating side direction
 *                 vector (1, 0, 0) is the North direction
 * @returns
 *      nullptr - if the connection cannot be established
 *      otherwise - returns the connection orientation
 */
std::optional< rofi::messages::ConnectorState::Orientation >
        canBeConnected( const ignition::math::Pose3d & lhs,
                        const ignition::math::Pose3d & rhs );
