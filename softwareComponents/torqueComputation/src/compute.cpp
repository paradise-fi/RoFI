#include <torqueComputation/compute.hpp>
#include <cassert>
#include <cmath>
#include "traverseRofiWorld.hpp"
#include "output.hpp"

using namespace arma;
using namespace rofi::configuration;
using namespace rofi::torqueComputation;


namespace rofi::torqueComputation {
    void sortPairAscending(std::pair<int, int>& p) {
        if (p.first > p.second) {
            std::swap(p.first, p.second);
        }
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
        const std::unordered_map<int, std::unique_ptr<Joint>>& joints,
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
        std::unordered_map<int, std::unique_ptr<Joint>> joints;

        auto roficomMap = traverseRoficomConnections(rofiWorld, joints);
        VariableManager variableManager;

        for (const auto& module : rofiWorld.modules()) {
            try {
                config.visitModuleStrategies.at(module.type)->createJoints(
                    module,
                    roficomMap,
                    joints,
                    variableManager,
                    config.gravitation
                );
            }
            catch (std::out_of_range& e) {
                std::string msg = "TorqueConfig error. Missing strategy for type with value: " + std::to_string(static_cast<int>(module.type)) +
                                  "\nError: " + e.what();
                throw std::logic_error(msg);
            }
        }

        if (joints.size() == 0) {
            return {true, 0, {}};
        }

        updateJoints(config.forces, roficomMap, joints, [](const std::tuple<int, int, vec>& tuple, Joint* joint) { 
            joint->appendForce(std::get<2>(tuple));
        });
        updateJoints(config.walls, roficomMap, joints, [](const std::tuple<int, int>&, Joint* joint) {
            joint->setIsWall();
        });

        if (printJoints) {
            printJointsInfo(joints);
        }
        
        if (solveTo == SolveTo::JointsCreation) {
            return {false, INFINITY, {}};
        }

        int variableCount = variableManager.getVariablesCount();
        size_t mostRowsCount = variableManager.getEdgesCount() * 3 + joints.size() * 6;
        mat constraintMatrix(mostRowsCount, variableCount, fill::zeros);
        vec constraintBounds(mostRowsCount, fill::zeros);

        auto [jointsVisited, edgesVisited] = visitJointsForEquations(0, joints, variableManager, constraintMatrix, constraintBounds);

        assert(jointsVisited == (joints.size()) && "Not all joints were visited!!!");
        assert(edgesVisited == variableManager.getEdgesCount() && "Not all edges were visited!!!");

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
}
