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
                if (!queue.empty()) {
                    queue.popMax();
                    const auto [newWorstD, _worstConfig] = queue.max();
                    worstDist = newWorstD;
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
    treed.setFixed(root, A, identity); // maybe change the choose root to return shoe as well

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
    EvalFunction& eval = Eval::trivial, AlgorithmStat* stat = nullptr, std::unordered_set<ID> allowed_indices = {})
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

bool isSnake(const Configuration& config) {
    return false;
}

double distFromConn(const Configuration& config, const Edge& connection) {
    if (config.findEdge(connection))
        return 0;
    const auto& realPos = config.getMatrices().at(connection.id2()).at(connection.side2());
    auto wantedPos = config.computeConnectedMatrix(connection);
    return sqDistance(realPos, wantedPos);
}

std::unordered_set<ID> makeAllowed(const Configuration& init, const Edge& connection) {
    std::unordered_set<ID> allowed;
    const auto& spannSuccCount = init.getSpanningSuccCount();
    const auto& spannPred = init.getSpanningPred();
    for (ID currId : {connection.id1(), connection.id2()}) {
        while (spannSuccCount.at(currId) < 2) {
            allowed.insert(currId);
            const auto& currPred = spannPred.at(currId);
            if (!currPred.has_value())
                break;
            currId = currPred.value().first;
        }
    }
    return allowed;
}

std::vector<Configuration> connectArm(const Configuration& init, const Edge& connection, AlgorithmStat* stat = nullptr) {
    // Edge connection is from end of arm to end of arm
    unsigned step = 90;
    double path_pref = 0.1;
    double free_pref = 1 - path_pref;
    ConfigPred pred;
    ConfigPool pool;

    // Already computed shortest distance from init to configuration.
    ConfigValue initDist;

    // Shortest distance from init through this configuration to goal.
    ConfigValue goalDist;

    double startDist = distFromConn(init, connection);
    double worstDist = startDist;

    if (startDist == 0)
        return {init};

    auto limit = init.getModules().size() * 100;
    MinMaxHeap<EvalPair, SnakeEvalCompare> queue(limit);

    const Configuration* pointer = pool.insert(init);
    initDist[pointer] = 0;
    goalDist[pointer] = startDist;
    pred[pointer] = pointer;
    int maxQSize = 0;

    queue.push( {goalDist[pointer], pointer} );

    std::unordered_set<ID> allowed = makeAllowed(init, connection);
    int i = 0;
    while (!queue.empty()) {
        maxQSize = std::max(maxQSize, queue.size());
        const auto [d, current] = queue.popMin();
        double currDist = initDist[current];
        if (++i > 100) {
            std::cout << IO::toString(*current) << std::endl;
            i = 0;
        }

        std::vector<Configuration> nextCfgs;
        biParalyzedOnlyRotNext(*current, nextCfgs, step, allowed);

        for (const auto& next : nextCfgs) {
            const Configuration* pointerNext;
            double newEval = distFromConn(next, connection);
            double newDist = path_pref * (currDist + 1) + free_pref * newEval;
            bool update = false;

            if (newEval != 0 && queue.full()) {
                if (newDist > worstDist)
                    continue;
                if (!queue.empty()) {
                    queue.popMax();
                    const auto [newWorstD, _worstConfig] = queue.max();
                    worstDist = newWorstD;
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
    if (stat != nullptr) {
        stat->pathLength = 0;
        stat->queueSize = maxQSize;
        stat->seenCfgs = pool.size();
    }
    return {};
}

std::vector<Configuration> treeToSnake(const Configuration& init, AlgorithmStat* stat = nullptr) {
    // držet si konce v prioritní frontě, když to neuspěje, tak popni
    //      podívej se na top fronty + najdi nejbližší jiný konec s opačnou polaritou (který je dostatečně blízko)
    //          vytvoř prostor na zavolání crippled-astar
    //          zavolej vytvoření prostoru
    //          zavolej crippled-astar s magnetem mezi konci
    //          pokud selže, najdi vyzkoušej najít jiný konec
    // jsi had, tak ok. Jinak jsme smutní.
    return {};
}

std::vector<Configuration> reconfigToSnake(const Configuration& init, AlgorithmStat* stat = nullptr) {
    std::vector<Configuration> path = SnakeStar(init, stat);
    if (path.empty())
        return path;
    path.push_back(treefy<MakeStar>(path.back()));
    auto toSnake = treeToSnake(path.back(), stat);
    path.reserve(path.size() + toSnake.size());
    for (const auto& config : toSnake) {
        path.push_back(config);
    }
    return path;
}