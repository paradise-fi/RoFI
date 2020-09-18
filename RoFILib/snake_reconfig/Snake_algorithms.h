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

std::vector<Configuration> SnakeStar(const Configuration& init) {
    unsigned step = 90;
    double path_pref = 0.1;
    double free_pref = 1 - path_pref;
    int limit = 1000; // <- prolly change to `a` times number of modules
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
    int i = 0;
    queue.push( {goalDist[pointer], pointer} );


    while (!queue.empty() && i++ < limit) {
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
                return path;
            }
        }
    }
    auto path = createPath(pred, bestConfig);
    return path;
}

std::vector<Configuration> SnakeStar2(const Configuration& init) {
    unsigned step = 90;
    double path_pref = 0.1;
    double free_pref = 1 - path_pref;
    auto limit = 3 * init.getModules().size();
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
    int i = 0;
    queue.push( {goalDist[pointer], pointer} );


    while (!queue.empty() && i++ < limit) {
        const auto [d, current] = queue.popMin();
        double currDist = initDist[current];

        std::vector<Configuration> nextCfgs;
        smartBisimpleOnlyRotNext(*current, nextCfgs, step);

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
                return path;
            }
        }
    }
    auto path = createPath(pred, bestConfig);
    return path;
}

ID closestMass(const Configuration& init) {
    Vector mass = init.massCenter();
    ID bestID = 0;
    ShoeId bestShoe = A;
    double bestDist = std::numeric_limits<double>::max();
    for (const auto& [id, ms] : init.getMatrices()) {
        for (unsigned i = 0; i < 2; ++i) {
            const auto& matrix = ms[i];
            double currDist = sqDistVM(matrix, mass);
            if (currDist < bestDist) {
                bestDist = currDist;
                bestID = id;
                bestShoe = i ? B : A;
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

bool isSnake(const Configuration& config) {
    for (const auto& [id, el] : config.getEdges()) {
        for (const auto& optEdge : el) {
            if (!optEdge.has_value())
                continue;
            if (optEdge.value().dock1() != ZMinus || optEdge.value().dock2() != ZMinus)
                return false;
        }
    }
    return true;
}

double distFromConn(const Configuration& config, const Edge& connection) {
    if (config.findEdge(connection))
        return 0;
    const auto& realPos = config.getMatrices().at(connection.id2()).at(connection.side2());
    auto wantedPos = config.computeConnectedMatrix(connection);
    return sqDistance(realPos, wantedPos);
}

void addSubtree(ID subRoot, std::unordered_set<ID>& allowed, const std::unordered_map<ID, std::array<std::optional<Edge>, 6>>& spannSucc);

std::unordered_set<ID> makeAllowed(const Configuration& init, ID subroot1, ID subroot2) {
    std::unordered_set<ID> allowed;
    const auto& spannSucc = init.getSpanningSucc();
    for (ID currId : {subroot1, subroot2}) {
        addSubtree(currId, allowed, spannSucc);
    }
    return allowed;
}

Vector findSubtreeMassCenter(const Configuration& init, ID subroot) {
    std::unordered_set<ID> subtree;
    addSubtree(subroot, subtree, init.getSpanningSucc());
    const auto& matrices = init.getMatrices();
    Vector mass({0,0,0,1});
    for (const auto& id : subtree) {
        const auto& ms = matrices.at(id);
        mass += center(ms[A]);
        mass += center(ms[B]);
    }
    mass /= subtree.size()*2;
    return mass;
}

double edgeSpaceEval(const Configuration& config, const Vector& mass, const std::unordered_set<ID>& subtrees) {
    double score = 0;
    for (const auto& [id, ms] : config.getMatrices()) {
        if (subtrees.find(id) != subtrees.end())
            continue;
        for (const auto& m : ms) {
            score += 1 / (distFromVec(m, mass)); //maybe check for div by 0
        }
    }
    return score;
}

std::vector<Configuration> makeEdgeSpace(const Configuration& init, ID subroot1, ID subroot2) {
    Vector mass1 = findSubtreeMassCenter(init, subroot1);
    Vector mass2 = findSubtreeMassCenter(init, subroot2);
    Vector realMass = (mass1 + mass2) / 2;

    std::unordered_set<ID> subtrees;
    addSubtree(subroot1, subtrees, init.getSpanningSucc());
    addSubtree(subroot2, subtrees, init.getSpanningSucc());

    unsigned step = 90;
    unsigned limit = 2 * init.getModules().size();
    double path_pref = 0.1;
    double free_pref = 1 - path_pref;
    ConfigPred pred;
    ConfigPool pool;

    // Already computed shortest distance from init to configuration.
    ConfigValue initDist;

    // Shortest distance from init through this configuration to goal.
    ConfigValue goalDist;

    double startDist = edgeSpaceEval(init, realMass, subtrees);

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
        smartBisimpleOnlyRotNext(*current, nextCfgs, step);

        for (const auto& next : nextCfgs) {
            const Configuration* pointerNext;
            double newEval = edgeSpaceEval(next, realMass, subtrees);
            double newDist = path_pref * (currDist + 1) + free_pref * newEval;
            bool update = false;

            if (limit <= queue.size() + i) {
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
        }
    }
    auto path = createPath(pred, bestConfig);

    return path;

}

std::vector<Configuration> connectArm(const Configuration& init, const Edge& connection, ID subroot1, ID subroot2) {
    // Edge connection is from end of arm to end of arm
    // Slowest part seems to be the makeSapce part, prolly change it a bit
    // Need to check if we choose right sides to be the ends of arms.
    // Maybe add check that the Z ports are both free! (just to be sure)
    auto spacePath = makeEdgeSpace(init, subroot1, subroot2);
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

    auto limit = init.getModules().size();
    MinMaxHeap<EvalPair, SnakeEvalCompare> queue(limit);

    const Configuration* pointer = spacePath.empty() ? pool.insert(init) : pool.insert(spacePath.back());
    initDist[pointer] = 0;
    goalDist[pointer] = startDist;
    pred[pointer] = pointer;

    queue.push( {goalDist[pointer], pointer} );

    std::unordered_set<ID> allowed = makeAllowed(init, subroot1, subroot2);
    int i = 0;
    while (!queue.empty() && i < limit) {
        const auto [d, current] = queue.popMin();
        double currDist = initDist[current];
        if (++i > 100) {
            std::cout << IO::toString(*current) << std::endl;
            i = 0;
        }

        std::vector<Configuration> nextCfgs;
        smartBisimpleParOnlyRotNext(*current, nextCfgs, step, allowed);

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

            if ((currDist + 1 < initDist[pointerNext]) || update) {
                initDist[pointerNext] = currDist + 1;
                goalDist[pointerNext] = newDist;
                pred[pointerNext] = current;
                queue.push({newDist, pointerNext});
            }

            if (newEval == 0) {
                auto path = createPath(pred, pointerNext);
                for (const auto& c : path) {
                    spacePath.emplace_back(c);
                }
                Action::Reconnect connnnn(true, connection);
                Action actttt(connnnn);
                spacePath.emplace_back(executeIfValid(spacePath.back(), actttt).value());
                return spacePath;
            }
        }
    }
    return {};
}

double spaceEval(const Configuration& config, const std::unordered_set<ID>& isolate) {
    double dist = 0;
    const auto& matrixMap = config.getMatrices();
    for (const auto& [id, mArray] : matrixMap) {
        if (isolate.find(id) != isolate.end())
            continue;
        for (auto isolatedId : isolate) {
            const auto& isolatedMArray = matrixMap.at(isolatedId);
            for (const auto& m1 : mArray) {
                for (const auto& m2 : isolatedMArray) {
                    dist += 1 / centerSqDistance(m1, m2);
                }
            }
        }
    }
    return dist;
}

void addSubtree(ID subRoot, std::unordered_set<ID>& allowed, const std::unordered_map<ID, std::array<std::optional<Edge>, 6>>& spannSucc) {
    std::queue<ID> toAdd;
    toAdd.push(subRoot);
    while (!toAdd.empty()) {
        auto currId = toAdd.front();
        toAdd.pop();
        allowed.insert(currId);
        for (const auto& optEdge : spannSucc.at(currId)) {
            if (!optEdge.has_value())
                continue;
            toAdd.push(optEdge.value().id2());
        }
    }
}

bool isFixed(const Configuration& fixed, const std::unordered_set<ID>& toFix) {
    const auto& spannTree = fixed.getSpanningSucc();
    unsigned zCon = 0;
    for (const auto& id : toFix) {
        for (const auto& optEdge : spannTree.at(id)) {
            if (!optEdge.has_value())
                continue;
            const auto& edge = optEdge.value();
            if (toFix.find(edge.id2()) == toFix.end())
                continue;
            if (edge.dock1() != ZMinus || edge.dock2() != ZMinus)
                continue;
            ++zCon;
        }
    }
    return zCon > 1;
}

std::optional<std::vector<Configuration>> boundedLeafDfs(const Configuration& curr, const std::unordered_set<ID>& allowed, std::unordered_set<Configuration, ConfigurationHash>& seen, unsigned limit) {
    if (limit <= 0)
        return {};
    seen.insert(curr);
    std::vector<Configuration> nextCfgs;
    unsigned step = 90;
    paralyzedNext(curr, nextCfgs, step, allowed);
    for (const auto& next : nextCfgs) {
        if (seen.find(next) != seen.end())
            continue;
        if (isFixed(next, allowed))
            return std::vector<Configuration>{next};
        seen.insert(next);
        auto res = boundedLeafDfs(next, allowed, seen, limit - 1);
        if (res.has_value()) {
            res.value().push_back(curr);
            return res.value();
        }
    }
    return {};
}

std::vector<Configuration> leafDfs(const Configuration& init, const std::unordered_set<ID>& allowed) {
    unsigned limit = 1;
    std::unordered_set<Configuration, ConfigurationHash> seen;
    while (true) {
        std::cout << limit << std::endl;
        auto res = boundedLeafDfs(init, allowed, seen, limit);
        if (res.has_value())
            return res.value();
        ++limit;
        seen.clear();
    }
    return {};
}

double moduleDist(ID id1, ID id2, const std::unordered_map<ID, std::array<Matrix, 2>>& matrices) {
    return moduleDistance(matrices.at(id1)[0], matrices.at(id1)[1], matrices.at(id2)[0], matrices.at(id2)[1]);
}

bool areLeafsFree(ID id1, ID id2, ID id3, const Configuration& config) {
    std::unordered_set<ID> toFix = {id1, id2, id3};
    for (auto id : toFix) {
        for (const auto& optEdge : config.getSpanningSucc().at(id)) {
            if (!optEdge.has_value())
                continue;
            const auto& edge = optEdge.value();
            if (toFix.find(edge.id2()) != toFix.end())
                continue;
            return false;
        }
    }
    return true;
}


std::unordered_set<ID> chooseLeafsToMove(const std::unordered_set<ID>& leafs, const Configuration& config) {
    const auto& matrices = config.getMatrices();
    if (leafs.size() < 4)
        return leafs;
    auto best1 = leafs.begin();
    auto best2 = leafs.begin();
    auto best3 = leafs.begin();
    double bestScore = std::numeric_limits<double>::max();
    for (auto it1 = leafs.begin(); it1 != leafs.end(); ++it1) {
        for (auto it2 = it1; it2 != leafs.end(); ++it2) {
            if (it1 == it2)
                continue;
            auto dist12 = moduleDist(*it1, *it2, matrices);
            if (dist12 >= bestScore)
                continue;
            for (auto it3 = it2; it3 != leafs.end(); ++it3) {
                if (it2 == it3)
                    continue;
                auto addDist = dist12 + moduleDist(*it1, *it3, matrices);
                if (addDist >= bestScore)
                    continue;
                addDist += moduleDist(*it2, *it3, matrices);
                if (addDist >= bestScore)
                    continue;
                if (!areLeafsFree(*it1, *it2, *it3, config))
                    continue;
                best1 = it1;
                best2 = it2;
                best3 = it3;
                bestScore = addDist;
            }
        }
    }
    return {*best1, *best2, *best3};
}

std::vector<Configuration> fixLeaf(const Configuration& init, ID toFix) {
    const auto& spannSuccCount = init.getSpanningSuccCount();
    const auto& spannSucc = init.getSpanningSucc();
    const auto& spannPred = init.getSpanningPred();
    if (spannSuccCount.at(toFix) != 0)
        return {};

    if (!spannPred.at(toFix).has_value())
        return {};

    auto pred = spannPred.at(toFix).value().first;
    std::unordered_set<ID> allowed;
    if (spannSuccCount.at(pred) > 1) {
        addSubtree(pred, allowed, spannSucc);
    } else if (spannPred.at(pred).has_value()) {
        addSubtree(spannPred.at(pred).value().first, allowed, spannSucc);
    } else {
        return {};
    }

    auto trueAllowed = chooseLeafsToMove(allowed, init);
    auto res = leafDfs(init, trueAllowed);
    std::vector<Configuration> revRes;
    for (auto i = res.size(); i > 0; --i) {
        revRes.push_back(res[i-1]);
    }
    return revRes;
}

void findLeafs(const Configuration& config, std::vector<std::pair<ID, ShoeId>>& leafsBlack, std::vector<std::pair<ID, ShoeId>>& leafsWhite, std::unordered_map<ID, std::pair<bool, ShoeId>>& allLeafs) {
    leafsBlack.clear();
    leafsWhite.clear();
    allLeafs.clear();
    std::queue<std::tuple<ID, ShoeId, bool>> bag;
    bag.emplace(config.getFixedId(), config.getFixedSide(), true);
    const auto& spannSucc = config.getSpanningSucc();
    const auto& spannPred = config.getSpanningPred();
    while(!bag.empty()) {
        auto [currId, currShoe, isWhite] = bag.front();
        auto otherShoe = currShoe == A ? B : A;
        bag.pop();
        bool isLeaf = true;
        for (const auto& optEdge : spannSucc.at(currId)) {
            if (!optEdge.has_value())
                continue;
            isLeaf = false;
            bag.emplace(optEdge.value().id2(), optEdge.value().side2(), !isWhite);
        }
        if (!isLeaf)
            continue;
        if (!spannPred.at(currId).has_value())
            continue;

        allLeafs[currId] = {isWhite, otherShoe};
        if (!isWhite) {
            leafsBlack.emplace_back(currId, otherShoe);
        } else {
            leafsWhite.emplace_back(currId, otherShoe);
        }
    }
}

void computeSubtreeSizes(const Configuration& config, std::unordered_map<ID, unsigned>& subtreeSizes) {
    std::vector<ID> bag;
    auto moduleCount = config.getModules().size();
    const auto& spannSucc = config.getSpanningSucc();
    bag.reserve(moduleCount);
    unsigned currIndex = 0;
    bag.emplace_back(config.getFixedId());
    while (bag.size() < moduleCount) {
        auto currId = bag[currIndex];
        for (const auto& optEdge : spannSucc.at(currId)) {
            if (!optEdge.has_value())
                continue;
            bag.emplace_back(optEdge.value().id2());
        }
        ++currIndex;
    }
    for (int i = moduleCount - 1; i >= 0; --i) {
        auto currId = bag[i];
        subtreeSizes[currId] = 1;
        for (const auto& optEdge : spannSucc.at(currId)) {
            if (!optEdge.has_value())
                continue;
            subtreeSizes[currId] += subtreeSizes[optEdge.value().id2()];
        }
    }
}

std::tuple<ID, ShoeId, unsigned> computeActiveRadius(const Configuration& config, const std::unordered_map<ID, unsigned>& subtreeSizes, ID id, ShoeId shoe) {
    // Computes radius of LEAF
    const auto& spannPred = config.getSpanningPred();
    if (!spannPred.at(id).has_value())
        return {id, shoe == A ? B : A, 0};
    const auto& spannSucc = config.getSpanningSucc();
    unsigned modRad = 1;
    unsigned radius = 2;
    unsigned prevSize = 1;
    auto currId = id;
    auto currShoe = shoe;
    auto prevId = id;
    auto prevShoe = shoe;
    while (spannPred.at(currId).has_value()) {
        prevId = currId;
        prevShoe = currShoe;
        prevSize = subtreeSizes.at(currId);
        std::tie(currId, currShoe) = spannPred.at(currId).value();
        auto newSize = subtreeSizes.at(currId);
        if (3 * (modRad+1) * prevSize < 2 * modRad * newSize) {
        //if (newSize > 2 * prevSize + 1 || (modRad > 3 && newSize - prevSize > 2)) {
            // Maybe something more like newSize/prevSize > a * (modRad + 1) / modRad
            for (const auto& optEdge : spannSucc.at(currId)) {
                if (!optEdge.has_value())
                    continue;
                if (optEdge.value().id2() != prevId)
                    continue;
                return {prevId, optEdge.value().side2() == A ? B : A, radius};
            }
            throw 42;
        }

        if (!spannPred.at(currId).has_value()) {
            radius += 2;
        } else {
            const auto& [predId, predShoe] = spannPred.at(currId).value();
            for (const auto& optEdge : spannSucc.at(predId)) {
                if (!optEdge.has_value())
                    continue;
                if (optEdge.value().id2() != currId)
                    continue;
                ++modRad;
                if (optEdge.value().side2() == currShoe) {
                    radius += 1;
                } else {
                    radius += 2;
                }
            }
        }
    }

    return {currId, spannPred.at(prevId).value().second == A ? B : A, radius};
}

void computeActiveRadiuses(const Configuration& config, const std::unordered_map<ID, unsigned>& subtreeSizes, const std::vector<std::pair<ID, ShoeId>>& ids, std::unordered_map<ID, std::tuple<ID, ShoeId, unsigned>>& radiuses) {
    for (const auto& [id, shoe] : ids) {
        radiuses[id] = computeActiveRadius(config, subtreeSizes, id, shoe);
    }
}

Configuration disjoinArm(const Configuration& init, const Edge& addedEdge) {
    // it is possible just to change the leafs bags and not recompute them, but we are not doing it now
    const auto& edges = init.getEdges();
    ID root = init.getFixedId();
    ID currId = addedEdge.id2();
    ID prevId = addedEdge.id1();
    ID futId;
    std::optional<Edge> toRemove;
    unsigned edgeCount;
    if (currId == root) {
        currId = prevId;
        prevId = root;
    }
    while (!toRemove.has_value() && currId != root) {
        edgeCount = 0;
        for (const auto& optEdge : edges.at(currId)) {
            if (!optEdge.has_value())
                continue;
            ++edgeCount;
            const auto& edge = optEdge.value();
            if (edge.id2() == prevId)
                continue;
            futId = edge.id2();
        }
        if (edgeCount > 2) {
            for (const auto& optEdge : edges.at(currId)) {
                if (!optEdge.has_value())
                    continue;
                const auto& edge = optEdge.value();
                if (edge.id2() != prevId)
                    continue;
                toRemove = {edge};
                break;
            }
            break;
        }
        prevId = currId;
        currId = futId;
    }

    if (!toRemove.has_value()) {
        for (const auto& optEdge : edges.at(currId)) {
            if (!optEdge.has_value())
                continue;
            const auto& edge = optEdge.value();
            if (edge.id2() != prevId)
                continue;
            toRemove = {edge};
            break;
        }
    }

    Action::Reconnect disc(false, toRemove.value());
    Action act(disc);
    auto a = executeIfValid(init, act);
    if (!a.has_value()) {
        std::cout << "HMMMMMMMM" << std::endl;
        std::cout << IO::toString(init) << std::endl << std::endl;
        std::cout << IO::toString(addedEdge) << std::endl;
    }
    return a.value();

}

using PriorityLeafQueue = std::priority_queue<std::tuple<double, ID, ID>, std::vector<std::tuple<double, ID, ID>>, std::greater<std::tuple<double, ID, ID>>>;

Vector getModuleMass(const Configuration& init, ID id) {
    Vector mass({0,0,0,1});
    for (const auto& m : init.getMatrices().at(id)) {
        mass += center(m);
    }
    return mass/2;
}

void computeDists(const Configuration& init, const std::unordered_map<ID, std::pair<bool, ShoeId>>& leafs, PriorityLeafQueue& dists) {
    for (auto it1 = leafs.begin(); it1 != leafs.end(); ++it1) {
        for (auto it2 = it1; it2 != leafs.end(); ++it2) {
            if (it1 == it2)
                continue;
            ID id1 = it1->first;
            ID id2 = it2->first;
            Vector mass1 = getModuleMass(init, id1);
            Vector mass2 = getModuleMass(init, id2);
            double dist = sqDistance(mass1, mass2);
            dists.emplace(dist, id1, id2);
        }
    }
}

std::vector<Configuration> treeToSnake(const Configuration& init) {
    // držet si konce v prioritní frontě, když to neuspěje, tak popni
    //      podívej se na top fronty + najdi nejbližší jiný konec s opačnou polaritou (který je dostatečně blízko)
    //          vytvoř prostor na zavolání crippled-astar
    //          zavolej vytvoření prostoru
    //          zavolej crippled-astar s magnetem mezi konci
    //          pokud selže, najdi vyzkoušej najít jiný konec
    // jsi had, tak ok. Jinak jsme smutní.

    // DNEŠNÍ ZJIŠTĚNÍ:
    // * chci vyzkoušet kvalitu "makeSpace" v závislosti na limitu
    // podobně vyzkoušet i další, vyzkoumat rychlost konvergence v závislosti na tom,
    // kolikátá iterace to je
    //
    // * makeSpace je v connectArm nejpomalejší část, možná vyzkoušet s menšími
    // limity a "po vrstvách"
    //
    // * zkontrolovat, jestli ve findLeafs označuju správné strany jako ty listové
    //
    // * možná si pamatovat i nelistové strany a nenapojovat jen z-z, ale i nejaké boční,
    // ty pak opravit
    //
    // * úvodní algoritmus na provzdušnění -- vyzkoušet vzdálenost od centerOfMass
    //
    // * fixedModule -> řeší se v treefy #done
    //
    // * jelikož máme nyní spanning tree, možná inteligentněji generovat bisimplenext -- ke každé rotaci
    // přidat jen rotace z modulů, které jsou "níže"
    //
    // * achjo, zdebugovat disjoinArm a zjistit, proč po odstranění to není valid ...

    // NEFUNGUJE

    std::vector<std::pair<ID, ShoeId>> leafsBlack;
    std::vector<std::pair<ID, ShoeId>> leafsWhite;
    std::unordered_map<ID, std::pair<bool, ShoeId>> allLeafs; // true for white, shoeId of real leaf

    std::unordered_map<ID, unsigned> subtreeSizes;
    std::unordered_map<ID, std::tuple<ID, ShoeId, unsigned>> activeRadiuses;

    std::vector<Configuration> path = {init};

    std::vector<Configuration> res;
    std::vector<Configuration> snakeRes;
    PriorityLeafQueue dists;

    while (true) {
        std::cout << "++++++++++++++ Try while ++++++++++++++\n";

        res.clear();
        auto& pConfig = path.back();
        if (isSnake(pConfig))
            return path;

        snakeRes = SnakeStar2(pConfig);
        auto& config = snakeRes.back();
        std::cout << "************** Finish snakeStar2 **************\n";

        findLeafs(config, leafsBlack, leafsWhite, allLeafs);
        const auto& matrices = config.getMatrices();
        const auto& edges = config.getEdges();

        computeSubtreeSizes(config, subtreeSizes);
        computeActiveRadiuses(config, subtreeSizes, leafsBlack, activeRadiuses);
        computeActiveRadiuses(config, subtreeSizes, leafsWhite, activeRadiuses);
        computeDists(config, allLeafs, dists);
        while(!dists.empty()) {
            auto [_d, id, s_id] = dists.top();
            dists.pop();
            std::cout << "|||||||||||||||| Trying " << id << " -> " << s_id << " ||||||||||||||||" << std::endl;
            const auto& [isWhite, shoe] = allLeafs[id];
            const auto& [s_isWhite, s_shoe] = allLeafs[s_id];
            const auto& [subRoot, subRootSide, radius] = activeRadiuses.at(id);
            const auto& [s_subRoot, s_subRootSide, s_radius] = activeRadiuses.at(s_id);
            unsigned rootDist = newyorkCenterDistance(matrices.at(subRoot)[subRootSide], matrices.at(s_subRoot)[s_subRootSide]);
            if (rootDist > radius + s_radius) {
                continue;
            }
            std::optional<Edge> desiredConn;
            if (!isWhite && s_isWhite)
                desiredConn = Edge(id, shoe, ConnectorId::ZMinus, Orientation::North, ConnectorId::ZMinus, s_shoe, s_id);
            else if (isWhite && !s_isWhite)
                desiredConn = Edge(s_id, s_shoe, ConnectorId::ZMinus, Orientation::North, ConnectorId::ZMinus, shoe, id);
            else if (!isWhite) {
                auto s_otherShoe = s_shoe == A ? B : A;
                for (const auto& s_conn : {ZMinus, XPlus, XMinus}) {
                    if (edges.at(s_id)[edgeIndex(s_otherShoe, s_conn)].has_value())
                        continue;
                    desiredConn = Edge(id, shoe, ConnectorId::ZMinus, Orientation::North, s_conn, s_otherShoe, s_id);
                    break;
                }
            }
            if (!desiredConn.has_value()) {
                auto otherShoe = shoe == A ? B : A;
                for (const auto& conn : {ZMinus, XPlus, XMinus}) {
                    if (edges.at(id)[edgeIndex(otherShoe, conn)].has_value())
                        continue;
                    desiredConn = Edge(s_id, s_shoe, ConnectorId::ZMinus, Orientation::North, conn, otherShoe, id);
                    break;
                }
            }
            if (!desiredConn.has_value()) {
                continue;
            }

            res = connectArm(config, desiredConn.value(), subRoot, s_subRoot);
            if (!res.empty()) {
                res.emplace_back(disjoinArm(res.back(), desiredConn.value()));
                break;
            }
        }
        if (res.empty()) {
            std::cout << "We didnt finish, but last computed is this\n";
            if (!path.empty())
                std::cout << IO::toString(path.back()) << std::endl;
            return {};
        }

        for (const auto& snakeConf : snakeRes) {
            path.emplace_back(snakeConf);
        }
        for (const auto& resConf : res)
            path.emplace_back(resConf);
    }
    return path;
}

std::vector<Configuration> reconfigToSnake(const Configuration& init) {
    std::vector<Configuration> path = SnakeStar(init);
    if (path.empty())
        return path;
    path.push_back(treefy<MakeStar>(path.back()));
    auto toSnake = treeToSnake(path.back());
    path.reserve(path.size() + toSnake.size());
    for (const auto& config : toSnake) {
        path.push_back(config);
    }
    // TODO: add fixSnake and changeFixedModule depending on centerOfMass
    return path;
}