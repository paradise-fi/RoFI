#include <torqueComputation/compute.hpp>
#include <cassert>
#include <cmath>
#include "traverseRofiWorld.hpp"

using namespace arma;
using namespace rofi::configuration;
using namespace rofi::torqueComputation;
using namespace nlohmann;


namespace rofi::torqueComputation {
    void sortPairAscending(std::pair<int, int>& p) {
        if (p.first > p.second) {
            std::swap(p.first, p.second);
        }
    }

    std::string moduleTypeToString(rofi::configuration::ModuleType moduleType) {
        switch (moduleType) {
            case ModuleType::Unknown:
                return "unknown";
            case ModuleType::Universal:
                return "universal";
            case ModuleType::Pad:
                return "pad";
            case ModuleType::Cube:
                return "cube";
            default:
                throw std::out_of_range("Invalid module type");
        }
    }

    rofi::configuration::ModuleType moduleTypeFromString(const std::string& str) {
        if (str == "unknown") {
            return rofi::configuration::ModuleType::Unknown;
        } else if (str == "universal") {
            return rofi::configuration::ModuleType::Universal;
        } else if (str == "pad") {
            return rofi::configuration::ModuleType::Pad;
        } else if (str == "cube") {
            return rofi::configuration::ModuleType::Cube;
        } else {
            throw std::invalid_argument("Invalid module type string: " + str);
        }
    }

    std::string componentTypeToString(ComponentTypeIndex componentType) {
        switch (componentType) {
            case ComponentTypeIndex::Roficom:
                return "Roficom";
            case ComponentTypeIndex::UmBody:
                return "UmBody";
            case ComponentTypeIndex::UmShoe:
                return "UmShoe";
            case ComponentTypeIndex::CubeBody:
                return "CubeBody";
            case ComponentTypeIndex::GammaMotor:
                return "GammaMotor";
            case ComponentTypeIndex::Motor:
                return "Motor";
            default:
                throw std::invalid_argument("Invalid component type");
        }
    }

    ComponentTypeIndex componentTypeFromString(const std::string& str) {
        if (str == "Roficom") {
            return ComponentTypeIndex::Roficom;
        } else if (str == "UmBody") {
            return ComponentTypeIndex::UmBody;
        } else if (str == "UmShoe") {
            return ComponentTypeIndex::UmShoe;
        } else if (str == "CubeBody") {
            return ComponentTypeIndex::CubeBody;
        } else if (str == "GammaMotor") {
            return ComponentTypeIndex::GammaMotor;
        } else if (str == "Motor") {
            return ComponentTypeIndex::Motor;
        } else {
            throw std::out_of_range("Invalid component type string: " + str);
        }
    }

    std::string vecToString(const vec3& v) {
        std::stringstream stream;
        stream << "[";

        for (unsigned int i = 0; i < v.n_elem; i++) {
            if (i > 0) {
                stream << ", ";
            }
            stream << std::round(v(i) * 1000) / 1000;
        }

        stream << "]";
        return stream.str();
    }

    std::string vecToString(const std::vector<int>& v) {
        std::stringstream stream;
        stream << "[";

        for (unsigned int i = 0; i < v.size(); i++) {
            if (i > 0) {
                stream << ", ";
            }
            stream << v[i];
        }

        stream << "]";
        return stream.str();
    }

    std::string momentsToString(const mat33& moments) {
        std::stringstream stream;
        for (unsigned int row = 0; row < 3; row++) {
            stream << "            [";
            for (unsigned int col = 0; col < 3; col++) {
                if (col > 0) {
                    stream << ", ";
                }
                stream << moments(row, col);
            }
            stream << "]";
            if (row < 2) {
                stream << ",";
            }
            stream << std::endl;
        }
        return stream.str();
    }

