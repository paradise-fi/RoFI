#include "traverseRofiWorld.hpp"
#include <torqueComputation/compute.hpp>

using namespace rofi::torqueComputation;
using namespace arma;

vec3 getRoficomPosition(const rofi::configuration::Component& roficom) {
    vec translateOrigin{-0.5, 0, 0, 1};
    auto position = roficom.getPosition() * translateOrigin;
    return toVec3(position);
}

vec3 getPosition(const rofi::configuration::Component& component) {
    vec position = component.getPosition().col(3);
    return toVec3(position);
}

vec3 toVec3(const vec& v) {
    return vec(v.memptr(), 3);
}

void releaseJoints(const std::unordered_map<int, Joint*>& joints) {
    for(auto& [key, joint] : joints) {
        delete joint;
    }
}

void createNeighborsLinks(
        Joint& origin,
        std::optional<double> originBound,
        Joint& neighbor,
        std::optional<double> neighborBound,
        VariableManager& variableManager
) {
    origin.appendNeighbor(&neighbor);
    neighbor.appendNeighbor(&origin);
    variableManager.addEdge(origin.getId(), neighbor.getId(), originBound, neighborBound);
}

std::unordered_map<std::pair<int, int>, int, IntPairHash> traverseRoficomConnections(
    const rofi::configuration::RofiWorld& world, std::unordered_map<int, rofi::torqueComputation::Joint*>& joints) {
        std::unordered_map<std::pair<int, int>, int, IntPairHash> result;
        for (const auto& roficom : world.roficomConnections()) {

            const auto& fromModule = roficom.getSourceModule(world);
            int fromCompId = roficom.sourceConnector;

            const auto& toModule = roficom.getDestModule(world);
            int toCompId = roficom.destConnector;

            int jointId = getJointId(fromModule.getId(), fromCompId);

            result[std::make_pair(fromModule.getId(), fromCompId)] = jointId;
            result[std::make_pair(toModule.getId(), toCompId)] = jointId;

            auto roficomJoint = new Joint(
                jointId,
                getRoficomPosition(fromModule.components()[fromCompId]),
                false
            );
            joints.emplace(
                jointId, 
                roficomJoint
            );
        }      

        return result;  
}

void VisitPadModuleStrategy::createJoints(
        const rofi::configuration::Module& rofiModule,
        const std::unordered_map<std::pair<int, int>, int, IntPairHash>& roficomMap,
        std::unordered_map<int, Joint*>& joints,
        VariableManager&,
        const arma::vec& gravitation
) const {
    int moduleId = rofiModule.getId();
    for (const auto& padComponent : rofiModule.components()) {
        auto padJointIdx = roficomMap.find(std::make_pair(moduleId, padComponent.getIndexInParent()));

        if (padJointIdx != roficomMap.end()) {
            //Roficom already created during traversing roficom connections
            auto* padJoint = joints.at(padJointIdx->second);
            padJoint->appendForce(getConfiguration().weights[ComponentTypeIndex::Roficom] * gravitation);
            padJoint->setIsWall();
        }
    }
}

