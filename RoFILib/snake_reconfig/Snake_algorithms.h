#pragma once

#include <Configuration.h>
#include <IO.h>
#include <Generators.h>
#include <Algorithms.h>
#include "MinMaxHeap.h"
#include "Snake_structs.h"
#include <limits>
#include <queue>
#include <cmath>
#include <memory>
#include <stack>

template<typename T>
void vectorAppend(std::vector<T>& base, const std::vector<T>& toApp) {
    base.reserve(base.size() + toApp.size());
    for (const auto& t : toApp) {
        base.emplace_back(t);
    }
}

struct SnakeEvalCompare {
public:
    bool operator()(const EvalPair& a, const EvalPair& b) const {
        return std::get<0>(a) < std::get<0>(b);
    }
};

template<typename GenNext, typename Score>
std::pair<std::vector<Configuration>, bool> limitedAstar(const Configuration& init, GenNext& genNext, Score& getScore, unsigned limit) {
    unsigned step = 90;
    double path_pref = 0.1;
    double free_pref = 1 - path_pref;
    ConfigPred pred;
    ConfigPool pool;

    // Already computed shortest distance from init to configuration.
    ConfigValue initDist;

    // Shortest distance from init through this configuration to goal.
    ConfigValue goalDist;

    double startDist = getScore(init);

    if (startDist == 0)
        return {std::vector<Configuration>{init}, true};

    MinMaxHeap<EvalPair, SnakeEvalCompare> queue(limit);

    const Configuration* pointer = pool.insert(init);
    const Configuration* bestConfig = pointer;
    double bestScore = startDist;
    double worstDist = startDist;

    initDist[pointer] = 0;
    goalDist[pointer] = startDist;
    pred[pointer] = pointer;
    int i = 0;
    queue.push( {goalDist[pointer], pointer} );


    while (!queue.empty() && i++ < limit) {
        const auto [d, current] = queue.popMin();
        double currDist = initDist[current];

        std::vector<Configuration> nextCfgs;
        genNext(*current, nextCfgs, step);

        for (const auto& next : nextCfgs) {
            const Configuration* pointerNext;
            double newEval = getScore(next);
            double newDist = path_pref * (currDist + 1) + free_pref * newEval;
            bool update = false;

            if (newEval != 0 && limit <= queue.size() + i) {
                if (newDist > worstDist)
                    continue;
                if (!queue.empty()) {
                    queue.popMax();
                    if (!queue.empty()) {
                        const auto [newWorstD, _worstConfig] = queue.max();
                        worstDist = newWorstD;
                    } else {
                        worstDist = newDist;
                    }
                }
            }

            if (newDist > worstDist)
                worstDist = newDist;

            if (!pool.has(next)) {
                pointerNext = pool.insert(next);
                initDist[pointerNext] = currDist + 1;
                update = true;
            } else {
                pointerNext = pool.get(next);
            }

            if (newEval < bestScore) {
                bestScore = newEval;
                bestConfig = pointerNext;
            }

            if ((currDist + 1 < initDist[pointerNext]) || update) {
                initDist[pointerNext] = currDist + 1;
                goalDist[pointerNext] = newDist;
                pred[pointerNext] = current;
                queue.push({newDist, pointerNext});
            }

            if (newEval == 0) {
                auto path = createPath(pred, pointerNext);
                return {path, true};
            }
        }
    }
    auto path = createPath(pred, bestConfig);
    return {path, false};
}


std::vector<Configuration> aerateConfig(const Configuration& init);

std::vector<Configuration> aerateFromRoot(const Configuration& init);

std::vector<Configuration> straightenSnake(const Configuration& init);


using chooseRootFunc = ID(const Configuration&);
ID closestMass(const Configuration& init);

template<typename Next>
inline Configuration treefy(const Configuration& init, chooseRootFunc chooseRoot = closestMass) {
    ID root = chooseRoot(init);
    Configuration treed = init;
    treed.clearEdges();
    treed.setFixed(root, A, identity);

    std::unordered_set<ID> seen{};
    std::stack<ID> dfs_stack{};

    Next oracle(init, root);

    dfs_stack.push(root);

    while (!dfs_stack.empty()) {
        ID curr = dfs_stack.top();
        dfs_stack.pop();
        if (seen.find(curr) != seen.end())
            continue;

        seen.insert(curr);
        std::vector<Edge> edges = oracle(dfs_stack, seen, curr);
        for (const auto& e : edges)
            treed.addEdge(e);
    }
    treed.computeMatrices();
    return treed;
}

std::pair<std::vector<Configuration>, bool> connectArm(const Configuration& init, const Edge& connection, ID subroot1, ID subroot2);

std::pair<std::vector<Configuration>, bool> treeToSnake(const Configuration& init);

std::pair<std::vector<Configuration>, bool> fixParity(const Configuration& init);

std::pair<std::vector<Configuration>, bool> fixDocks(const Configuration& init);

std::pair<std::vector<Configuration>, bool> flattenCircle(const Configuration& init);

std::pair<std::vector<Configuration>, bool> reconfigToSnake(const Configuration& init, std::ofstream* debug_output = nullptr);

std::vector<Configuration> reconfigThroughSnake(const Configuration& from, const Configuration& to);