    void printJointsInfo(const std::unordered_map<int, Joint*>& joints) {
        std::cout << "{\"joints\": {" << std::endl;

        for (const auto [jointId, joint] : joints) {
            vec3 forces_sum = vec::fixed<3>(fill::zeros);
            for (const auto& force : joint->getForces()) {
                forces_sum += force;
            }
            auto norm = joint->getNorm();

            std::cout << "    \"" << jointId << "\" : {\n";
            std::cout << "        \"id\": " << jointId << ",\n";
            std::cout << "        \"coors\": " << vecToString(joint->getCoors()) << ",\n";
            if (norm) {
            std::cout << "        \"norm\": " << vecToString(norm.value()) <<",\n";
            }
            std::cout << "        \"is_wall\": " << (joint->getIsWall() ? "true" : "false") << ",\n";
            std::cout << "        \"is_bounded\": " << (joint->getIsBounded() ? "true" : "false") << ",\n";
            std::cout << "        \"neighbors\": " << vecToString(joint->getCanonicNeighborsIds()) <<",\n";
            std::cout << "        \"forces\": " << vecToString(forces_sum) <<",\n";
            std::cout << "        \"moments\": [\n";
            std::cout << momentsToString(joint->getMoments());
            std::cout << "        ]\n";
            std::cout << "    }," << std::endl;
        }
        std::cout << "}}" << std::endl;
    }

    void printMatrixInfo(
        const mat& constraintMatrix, 
        const vec& constraintBounds,
        const VariableManager& variableManager
    ) {
        std::cout << std::endl << "Dimension: " << constraintMatrix.n_rows << " x " << constraintMatrix.n_cols << std::endl << std::endl;

        for (const auto& varName : variableManager.getVariableNames()) {
            std::cout << varName << ";";
        }
        std::cout << std::endl << std::endl;
        for (unsigned int row = 0; row < constraintMatrix.n_rows; row++) {
            for (unsigned int col = 0; col < constraintMatrix.n_cols; col++) {
                std::cout << std::round(100000 * constraintMatrix(row, col)) / 100000 << ";";
            }
            std::cout << " = " << constraintBounds(row) << std::endl;
        }
        std::cout << std::endl << std::endl;
        std::cout << "Bound lower:" << std::endl;
        for (const auto& lb : variableManager.getVariablesLowerBounds()) {
            std::cout << lb << ";";
        }
        std::cout << std::endl << std::endl;
        std::cout << "Bound upper:" << std::endl;
        for (const auto& ub : variableManager.getVariablesUpperBounds()) {
            std::cout << ub << ";";
        }
        std::cout << std::endl << std::endl;
    }

    int visitJointForEquation(
        int rowIndex,
        const Joint& joint, 
        const VariableManager& variableManager,
        mat& constraintMatrix, 
        vec& constraintBounds
    ) {
        

        std::optional<arma::mat> forceLhs = joint.createForceLhs(variableManager);
        if (forceLhs != std::nullopt) {

            constraintMatrix.rows(rowIndex, rowIndex + 2) = forceLhs.value();

            vec3 forces_sum = vec::fixed<3>(fill::zeros);
            for (const auto& force : joint.getForces()) {
                forces_sum += force;
            }
            constraintBounds.subvec(rowIndex, rowIndex + 2) = -forces_sum;
            rowIndex += 3;
        }

        std::optional<arma::mat> momentLhs = joint.createMomentLhs(variableManager);
        if (momentLhs != std::nullopt) {
            constraintMatrix.rows(rowIndex, rowIndex + 2) = momentLhs.value();
            constraintBounds.subvec(rowIndex, rowIndex + 2) = vec::fixed<3>(fill::zeros);
            rowIndex += 3;
        }
        return rowIndex;
    }

