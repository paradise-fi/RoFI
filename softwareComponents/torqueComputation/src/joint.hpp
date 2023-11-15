#pragma once

#include <vector>
#include <armadillo>

#include "utils.hpp"
#include "variableManager.hpp"

namespace rofi::torqueComputation {
    /**
     * Main class for torque computation. Contains info about joint and all logic how equations are created.
     */
    class Joint {
    public:
        /**
         * Constructor for rigid joint.
         * @param id ID of joint
         * @param coors Coordinates of joint
         * @param isWall <b>Whether joint is wall - sum of forces does not have to be 0.</b>
         * @param norm Can be used to change moment base of joint, otherwise standard is used.
         */
        Joint(
            int id, 
            arma::vec3 coors, 
            bool isWall,
            std::optional<arma::vec3> norm = std::nullopt
        ) : Joint(id, coors, isWall, norm, false) {}

        /**
         * Constructor for motor joint
         * @param id ID of joint
         * @param coors Coordinates of joint
         * @param norm <b>Have to be perpendicular to plane of rotation.</b>
         */
        Joint(
            int id, 
            arma::vec3 coors, 
            arma::vec3 norm
        ) : Joint(id, coors, false, norm, true) {}

        /**
         * Compute matrix which rows transforms variable vector to vector expressed
         * in standard base. It describes forces affecting this joint.
         * @param variableManager
         * @return
         */
        std::optional<arma::mat> createForceLhs(const VariableManager& variableManager) const;

        /**
         * Compute matrix which rows transforms variable vector to vector expressed
         * in standard base. It describes moments affecting this joint.
         * @param variableManager
         * @return
         */
        std::optional<arma::mat> createMomentLhs(const VariableManager& variableManager) const;

        /**
         * Compute matrix which rows transforms variable vector to vector expressed
         * in standard base. It describes moments affecting edge between this joint and neighbor.
         * (Invariant: (origin, neighbor) == -(neighbor, origin))
         * @param neighbor
         * @param variableManager
         * @return
         */
        arma::mat createMomentEdgeLhs(const Joint& neighbor, const VariableManager& variableManager) const;

        /**
         * Project moments base onto plane perpendicular to vector from this joint
         * to neighbor and create transformation matrix m which express how each
         * moment variable affects force on neighbor.
         * Each row of (M_0, M_1, M_2) @ m is force affecting neighbor and opposite
         * force affects this joint.
         * @param neighbor
         * @return
         */
        arma::mat33 getMomentForces(const Joint& neighbor) const;

        /**
         * Get neighbor by its ID
         * @param neighborId
         * @return
         */
        Joint* getNeighbourById(int neighborId);

        /**
         * Project moments base onto edge vector from this joint to neighbor
         * and create transformation matrix m which express how each
         * moment variable affects this edge.
         * Each row of (M_0, M_1, M_2) @ m is torque affecting edge to neighbor.
         * @param neighbor
         * @return
         */
        arma::mat33 getMomentsProjectedOnEdge(const Joint& neighbor) const;

        /**
         * Add neighbor to this Joint (opposite direction must be created separately).
         * @param neighbor
         */
        void appendNeighbor(Joint* neighbor);

        /**
         * Add force to this joint.
         * @param force
         */
        void appendForce(arma::vec3 force);

        /**
         * Check if joint is in valid state. Throw std::logic_error with error message otherwise.
         */
        void jointSanityCheck() const;

        /**
         * Get sorted neighbor IDs.
         * @return
         */
        std::vector<int> getCanonicNeighborsIds() const;

        /**
         * Set Joint as wall.
         */
        void setIsWall() { isWall = true; }

        bool operator==(const Joint& other) const;

        int getId() const { return id; }
        arma::vec3 getCoors() const { return coors; }
        std::optional<arma::vec3> getNorm() const { return norm; }
        const std::vector<arma::vec3>& getForces() const { return forces; }
        const std::vector<Joint*>& getNeighbors() const { return neighbors; }
        bool getIsWall() const { return isWall; }
        bool getIsBounded() const { return isBounded; }
        const arma::mat33& getMoments() const { return moments; }
        
    private:
        int id;
        arma::vec3 coors;
        std::optional<arma::vec3> norm;
        bool isWall;
        bool isBounded;
        arma::mat33 moments;
        std::vector<Joint*> neighbors;
        std::vector<arma::vec3> forces;

        Joint(
            int id, 
            arma::vec3 coors, 
            bool isWall,
            std::optional<arma::vec3> norm,
            bool isBounded
        );
    };
}
