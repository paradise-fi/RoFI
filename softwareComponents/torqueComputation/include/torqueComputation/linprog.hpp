#pragma once
#include <armadillo>
#include <optional>
#include "ClpSimplex.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinBuild.hpp"
#include "CoinModel.hpp"

namespace rofi::torqueComputation {
    struct OptimizeResult {
        bool feasible;
        double objectiveValue;
        std::optional<std::vector<double>> result;
    };

    /**
     * <p>Minimize a linear objective function subject to linear equality and inequality constraints.
     * Linear programming solves problems of the following form:</p>
     *
     * <p>minimize x: objectiveCoefficients^T @ x</p>
     * <p>such that constraintLowerBounds <= constraintMatrix @ x <= constraintUpperBounds</p>
     * <p>lowerBounds <= x <= upperBounds</p>
     *
     * @param constraintMatrix Each row of constraintMatrix specifies the coefficients of a linear constraint on x.
     * @param constraintLowerBounds Each element represents an lower bound on the corresponding value of constraintMatrix @ x.
     * @param constraintUpperBounds Each element represents an upper bound on the corresponding value of constraintMatrix @ x.
     * @param lowerBounds Each element represents an lower bound on the corresponding value of x.
     * @param upperBounds Each element represents an upper bound on the corresponding value of x.
     * @param objectiveCoefficients The coefficients of the linear objective function to be minimized. If not given, any feasible solution is returned.
     * @return
     */
    OptimizeResult linprog(
            const arma::mat& constraintMatrix,
            const arma::vec& constraintLowerBounds,
            const arma::vec& constraintUpperBounds,
            const arma::vec& lowerBounds,
            const arma::vec& upperBounds,
            const std::optional<arma::vec>& objectiveCoefficients = std::nullopt
    );
}