#include "traverseRofiWorld.hpp"
#include <torqueComputation/compute.hpp>
#include <configuration/joints.hpp>

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
    const rofi::configuration::RofiWorld& world, 
    std::unordered_map<int, std::unique_ptr<Joint>>& joints
) {
        std::unordered_map<std::pair<int, int>, int, IntPairHash> result;
        for (const auto& roficom : world.roficomConnections()) {

            const auto& fromModule = roficom.getSourceModule(world);
            int fromCompId = roficom.sourceConnector;

            const auto& toModule = roficom.getDestModule(world);
            int toCompId = roficom.destConnector;

            int jointId = getJointId(fromModule.getId(), fromCompId);

            result[std::make_pair(fromModule.getId(), fromCompId)] = jointId;
            result[std::make_pair(toModule.getId(), toCompId)] = jointId;

            auto* roficomJoint = new Joint(
                jointId,
                getRoficomPosition(fromModule.components()[fromCompId]),
                false
            );
            joints.emplace(
                jointId, 
                std::unique_ptr<Joint>(roficomJoint)
            );
        }      

        return result;  
}

void VisitPadModuleStrategy::createJoints(
        const rofi::configuration::Module& rofiModule,
        const std::unordered_map<std::pair<int, int>, int, IntPairHash>& roficomMap,
        std::unordered_map<int, std::unique_ptr<Joint>>& joints,
        VariableManager&,
        const arma::vec& gravitation
) const {
    int moduleId = rofiModule.getId();
    for (const auto& padComponent : rofiModule.components()) {
        auto padJointIdx = roficomMap.find(std::make_pair(moduleId, padComponent.getIndexInParent()));

        if (padJointIdx != roficomMap.end()) {
            //Roficom already created during traversing roficom connections
            auto* padJoint = joints.at(padJointIdx->second).get();
            padJoint->appendForce(getConfiguration().weights[ComponentTypeIndex::Roficom] * gravitation);
            padJoint->setIsWall();
        }
    }
}

class UniversalModuleJointVisitor : public rofi::configuration::JointVisitor {
public:
    UniversalModuleJointVisitor(
        const rofi::configuration::Module& rofiModule,
        const std::unordered_map<std::pair<int, int>, int, IntPairHash>& roficomMap,
        std::unordered_map<int, std::unique_ptr<Joint>>& joints,
        VariableManager& variableManager,
        const arma::vec& gravitation,
        const ComponentConfiguration& configuration
    ) :
            _rofiModule(rofiModule),
            _roficomMap(roficomMap),
            _joints(joints),
            _variableManager(variableManager),
            _gravitation(gravitation),
            _configuration(configuration) {
                const auto& shoeA = rofiModule.components()[6];
                const auto& shoeB = rofiModule.components()[9];

                _delta = (getPosition(shoeB) - getPosition(shoeA)) / 3;
                _middle = (getPosition(shoeB) + getPosition(shoeA)) / 2;
                _direction = 1;
            }

    void setProcessedComponentIds(const rofi::configuration::ComponentJoint& cmpJoint) {
        _sourceComponentId = cmpJoint.sourceComponent;
        _destinationComponentId = cmpJoint.destinationComponent;
    }

    void operator()(rofi::configuration::RotationJoint& rotationJoint) override {
        int moduleId = _rofiModule.getId();
        int jointIdFrom = getJointId(moduleId, _sourceComponentId);
        int jointIdTo = getJointId(moduleId, _destinationComponentId);

        const auto& shoe = _rofiModule.components()[_destinationComponentId];
        auto norm = toVec3(
                _rofiModule.components()[_sourceComponentId].getPosition() * 
                rotationJoint.axis()
        );
        // Compognent ID is artificially created, is unique (greater than 9)
        int jointMotorId = getJointId(moduleId, _sourceComponentId + _destinationComponentId);
        vec origin = getPosition(shoe);

        auto* bodyJoint = new Joint(
                jointIdFrom,
                origin + _direction * _delta,
                false
        );
        bodyJoint->appendForce(_configuration.weights[ComponentTypeIndex::UmBody] * _gravitation);
        _joints.emplace(jointIdFrom, std::unique_ptr<Joint>(bodyJoint));

        auto* shoeJoint = new Joint(
                jointIdTo,
                origin - _direction * _delta,
                false
        );
        shoeJoint->appendForce(_configuration.weights[ComponentTypeIndex::UmShoe] * _gravitation);
        _joints.emplace(jointIdTo, std::unique_ptr<Joint>(shoeJoint));

        auto* motorJoint = new Joint(
                jointMotorId,
                origin,
                norm
        );
        motorJoint->appendForce(_configuration.weights[ComponentTypeIndex::Motor] * _gravitation);
        _joints.emplace(jointMotorId, std::unique_ptr<Joint>(motorJoint));

        createNeighborsLinks(
                *bodyJoint,
                _configuration.bounds[ComponentTypeIndex::UmBody],
                *motorJoint,
                _configuration.bounds[ComponentTypeIndex::Motor],
                _variableManager
        );
        createNeighborsLinks(
                *shoeJoint,
                _configuration.bounds[ComponentTypeIndex::UmShoe],
                *motorJoint,
                _configuration.bounds[ComponentTypeIndex::Motor],
                _variableManager
        );

        _direction *= -1;
    }