    std::pair<uint, int> visitJointsForEquations(
        int rowIndex,
        const std::unordered_map<int, Joint*>& joints,
        const VariableManager& variableManager,
        mat& constraintMatrix,
        vec& constraintBounds
    ) {
        std::set<std::pair<int, int>> edgesVisited;

        uint jointsVisitedCount = 0;
        int edgesVisitedCount = 0;

        for (const auto& [jointId, joint] : joints) {
            rowIndex = visitJointForEquation(rowIndex, *joint, variableManager, constraintMatrix, constraintBounds);
            jointsVisitedCount++;

            for (const auto& neighbor : joint->getNeighbors()) {
                auto edge = std::make_pair(jointId, neighbor->getId());
                sortPairAscending(edge);
                
                if (edgesVisited.find(edge) == edgesVisited.end()) {
                    mat edgeMomentLhs = joint->createMomentEdgeLhs(*neighbor, variableManager);
                    constraintMatrix.rows(rowIndex, rowIndex + 2) = edgeMomentLhs;
                    constraintBounds.subvec(rowIndex, rowIndex + 2) = vec::fixed<3>(fill::zeros);

                    rowIndex += 3;
                    edgesVisitedCount++;
                    edgesVisited.insert(edge);
                }
            }
        }
        return std::make_pair(jointsVisitedCount, edgesVisitedCount);
    }

    OptimizeResult computeTorque(
        const RofiWorld& rofiWorld, 
        const TorqueConfig& config, 
        SolveTo solveTo /*= SolveTo::Solve*/,
        bool printJoints /* = false*/,
        bool printMatrix /* = false*/
    ) {
        std::unordered_map<int, Joint*> joints;

        auto roficomMap = traverseRoficomConnections(rofiWorld, joints);
        VariableManager variableManager;

        for (const auto& module : rofiWorld.modules()) {
            try {
                config.visitModuleStrategies.at(moduleTypeToString(module.type))->createJoints(
                    module,
                    roficomMap,
                    joints,
                    variableManager,
                    config.gravitation
                );
            }
            catch (std::out_of_range& e) {
                std::string msg = "TorqueConfig error. Missing strategy for: (" + moduleTypeToString(module.type) + 
                                  "\nError: " + e.what();
                throw std::logic_error(msg);
            }
        }

        if (joints.size() == 0) {
            return {true, 0, {}};
        }

        updateJoints(config.forces, roficomMap, joints, [](const std::tuple<int, int, arma::vec>& tuple, rofi::torqueComputation::Joint* joint) { 
            joint->appendForce(std::get<2>(tuple));
        });
        updateJoints(config.walls, roficomMap, joints, [](const std::tuple<int, int>&, rofi::torqueComputation::Joint* joint) {
            joint->setIsWall();
        });

        if (printJoints) {
            printJointsInfo(joints);
        }
        
        if (solveTo == SolveTo::JointsCreation) {
            releaseJoints(joints);
            return {false, INFINITY, {}};
        }

        int variableCount = variableManager.getVariablesCount();
        size_t mostRowsCount = variableManager.getEdgesCount() * 3 + joints.size() * 6;
        mat constraintMatrix(mostRowsCount, variableCount, fill::zeros);
        vec constraintBounds(mostRowsCount, fill::zeros);

        auto [jointsVisited, edgesVisited] = visitJointsForEquations(0, joints, variableManager, constraintMatrix, constraintBounds);

        assert(jointsVisited == (joints.size()) && "Not all joints were visited!!!");
        assert(edgesVisited == variableManager.getEdgesCount() && "Not all edges were visited!!!");

        releaseJoints(joints);

        if (printMatrix) {
            printMatrixInfo(constraintMatrix, constraintBounds, variableManager);
        }

        if (solveTo == SolveTo::MatrixComposition) {
            return {false, INFINITY, {}};
        }

        vec constraintBoundsLower = constraintBounds - 10e-3;


        OptimizeResult result = linprog(
            constraintMatrix,
            constraintBoundsLower,
            constraintBounds,
            variableManager.getVariablesLowerBounds(),
            variableManager.getVariablesUpperBounds()
        );

        return result;
    }

    nlohmann::json visitModuleStrategyToJSON(const VisitModuleStrategy& strategy) {
        const auto& config = strategy.getConfiguration();
        json res = json::object();
        for (size_t i = 0; i < config.weights.size(); i++) {
            auto key = componentTypeToString(static_cast<ComponentTypeIndex>(i));
            if (0 < config.weights[i]) {
                res["weights"][key] = config.weights[i];
            }
            if (config.bounds[i].has_value()) {
                res["bounds"][key] = config.bounds[i].value();
            }
        }
        return res;
    }

