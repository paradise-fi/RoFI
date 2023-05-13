#pragma once

#include <armadillo>
#include <iostream> 
#include <memory>
#include <unordered_map>

#include "variableManager.hpp"
#include "joint.hpp"

/**
 * Print joints info as JSON to provided stream.
 * @param joints Joints to print
 * @param os Stream where output will be printed to
 */
void printJointsInfo(
    const std::unordered_map<int, std::unique_ptr<rofi::torqueComputation::Joint>>& joints,
    std::ostream& os
);

/**
 * Print joints info as JSON to standard output.
 * @param joints Joints to print
 */
void printJointsInfo(
    const std::unordered_map<int, std::unique_ptr<rofi::torqueComputation::Joint>>& joints
);

/**
 * Print dimension of resulted constraint matrix, variable names, constraint matrix and its bounds to provided stream.
 * @param constraintMatrix Constraint matrix to print
 * @param constraintBounds Bounds of variables to print
 * @param variableManager Variable manager with created variables
 * @param os Stream where output will be printed to
 */
void printMatrixInfo(
    const arma::mat& constraintMatrix, 
    const arma::vec& constraintBounds,
    const VariableManager& variableManager,
    std::ostream& os
);

/**
 * Print dimension of resulted constraint matrix, variable names, constraint matrix and its bounds to standard output.
 * @param constraintMatrix Constraint matrix to print
 * @param constraintBounds Bounds of variables to print
 * @param variableManager Variable manager with created variables
 */
void printMatrixInfo(
    const arma::mat& constraintMatrix, 
    const arma::vec& constraintBounds,
    const VariableManager& variableManager
);