    void operator()(rofi::configuration::ModularRotationJoint& modularRotationJoint) override {
        int moduleId = _rofiModule.getId();
        int jointIdFrom = getJointId(moduleId, _sourceComponentId);
        int jointIdTo = getJointId(moduleId, _destinationComponentId);

        // Body joints are already created
        auto* from = _joints.at(jointIdFrom).get();
        auto* to = _joints.at(jointIdTo).get();

        // Compognent ID is artificially created, is unique (greater than 9)
        int jointMotorId = getJointId(moduleId, _sourceComponentId + _destinationComponentId);
        auto norm = toVec3(
                _rofiModule.components()[_sourceComponentId].getPosition() *
                modularRotationJoint.axis()
        );

        auto* gammaMotorJoint = new Joint(
                jointMotorId,
                _middle,
                norm
        );
        gammaMotorJoint->appendForce(_configuration.weights[ComponentTypeIndex::GammaMotor] * _gravitation);
        _joints.emplace(jointMotorId, std::unique_ptr<Joint>(gammaMotorJoint));

        createNeighborsLinks(
                *from,
                _configuration.bounds[ComponentTypeIndex::UmBody],
                *gammaMotorJoint,
                _configuration.bounds[ComponentTypeIndex::GammaMotor],
                _variableManager
        );
        createNeighborsLinks(
                *to,
                _configuration.bounds[ComponentTypeIndex::UmBody],
                *gammaMotorJoint,
                _configuration.bounds[ComponentTypeIndex::GammaMotor],
                _variableManager
        );
    }

    void operator()(rofi::configuration::RigidJoint&) override {
        int moduleId = _rofiModule.getId();
        int shoeIdx = _sourceComponentId;
        int roficomIdx = _destinationComponentId;

        auto* shoeJoint = _joints.at(getJointId(moduleId, shoeIdx)).get();

        auto roficomJointIdx = _roficomMap.find(std::make_pair(moduleId, roficomIdx));
        if (roficomJointIdx != _roficomMap.end()) {
            //Roficom already created during traversing roficom connections
            auto* roficomJoint = _joints.at(roficomJointIdx->second).get();
            createNeighborsLinks(
                    *shoeJoint,
                    _configuration.bounds[ComponentTypeIndex::UmShoe],
                    *roficomJoint,
                    _configuration.bounds[ComponentTypeIndex::Roficom],
                    _variableManager
            );
        }
        else {
            vec position = getRoficomPosition(_rofiModule.components()[roficomIdx]);
            int roficomJointId = getJointId(moduleId, roficomIdx);
            auto* roficomJoint = new Joint(
                    roficomJointId,
                    position,
                    false
            );
            roficomJoint->appendForce(_configuration.weights[ComponentTypeIndex::Roficom] * _gravitation);
            _joints.emplace(roficomJointId, std::unique_ptr<Joint>(roficomJoint));

            createNeighborsLinks(
                    *shoeJoint,
                    _configuration.bounds[ComponentTypeIndex::UmShoe],
                    *roficomJoint,
                    _configuration.bounds[ComponentTypeIndex::Roficom],
                    _variableManager
            );
        }
    }

private:
    const rofi::configuration::Module& _rofiModule;
    const std::unordered_map<std::pair<int, int>, int, IntPairHash>& _roficomMap;
    std::unordered_map<int, std::unique_ptr<Joint>>& _joints;
    VariableManager& _variableManager;
    const arma::vec& _gravitation;
    const ComponentConfiguration& _configuration;

    arma::vec _delta;
    arma::vec _middle;
    int _direction;

    int _sourceComponentId = -1;
    int _destinationComponentId = -1;
};

void VisitUniversalModuleStrategy::createJoints(
        const rofi::configuration::Module& rofiModule,
        const std::unordered_map<std::pair<int, int>, int, IntPairHash>& roficomMap,
        std::unordered_map<int, std::unique_ptr<Joint>>& joints,
        VariableManager& variableManager,
        const arma::vec& gravitation
) const {
    UniversalModuleJointVisitor visitor(
        rofiModule,
        roficomMap,
        joints,
        variableManager,
        gravitation,
        getConfiguration()
    );

    for (const auto& cmpJoint : rofiModule.joints()) {
        visitor.setProcessedComponentIds(cmpJoint);
        cmpJoint.joint->accept(visitor);
    }
}