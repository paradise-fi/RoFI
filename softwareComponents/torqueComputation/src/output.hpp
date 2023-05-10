#pragma once

#include <armadillo>
#include <iostream> 
#include <memory>
#include <unordered_map>

#include "variableManager.hpp"
#include "joint.hpp"

std::string vecToString(const arma::vec3& v);

std::string vecToString(const std::vector<int>& v);

std::string momentsToString(const arma::mat33& moments);

void printJointsInfo(const std::unordered_map<int, std::unique_ptr<rofi::torqueComputation::Joint>>& joints);

void printMatrixInfo(
    const arma::mat& constraintMatrix, 
    const arma::vec& constraintBounds,
    const VariableManager& variableManager
);