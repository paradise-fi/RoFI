#pragma once

#include <armadillo>

#include <configuration/rofiworld.hpp>
#include <configuration/universalModule.hpp>
#include <functional>
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include "utils.hpp"
#include "joint.hpp"

/**
 * Compute roficom position as vec3 based on its Component position
 * @param roficom
 * @return
 */
arma::vec3 getRoficomPosition(const rofi::configuration::Component& roficom);

/**
 * Get position of components transformed to vec3
 * @param component
 * @return
 */
arma::vec3 getPosition(const rofi::configuration::Component& component);

/**
 * Extract first three components of vector and returns them as vec3
 * @param vector
 * @return
 */
arma::vec3 toVec3(const arma::vec& vector);

/**
 * Delete allocated Joints from memory.
 * @param joints
 */
void releaseJoints(const std::unordered_map<int, rofi::torqueComputation::Joint*>& joints);

/**
 * Update joints based on TorqueConfig - e.g. add custom forces and walls.
 * @tparam Tuple
 * @tparam JointHandler
 * @param source
 * @param roficomMap
 * @param joints
 * @param jointHandler
 */
template <typename Tuple, typename JointHandler>
void updateJoints(
    const std::vector<Tuple>& source,
    const std::unordered_map<std::pair<int, int>, int, IntPairHash>& roficomMap,
    const std::unordered_map<int, rofi::torqueComputation::Joint*>& joints,
    const JointHandler& jointHandler
) {
    for (const auto& tuple : source) {
        int jointId;
        int moduleId = std::get<0>(tuple);
        int componentId = std::get<1>(tuple);

        auto roficomMapIt = roficomMap.find(std::make_pair(moduleId, componentId));
        if (roficomMapIt != roficomMap.end()) {
            jointId = roficomMapIt->second;
        }
        else {
            jointId = getJointId(moduleId, componentId);
        }
        try {
            jointHandler(tuple, joints.at(jointId));
            //joints.at(jointId)->setIsWall();
        }
        catch (std::out_of_range& e) {
            std::string msg = "TorqueConfig error. Invalid joint ID for wall: (" + std::to_string(moduleId) + ", " + std::to_string(componentId) + ")" + 
                                "\nError: " + e.what();
            releaseJoints(joints);
            throw std::logic_error(msg);
        }
    }
}

/**
 * Create Joints where modules are jointed.
 * Creates map such that this joint can be addressed from both modules.
 * @param world
 * @param joints
 * @return
 */
std::unordered_map<std::pair<int, int>, int, IntPairHash> traverseRoficomConnections(
    const rofi::configuration::RofiWorld& world, 
    std::unordered_map<int, rofi::torqueComputation::Joint*>& joints
);