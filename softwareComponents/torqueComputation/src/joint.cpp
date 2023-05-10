#include "joint.hpp"
#include <iostream>

using namespace arma;

namespace rofi::torqueComputation {
    std::optional<mat> Joint::createForceLhs(const VariableManager& variableManager) const {
        if (isWall) {
            // No equation to add
            return std::nullopt;
        }

        std::string msg;
        if (!jointSanityCheck(msg)) {
            throw std::logic_error(msg);
        }

        mat forceLhs(3, variableManager.getVariablesCount(), fill::zeros);

        for (Joint* neighbor : neighbors) {
            int ownTorqueVars = variableManager.getMomentStartIndexForEdge(id, neighbor->id);
            int neighborTorqueVars = variableManager.getMomentStartIndexForEdge(neighbor->id, id);
            mat ownMomentForces = getMomentForces(*neighbor);
            mat neighborMomentForces = neighbor->getMomentForces(*this);

            for (int i = 0; i < 3; i++) {
                forceLhs.col(ownTorqueVars + i) -= ownMomentForces.row(i).t();
                forceLhs.col(neighborTorqueVars + i) += neighborMomentForces.row(i).t();
            }

            int linkForceVar = variableManager.getForceReactionIndexForEdge(id, neighbor->id);
            vec linkVector = normalise(neighbor->coors - coors);
            forceLhs.col(linkForceVar) += linkVector;
        }

        return forceLhs;
    }

    std::optional<mat> Joint::createMomentLhs(const VariableManager& variableManager) const {
        if (isWall) {
            // No equation to add
            return std::nullopt;
        }

        mat momentLhs(3, variableManager.getVariablesCount(), fill::zeros);

        for (Joint* neighbor : neighbors) {
            int ownTorqueVars = variableManager.getMomentStartIndexForEdge(id, neighbor->id);

            for (int i = 0; i < 3; i++) {
                momentLhs.col(ownTorqueVars + i) += moments.row(i).t();
            }
        }

        return momentLhs;
    }

    mat Joint::createMomentEdgeLhs(const Joint& neighbor, const VariableManager& variableManager) const {
        mat  edgeMomentLhs(3, variableManager.getVariablesCount(), fill::zeros);

        int ownTorqueVars = variableManager.getMomentStartIndexForEdge(id, neighbor.id);
        int neighborTorqueVars = variableManager.getMomentStartIndexForEdge(neighbor.id, id);

        mat  ownMomentsProjected = getMomentsProjectedOnEdge(neighbor);
        mat  neighborMomentsProjected = neighbor.getMomentsProjectedOnEdge(*this);

        for (int i = 0; i < 3; i++) {
             edgeMomentLhs.col(ownTorqueVars + i) +=  ownMomentsProjected.row(i).t();
             edgeMomentLhs.col(neighborTorqueVars + i) -=  neighborMomentsProjected.row(i).t();
        }

        return  edgeMomentLhs;
    }

    mat33 Joint::getMomentForces(const Joint& neighbor) const {
        std::vector<int>  canonicNeighborsIds = getCanonicNeighborsIds();
        if (std::find(
             canonicNeighborsIds.begin(),
             canonicNeighborsIds.end(), 
            neighbor.id) ==  canonicNeighborsIds.end()
        ) {
            throw std::logic_error("Provided joint is not a neighbor of handled joint");
        }

        vec  neighborVector = neighbor.coors - coors;
        mat  momentForces = projectMatrixOntoPlane(moments,  neighborVector) * getCrossDivisionMatrix(neighborVector);

        return  momentForces;
    }

    mat33 Joint::getMomentsProjectedOnEdge(const Joint& neighbor) const {
        std::vector<int>  canonicNeighborsIds = getCanonicNeighborsIds();
        if (std::find(
             canonicNeighborsIds.begin(),
             canonicNeighborsIds.end(), 
            neighbor.id) ==  canonicNeighborsIds.end()
        ) {
            throw std::logic_error("Provided joint is not a neighbor of handled joint");
        }

        vec  neighborVector = neighbor.coors - coors;
        return projectMatrixOntoVector(moments,  neighborVector);
    }

    void Joint::appendNeighbor(Joint* neighbor) {
        neighbors.push_back(neighbor);
    }

    void Joint::appendForce(arma::vec3 force) {
        forces.push_back(force);   
    }

    Joint* Joint::getNeighbourById(int  neighborId) {
        for (Joint* neighbor : neighbors) {
            if (neighbor->id ==  neighborId) {
                return neighbor;
            }
        }
        return nullptr;
    }

    bool Joint::jointSanityCheck(std::string& msg) const {
        if (!isBounded) {
            return true;
        }

        if (2 < neighbors.size()) {
            msg = "Bounded motor can only have at most 2 neighbors";
            return false;
        }

        for (Joint* neighbor : neighbors) {
            vec linkVector = neighbor->coors - coors;
            //Not perpendicular nor parallel is not valid for main moment
            if (!almostEqual(dot(moments.row(2), linkVector), 0.0) &&
                !isParallel(moments.row(2).t(), linkVector)) {
                    std::stringstream stream;
                    stream << "Joint is in invalid state!" << std::endl <<
                              "ID: " << id << std::endl <<
                              "Perpendicular: " << dot(moments.row(2), linkVector) << " == 0.0" << std::endl <<
                              "Norm: " << moments.row(2) <<
                              "Link vector: " << linkVector.t() << std::endl;

                    msg = stream.str();
                    return false;
            }
        }
        return true;
    }

    std::vector<int> Joint::getCanonicNeighborsIds() const {
        std::vector<int>  neighborIds;
        for (Joint* neighbor : neighbors) {
             neighborIds.push_back(neighbor->id);
            
        }
        std::sort( neighborIds.begin(),  neighborIds.end());
        return  neighborIds;
    }

    bool Joint::operator==(const Joint& other) const {
        const double TOLERANCE = 1e-6;
        return id == other.id &&
            approx_equal(coors, other.coors, "absdiff", TOLERANCE) &&
            (
                (norm == std::nullopt && other.norm == std::nullopt) ||
                (
                    norm && other.norm &&
                    approx_equal(norm.value(), other.norm.value(), "absdiff", TOLERANCE)
                )
            ) &&
            isWall == other.isWall &&
            isBounded == other.isBounded &&
            approx_equal(moments, other.moments, "absdiff", TOLERANCE);
    }

    Joint::Joint(
        int id, 
        vec3 coors, 
        bool isWall,    
        std::optional<vec3> normVec,
        bool isBounded
    ) : id(id), coors(coors), norm(normVec), isWall(isWall), isBounded(isBounded) {
        if (normVec) {
            norm = normalise(normVec.value());
            vec momentsOrthoX = normalise(generatePerpendicularVector(norm.value()));
            vec momentsOrthoY = cross(norm.value(), momentsOrthoX);
            mat momentsTemp(3, 3);
            momentsTemp.row(0) = momentsOrthoX.t();
            momentsTemp.row(1) = momentsOrthoY.t();
            momentsTemp.row(2) = norm.value().t();
            moments = momentsTemp;
        }
        else {
            moments = eye<mat>(3, 3); 
        }
    }
}