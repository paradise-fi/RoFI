#include "variableManager.hpp"
#include <cassert>
#include <cmath>

using namespace arma;

VariableManager::VariableManager() : edgesCount(0) {}

void VariableManager::addEdge(
        int fromId,
        int toId,
        std::optional<double> fromBound/* = std::nullopt*/,
        std::optional<double> toBound/* = std::nullopt*/
) {
    if (mapIndices.find(std::make_pair(fromId, toId)) != mapIndices.end() ||
        mapIndices.find(std::make_pair(toId, fromId)) != mapIndices.end()
    ) {
        throw std::logic_error("edge was already added");
    }

    mapIndices[std::make_pair(fromId, toId)] = getVariablesCount();
    mapIndices[std::make_pair(toId, fromId)] = getVariablesCount() + 4;

    if (fromBound) {
        bounds[getVariablesCount() + 2] = fromBound.value();
    }
    if (toBound) {
        bounds[getVariablesCount() + 6] = toBound.value();
    }

    edgesCount++;
}

int VariableManager::getMomentStartIndexForEdge(int fromId, int toId) const {
    auto it = mapIndices.find(std::make_pair(fromId, toId));
    if (it == mapIndices.end()) {
        throw std::out_of_range("invalid edge");
    }
    return it->second;
}

int VariableManager::getForceReactionIndexForEdge(int fromId, int toId) const {
    return (
            getMomentStartIndexForEdge(fromId, toId) +
            getMomentStartIndexForEdge(toId, fromId)
    ) / 2 + 1;
}

int VariableManager::getVariablesCount() const {
    return 7 * edgesCount;
}
int VariableManager::getEdgesCount() const {
    return edgesCount;
}

vec VariableManager::getVariablesLowerBounds() const {
    vec result(getVariablesCount(), fill::value(-INFINITY));
    for (const auto &item: bounds) {
        result[item.first] = -item.second;
    }

    return result;
}

vec VariableManager::getVariablesUpperBounds() const {
    vec result(getVariablesCount(), fill::value(INFINITY));
    for (const auto &item : bounds) {
        result[item.first] = item.second;
    }

    return result;
}

std::vector<std::string> VariableManager::getVariableNames() const {
    std::vector<std::string> result(getVariablesCount(), "");
    for (const auto &edge : mapIndices) {
        auto index = getMomentStartIndexForEdge(edge.first.first, edge.first.second);
        for (int i = 0; i < 3; ++i) {
            result[index + i] =
                    "M_" + std::to_string(edge.first.first) +
                    "_" + std::to_string(edge.first.second) +
                    "-{" + std::to_string(i) + "}";
        }
        result[getForceReactionIndexForEdge(edge.first.first, edge.first.second)] =
                "F_" + std::to_string(edge.first.first) +
                "_" + std::to_string(edge.first.second) + "-lnk";
    }
    return result;
}