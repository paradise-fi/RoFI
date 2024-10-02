#pragma once

#include <unordered_map>
#include <vector>
#include <tuple>
#include <optional>
#include <armadillo>

#include "utils.hpp"

/**
 * Stores variable indices. Ensures that same variables are added to same
 * column. Stores variables in block of 7:
 * <ul>
 *  <li> 3 variables for moment a -> b in 3 directions</li>
 *  <li> 1 variable for reaction force on disoriented edge a - b</li>
 *  <li> 3 variables for moment opposite b -> a in 3 directions</li>
 * </ul>
 */
class VariableManager {
public:
    VariableManager();

    /**
     * Register variables for new edge. Moment variable in main direction can be bounded.
     * @param fromId ID of joint on origin of directed edge
     * @param toId ID of joint on end of egde
     * @param fromBound Bound for main moment of joint on origin of directed edge for this edge
     * @param toBound Bound for main moment of joint on end of directed edge for this edge
     */
    void addEdge(
            int fromId,
            int toId,
            std::optional<double> fromBound = std::nullopt,
            std::optional<double> toBound = std::nullopt
    );

    /**
     * Get index of first of three moment variables.
     * @param fromId ID of joint on origin of directed edge
     * @param toId ID of joint on end of directed edge
     * @return Index of first of three moment variables
     */
    int getMomentStartIndexForEdge(int fromId, int toId) const;

    /**
     * Get index of variable describing force reaction.
     * @param fromId ID of joint on origin of directed edge
     * @param toId ID of joint on end of directed edge
     * @return Index of variable describing force reaction
     */
    int getForceReactionIndexForEdge(int fromId, int toId) const;

    /**
     * Get variables count.
     * @return Variables count
     */
    int getVariablesCount() const;

    /**
     * Get registered edges count.
     * @return Edges count
     */
    int getEdgesCount() const;

    /**
     * Get lower bounds for variables.
     * @return Lower bounds for variables
     */
    arma::vec getVariablesLowerBounds() const;

    /**
     * Get upper bounds for variables.
     * @return Upper bounds for variables
     */
    arma::vec getVariablesUpperBounds() const;

    /**
     * Get variables names for debugging purposes.
     * @return Vector of variable names
     */
    std::vector<std::string> getVariableNames() const;

private:
    int edgesCount = 0;
    std::unordered_map<std::pair<int, int>, int, IntPairHash> mapIndices;
    std::unordered_map<int, double> bounds;
};