void VisitUniversalModuleStrategy::createJoints(
        const rofi::configuration::Module& rofiModule,
        const std::unordered_map<std::pair<int, int>, int, IntPairHash>& roficomMap,
        std::unordered_map<int, Joint*>& joints,
        VariableManager& variableManager,
        const arma::vec& gravitation
) const {
    int moduleId = rofiModule.getId();
    const auto& shoeA = rofiModule.components()[6];
    const auto& shoeB = rofiModule.components()[9];
    arma::vec delta = (getPosition(shoeB) - getPosition(shoeA)) / 3;
    int multiplier = 1;

    for (const auto& cmpJoint : rofiModule.joints()) {

        if (typeid(*cmpJoint.joint) == typeid(rofi::configuration::RotationJoint)) {
            int jointIdFrom = getJointId(moduleId, cmpJoint.sourceComponent);
            int jointIdTo = getJointId(moduleId, cmpJoint.destinationComponent);

            const auto& shoe = rofiModule.components()[cmpJoint.destinationComponent];
            auto norm = toVec3(
                    rofiModule.components()[cmpJoint.sourceComponent].getPosition() *
                    (dynamic_cast<rofi::configuration::RotationJoint*>(&(*cmpJoint.joint)))->axis()
            );
            // Component ID is artificially created, is unique (greater than 9)
            int jointMotorId = getJointId(rofiModule.getId(), cmpJoint.sourceComponent + cmpJoint.destinationComponent);
            arma::vec origin = getPosition(shoe);

            auto* bodyJoint = new Joint(
                    jointIdFrom,
                    origin + multiplier * delta,
                    false
            );
            bodyJoint->appendForce(getConfiguration().weights[ComponentTypeIndex::UmBody] * gravitation);
            joints.emplace(jointIdFrom, bodyJoint);

            auto* shoeJoint = new Joint(
                    jointIdTo,
                    origin - multiplier * delta,
                    false
            );
            shoeJoint->appendForce(getConfiguration().weights[ComponentTypeIndex::UmShoe] * gravitation);
            joints.emplace(jointIdTo, shoeJoint);

            auto* motorJoint = new Joint(
                    jointMotorId,
                    origin,
                    norm,
                    true
            );
            motorJoint->appendForce(getConfiguration().weights[ComponentTypeIndex::Motor] * gravitation);
            joints.emplace(jointMotorId, motorJoint);

            createNeighborsLinks(
                    *bodyJoint,
                    getConfiguration().bounds[ComponentTypeIndex::UmBody],
                    *motorJoint,
                    getConfiguration().bounds[ComponentTypeIndex::Motor],
                    variableManager
            );
            createNeighborsLinks(
                    *shoeJoint,
                    getConfiguration().bounds[ComponentTypeIndex::UmShoe],
                    *motorJoint,
                    getConfiguration().bounds[ComponentTypeIndex::Motor],
                    variableManager
            );

            multiplier *= -1;
        }
        else if (typeid(*cmpJoint.joint) == typeid(rofi::configuration::ModularRotationJoint)) {
            int jointIdFrom = getJointId(moduleId, cmpJoint.sourceComponent);
            int jointIdTo = getJointId(moduleId, cmpJoint.destinationComponent);

            // Body joints are already created
            auto* from = joints.at(jointIdFrom);
            auto* to = joints.at(jointIdTo);

            // Arbitraty unique component ID
            int jointMotorId = getJointId(moduleId, 20);
            auto norm = toVec3(
                    rofiModule.components()[cmpJoint.sourceComponent].getPosition() *
                    (dynamic_cast<rofi::configuration::ModularRotationJoint*>(&(*cmpJoint.joint)))->axis()
            );

            auto* gammaMotorJoint = new Joint(
                    jointMotorId,
                    (getPosition(shoeB) + getPosition(shoeA)) / 2,
                    norm,
                    true
            );
            gammaMotorJoint->appendForce(getConfiguration().weights[ComponentTypeIndex::GammaMotor] * gravitation);
            joints.emplace(jointMotorId, gammaMotorJoint);

            createNeighborsLinks(
                    *from,
                    getConfiguration().bounds[ComponentTypeIndex::UmBody],
                    *gammaMotorJoint,
                    getConfiguration().bounds[ComponentTypeIndex::GammaMotor],
                    variableManager
            );
            createNeighborsLinks(
                    *to,
                    getConfiguration().bounds[ComponentTypeIndex::UmBody],
                    *gammaMotorJoint,
                    getConfiguration().bounds[ComponentTypeIndex::GammaMotor],
                    variableManager
            );

        }
        else {
            int shoeIdx = cmpJoint.sourceComponent;
            int roficomIdx = cmpJoint.destinationComponent;

            auto* shoeJoint = joints.at(getJointId(moduleId, shoeIdx));

            auto roficomJointIdx = roficomMap.find(std::make_pair(moduleId, roficomIdx));
            if (roficomJointIdx != roficomMap.end()) {
                //Roficom already created during traversing roficom connections
                auto* roficomJoint = joints.at(roficomJointIdx->second);
                createNeighborsLinks(
                        *shoeJoint,
                        getConfiguration().bounds[ComponentTypeIndex::UmShoe],
                        *roficomJoint,
                        getConfiguration().bounds[ComponentTypeIndex::Roficom],
                        variableManager
                );
            }
            else {
                arma::vec position = getRoficomPosition(rofiModule.components()[roficomIdx]);
                int roficomJointId = getJointId(moduleId, roficomIdx);
                auto* roficomJoint = new Joint(
                        roficomJointId,
                        position,
                        false
                );
                roficomJoint->appendForce(getConfiguration().weights[ComponentTypeIndex::Roficom] * gravitation);
                joints.emplace(roficomJointId, roficomJoint);

                createNeighborsLinks(
                        *shoeJoint,
                        getConfiguration().bounds[ComponentTypeIndex::UmShoe],
                        *roficomJoint,
                        getConfiguration().bounds[ComponentTypeIndex::Roficom],
                        variableManager
                );
            }
        }

    }
}