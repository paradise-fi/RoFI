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


struct SnakeEvalCompare {
public:
    bool operator()(const EvalPair& a, const EvalPair& b) const {
        return std::get<0>(a) < std::get<0>(b);
    }
};

inline double eval(const Configuration& configuration, SpaceGrid& sg) {
    sg.loadConfig(configuration);
    return sg.getDist();
}

inline std::vector<Configuration> SnakeStar(const Configuration& init, AlgorithmStat* stat = nullptr, 
    int limit = 5000, double path_pref = 0.1)
{
    unsigned step = 90;
    double free_pref = 1 - path_pref;
    ConfigPred pred;
    ConfigPool pool;

    SpaceGrid grid(init.getIDs().size());
    // Already computed shortest distance from init to configuration.
    ConfigValue initDist;

    // Shortest distance from init through this configuration to goal.
    ConfigValue goalDist;

    double startDist = eval(init, grid);

    if (startDist == 0)
        return {init};

    MinMaxHeap<EvalPair, SnakeEvalCompare> queue(limit);

    const Configuration* pointer = pool.insert(init);
    const Configuration* bestConfig = pointer;
    unsigned bestScore = startDist;
    double worstDist = startDist;

    initDist[pointer] = 0;
    goalDist[pointer] = startDist;
    pred[pointer] = pointer;
    int maxQSize = 0;
    int i = 0;
    queue.push( {goalDist[pointer], pointer} );


    while (!queue.empty() && i++ < limit) {
        maxQSize = std::max(maxQSize, queue.size());
        const auto [d, current] = queue.popMin();
        double currDist = initDist[current];

        std::vector<Configuration> nextCfgs;
        simpleNext(*current, nextCfgs, step);

        for (const auto& next : nextCfgs) {
            const Configuration* pointerNext;
            double newEval = eval(next, grid);
            double newDist = path_pref * (currDist + 1) + free_pref * newEval;
            bool update = false;

            if (newEval != 0 && limit <= queue.size() + i) {
                if (newDist > worstDist)
                    continue;

                worstDist = newDist;
                if (!queue.empty())
                    queue.popMax();
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
                if (stat != nullptr) {
                    stat->pathLength = path.size();
                    stat->queueSize = maxQSize;
                    stat->seenCfgs = pool.size();
                }
                return path;
            }
        }
    }
    auto path = createPath(pred, bestConfig);
    if (stat != nullptr) {
        stat->pathLength = path.size();
        stat->queueSize = maxQSize;
        stat->seenCfgs = pool.size();
    }

    return path;
}

ID closestMass(const Configuration& init) {
    Vector mass = init.massCenter();
    ID bestID = 0;
    double bestDist = std::numeric_limits<double>::max();
    for (const auto& [id, ms] : init.getMatrices()) {
        for (const auto& matrix : ms) {
            double currDist = sqDistVM(matrix, mass);
            if (currDist < bestDist) {
                bestDist = currDist;
                bestID = id;
            }
        }
    }
    return bestID;
}

class MakeStar {
public:
    MakeStar(const Configuration& init, ID root)
    : mass(init.massCenter())
    , config(init)
    , dists() {
        for (const auto& [id, ms] : init.getMatrices()) {
            double currDist = 0;
            for (unsigned side = 0; side < 2; ++side)
                currDist += sqDistVM(ms[side], mass);

            dists.emplace(id, currDist);
        }
    }

    std::vector<Edge> operator()(std::stack<ID>& dfs_stack, std::unordered_set<ID>& seen, ID curr) {
        std::vector<Edge> nEdges = config.getEdges(curr, seen);
        std::sort(nEdges.begin(), nEdges.end(), [&](const Edge& a, const Edge& b){
            return dists[a.id2()] < dists[b.id2()];
        });
        for (const auto& e : nEdges)
            dfs_stack.push(e.id2());

        return nEdges;
    }

private:
    const Vector mass;
    const Configuration& config;
    std::unordered_map<ID, double> dists;
};


using chooseRootFunc = ID(const Configuration&);

template<typename Next>
inline Configuration treefy(const Configuration& init, chooseRootFunc chooseRoot = closestMass) {
    ID root = chooseRoot(init);
    Configuration treed = init;
    treed.clearEdges();
    
    std::unordered_set<ID> seen{};
    std::stack<ID> dfs_stack{};

    Next oracle(init, root);

    dfs_stack.push(root);

    while(!dfs_stack.empty()) {
        ID curr = dfs_stack.top();
        dfs_stack.pop();
        if (seen.find(curr) != seen.end())
            continue;

        seen.insert(curr);
        std::vector<Edge> edges = oracle(dfs_stack, seen, curr);
        for (const auto& e : edges)
            treed.addEdge(e);
    }

    return treed;
}

inline std::vector<Configuration> paralyzedAStar(const Configuration& init, const Configuration& goal, 
    EvalFunction& eval = Eval::trivial, AlgorithmStat* stat = nullptr, std::unordered_set<unsigned> allowed_indices = {})
{
    ConfigPred pred;
    ConfigPool pool;
    unsigned step = 90;

    // Already computed shortest distance from init to configuration.
    ConfigValue initDist;

    // Shortest distance from init through this configuration to goal.
    ConfigValue goalDist;

    if (init == goal)
        return {init};

    std::priority_queue<EvalPair, std::vector<EvalPair>, SnakeEvalCompare> queue;

    const Configuration* pointer = pool.insert(init);
    initDist[pointer] = 0;
    goalDist[pointer] = eval(init, goal);
    pred[pointer] = pointer;
    unsigned long maxQSize = 0;

    queue.push( {goalDist[pointer], pointer} );

    while (!queue.empty()) {
        maxQSize = std::max(maxQSize, queue.size());
        const auto [d, current] = queue.top();
        double currDist = initDist[current];
        queue.pop();

        std::vector<Configuration> nextCfgs;
        paralyzedNext(*current, nextCfgs, step, allowed_indices);
        for (const auto& next : nextCfgs) {
            const Configuration* pointerNext;
            double newDist = currDist + 1 + eval(next, goal);
            bool update = false;

            if (!pool.has(next)) {
                pointerNext = pool.insert(next);
                initDist[pointerNext] = currDist + 1;
                update = true;
            } else {
                pointerNext = pool.get(next);
            }

            if ((currDist + 1 < initDist[pointerNext]) || update) {
                initDist[pointerNext] = currDist + 1;
                goalDist[pointerNext] = newDist;
                pred[pointerNext] = current;
                queue.push({newDist, pointerNext});
            }

            if (next == goal) {
                auto path = createPath(pred, pointerNext);
                if (stat != nullptr) {
                    stat->pathLength = path.size();
                    stat->queueSize = maxQSize;
                    stat->seenCfgs = pool.size();
                }
                return path;
            }
        }
    }
    if (stat != nullptr) {
        stat->pathLength = 0;
        stat->queueSize = maxQSize;
        stat->seenCfgs = pool.size();
    }
    return {};
}