    VisitModuleStrategy* jsonToVisitModuleStrategy(rofi::configuration::ModuleType moduleType, const json& j)
    {
        ComponentConfiguration config;

        config.weights.resize(ComponentTypeIndex::COMPONENT_TYPE_LENGTH, 0);
        if (j.contains("weights")) {
            auto jWeights = j.at("weights");
            for (auto it = jWeights.begin(); it != jWeights.end(); ++it) {
                int i = static_cast<int>(componentTypeFromString(it.key()));
                double weight = it.value().get<double>();
                config.weights[i] = weight;
            }
        }

        config.bounds.resize(ComponentTypeIndex::COMPONENT_TYPE_LENGTH);
        if (j.contains("bounds")) {
            auto jBounds = j.at("bounds");
            for (auto it = jBounds.begin(); it != jBounds.end(); ++it) {
                int i = static_cast<int>(componentTypeFromString(it.key()));
                double bound = it.value().get<double>();
                config.bounds[i] = bound;
            }
        }
        
        switch(moduleType) {
            case rofi::configuration::ModuleType::Universal:
                return new VisitUniversalModuleStrategy(config);
            case rofi::configuration::ModuleType::Pad:
                return new VisitPadModuleStrategy(config);
            default:
                std::string msg = "TorqueConfig error. VisitModuleStrategy for module type {";
                msg.append(std::to_string(static_cast<int>(moduleType)));
                msg.append("} must be programmed and added to parser.");
                throw std::logic_error(msg);
        }
    }

    nlohmann::json toJSON(const TorqueConfig& config) {
        json res;
        res["gravitation"] = config.gravitation;

        for (const auto& [moduleType, strategy] : config.visitModuleStrategies) {
            res["visitModuleStrategies"][moduleType] = visitModuleStrategyToJSON(*strategy);
        }

        for (const auto& [moduleId, componentId, force] : config.forces) {
            json jForce;
            jForce["moduleId"] = moduleId;
            jForce["componentId"] = componentId;
            jForce["force"] = force;
            res["forces"].push_back(jForce);
        }

        for (const auto& [moduleId, componentId] : config.walls) {
            json jWall;
            jWall["moduleId"] = moduleId;
            jWall["componentId"] = componentId;
            res["walls"].push_back(jWall);
        }

        return res;
    }

    TorqueConfig fromJSON(const nlohmann::json& j) {
        TorqueConfig config;
        if (!j.contains("gravitation") ) {
            throw std::logic_error( "Cannot find gravitation in config json" );
        }
        if (!j.contains("visitModuleStrategies") ) {
            throw std::logic_error( "Cannot find visitModuleStrategies in config json" );
        }

        config.gravitation = vec(j.at("gravitation").get<std::vector<double>>());

        for (auto it = j["visitModuleStrategies"].begin(); it != j["visitModuleStrategies"].end(); ++it) {
            auto moduleType = it.key();
            auto strategy = jsonToVisitModuleStrategy(moduleTypeFromString(moduleType), it.value());
            config.visitModuleStrategies.emplace(moduleType, strategy);
        }

        if (j.contains("forces")) {
            for (const auto& jForce : j["forces"]) {
                int moduleId = jForce.at("moduleId").get<int>();
                int componentId = jForce.at("componentId").get<int>();
                arma::vec force = vec(jForce.at("force").get<std::vector<double>>());
                config.forces.emplace_back(moduleId, componentId, force);
            }
        }

        if (j.contains("walls")) {
            for (const auto& jWall : j["walls"]) {
                int moduleId = jWall.at("moduleId").get<int>();
                int componentId = jWall.at("componentId").get<int>();
                config.walls.emplace_back(moduleId, componentId);
            }
        }
        return config;
    }
}
