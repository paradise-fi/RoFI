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
std::vector<Configuration> limitedAstar(const Configuration& init, GenNext& genNext, Score& getScore, unsigned limit, bool returnBest) {
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
                return path;
            }
        }
    }
    if (!returnBest)
        return {};
    auto path = createPath(pred, bestConfig);
    return path;
}

std::vector<Configuration> aerateConfig(const Configuration& init) {
    auto nModule = init.getModules().size();
    int limit = 2 * nModule * nModule;

    SimpleNextGen simpleGen{};
    SpaceGridScore grid(init.getIDs().size());

    return limitedAstar(init, simpleGen, grid, limit, true);
}

std::vector<Configuration> aerateFromRoot(const Configuration& init) {
    SmartBisimpleOnlyRotGen smartGen{};
    AwayFromRootScore score{};

    return limitedAstar(init, smartGen, score, 3 * init.getModules().size(), true);
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
    treed.computeMatrices();
    return treed;
}

/* ==================== end TREEFY ==================== */

bool isPseudoSnake(const Configuration& config) {
    const auto& spannSuccCount = config.getSpanningSuccCount();
    ID fixedId = config.getFixedId();
    for (const auto& [id, sc] : spannSuccCount) {
        if (sc < 2)
            continue;
        if (sc == 2 && id == fixedId)
            continue;
        return false;
    }
    return true;
}

bool isParitySnake(const Configuration& config) {
    const auto& spannSucc = config.getSpanningSucc();
    std::queue<std::pair<ID, ShoeId>> bag;
    ID fixedId = config.getFixedId();
    for (const auto& optEdge : spannSucc.at(fixedId)) {
        if (!optEdge.has_value())
            continue;
        bag.emplace(optEdge.value().id2(), optEdge.value().side2());
    }
    while(!bag.empty()) {
        auto [id, side] = bag.front();
        bag.pop();
        for (const auto& optEdge : spannSucc.at(id)) {
            if (!optEdge.has_value())
                continue;
            const Edge& edge = optEdge.value();
            if (edge.side1() == side)
                return false;
            bag.emplace(edge.id2(), edge.side2());
        }
    }
    return true;
}

std::optional<Edge> getInvalidEdge(const Configuration& config) {
    for (const auto& [id, el] : config.getEdges()) {
        for (const auto& optEdge : el) {
            if (!optEdge.has_value())
                continue;
            if (optEdge.value().dock1() != ZMinus || optEdge.value().dock2() != ZMinus || optEdge.value().ori() != North)
                return optEdge.value();
        }
    }
    return {};
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

std::vector<Configuration> makeEdgeSpace(const Configuration& init, ID subroot1, ID subroot2) {
    Vector mass1 = findSubtreeMassCenter(init, subroot1);
    Vector mass2 = findSubtreeMassCenter(init, subroot2);
    Vector realMass = (mass1 + mass2) / 2;

    std::unordered_set<ID> subtrees;
    addSubtree(subroot1, subtrees, init.getSpanningSucc());
    addSubtree(subroot2, subtrees, init.getSpanningSucc());

    SmartBisimpleOnlyRotGen smartGen{};
    EdgeSpaceScore edgeScore(realMass, subtrees);

    return limitedAstar(init, smartGen, edgeScore, 2 * init.getModules().size(), true);
}

std::vector<Configuration> connectArm(const Configuration& init, const Edge& connection, ID subroot1, ID subroot2) {
    // Edge connection is from end of arm to end of arm

    auto spacePath = makeEdgeSpace(init, subroot1, subroot2);
    std::unordered_set<ID> allowed = makeAllowed(init, subroot1, subroot2);

    BiParalyzedGen parGen(allowed);
    DistFromConnScore connScore(connection);

    auto astarPath = limitedAstar(spacePath.back(), parGen, connScore, init.getModules().size(), false);

    if (astarPath.empty())
        return {};

    vectorAppend(spacePath, astarPath);
    return spacePath;
}

double moduleDist(ID id1, ID id2, const std::unordered_map<ID, std::array<Matrix, 2>>& matrices) {
    return moduleDistance(matrices.at(id1)[0], matrices.at(id1)[1], matrices.at(id2)[0], matrices.at(id2)[1]);
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
            bool isNextWhite = currShoe == optEdge.value().side1() ? !isWhite : isWhite;
            bag.emplace(optEdge.value().id2(), optEdge.value().side2(), isNextWhite);
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
    if (config.getSpanningSuccCount().at(config.getFixedId()) == 1) {
        ShoeId side;
        for (const auto& optEdge : spannSucc.at(config.getFixedId())) {
            if (!optEdge.has_value())
                continue;
            side = optEdge.value().side1() == A ? B : A;
            break;
        }
        bool isRootWhite = config.getFixedSide() == side;
        allLeafs[config.getFixedId()] = {isRootWhite, side};
        if (isRootWhite)
            leafsWhite.emplace_back(config.getFixedId(), side);
        else
            leafsBlack.emplace_back(config.getFixedId(), side);
    }
}

void computeSubtreeSizes(const Configuration& config, std::unordered_map<ID, unsigned>& subtreeSizes) {
    std::vector<ID> bag;
    auto moduleCount = config.getMatrices().size();
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

void computeDists(const Configuration& config, const std::unordered_map<ID, std::pair<bool, ShoeId>>& leafs, PriorityLeafQueue& dists) {
    for (auto it1 = leafs.begin(); it1 != leafs.end(); ++it1) {
        for (auto it2 = it1; it2 != leafs.end(); ++it2) {
            if (it1 == it2)
                continue;
            ID id1 = it1->first;
            ID id2 = it2->first;
            Vector mass1 = config.getModuleMass(id1);
            Vector mass2 = config.getModuleMass(id2);
            double dist = sqDistance(mass1, mass2);
            dists.emplace(dist, id1, id2);
        }
    }
}

std::vector<Configuration> treeToSnake(const Configuration& init) {
    // DNEŠNÍ ZJIŠTĚNÍ:
    // * chci vyzkoušet kvalitu "makeSpace" v závislosti na limitu
    // podobně vyzkoušet i další, vyzkoumat rychlost konvergence v závislosti na tom,
    // kolikátá iterace to je
    //
    // * makeSpace je v connectArm nejpomalejší část, možná vyzkoušet s menšími
    // limity a "po vrstvách"
    //

    std::vector<std::pair<ID, ShoeId>> leafsBlack;
    std::vector<std::pair<ID, ShoeId>> leafsWhite;
    std::unordered_map<ID, std::pair<bool, ShoeId>> allLeafs; // true for white, shoeId of real leaf

    std::unordered_map<ID, unsigned> subtreeSizes;
    std::unordered_map<ID, std::tuple<ID, ShoeId, unsigned>> activeRadiuses;

    std::vector<Configuration> path = {init};

    std::vector<Configuration> res;
    std::vector<Configuration> snakeRes;

    while (true) {
        std::cout << "++++++++++++++ Try while ++++++++++++++\n";

        res.clear();
        auto& pConfig = path.back();
        if (isPseudoSnake(pConfig))
            return path;

        snakeRes = aerateFromRoot(pConfig);
        auto& config = snakeRes.back();
        std::cout << "************** Finish aerateFromRoot **************\n";

        findLeafs(config, leafsBlack, leafsWhite, allLeafs);
        const auto& matrices = config.getMatrices();
        const auto& edges = config.getEdges();

        computeSubtreeSizes(config, subtreeSizes);
        computeActiveRadiuses(config, subtreeSizes, leafsBlack, activeRadiuses);
        computeActiveRadiuses(config, subtreeSizes, leafsWhite, activeRadiuses);
        PriorityLeafQueue dists;
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

std::vector<Configuration> straightenSnake(const Configuration& init) {
    SmartBisimpleOnlyRotGen smartGen{};
    FurthestPointsScore furthestScore{};

    return limitedAstar(init, smartGen, furthestScore, init.getModules().size(), true);
}

std::vector<std::pair<ID, ShoeId>> colourAndFindLeafs(const Configuration& config, std::unordered_map<ID,bool>& colours) {
    colours.clear();
    std::queue<std::tuple<ID, ShoeId, bool>> bag;
    std::vector<std::pair<ID, ShoeId>> leafs;
    bag.emplace(config.getFixedId(), config.getFixedSide(), true);
    const auto& spannSucc = config.getSpanningSucc();
    while(!bag.empty()) {
        auto [currId, currShoe, isWhite] = bag.front();
        auto otherShoe = currShoe == A ? B : A;
        bag.pop();
        bool isLeaf = true;
        for (const auto& optEdge : spannSucc.at(currId)) {
            if (!optEdge.has_value())
                continue;
            isLeaf = false;
            bool isNextWhite = currShoe == optEdge.value().side1() ? !isWhite : isWhite;
            bag.emplace(optEdge.value().id2(), optEdge.value().side2(), isNextWhite);
        }

        colours[currId] = (isWhite && currShoe == A) || (!isWhite && currShoe == B);
        if (!isLeaf)
            continue;
        leafs.emplace_back(currId, currShoe == A ? B : A);
    }
    if (config.getSpanningSuccCount().at(config.getFixedId()) == 1) {
        for (const auto& optEdge : spannSucc.at(config.getFixedId())) {
            if (!optEdge.has_value())
                continue;
            leafs.emplace_back(config.getFixedId(), optEdge.value().side1() == A ? B : A);
            break;
        }
    }
    return leafs;
}

bool canConnect(ShoeId shoe1, bool aColour1, ShoeId shoe2, bool aColour2) {
    return (shoe1 == shoe2 && aColour1 != aColour2) || (shoe1 != shoe2 && aColour1 == aColour2);
}

std::pair<Edge, unsigned> getEdgeToStrictDisjoinArm(const Configuration& config, const Edge& added) {
    unsigned len = 0;
    ID currId = added.id2();
    ShoeId currShoe = added.side2();
    ID prevId = added.id1();
    const auto& edges = config.getEdges();
    while (true) {
        for (const auto& optEdge : edges.at(currId)) {
            if (!optEdge.has_value())
                continue;
            const Edge& edge = optEdge.value();
            if (edge.id2() == prevId)
                continue;
            if (edge.side1() == currShoe)
                return {edge, len};
            prevId = currId;
            currId = edge.id2();
            currShoe = edge.side2();
            break;
        }
        ++len;
    }
}

ConnectorId getEmptyConn(const Configuration& init, ID id, ShoeId shoe) {
    if (!init.getEdges().at(id)[edgeIndex(shoe, ZMinus)].has_value())
        return ZMinus;
    else if (!init.getEdges().at(id)[edgeIndex(shoe, XPlus)].has_value())
        return XPlus;
    return XMinus;
}

std::vector<Configuration> fixParity(const Configuration& init) {
    std::vector<Configuration> path = {init};
    std::vector<Configuration> straightRes;
    std::vector<Configuration> res;
    std::unordered_map<ID, bool> colours; // true iff A is White
    while (true) {
        res.clear();
        auto& preConfig = path.back();
        if (isParitySnake(preConfig))
            return path;
        straightRes = straightenSnake(preConfig);
        auto& config = straightRes.back();
        auto leafs = colourAndFindLeafs(config, colours);
        if (leafs.size() != 2) {
            std::cout << "Wut\n";
            std::cout << IO::toString(config) << std::endl;
            throw 55;
        }
        auto [id1, side1] = leafs[0];
        auto [id2, side2] = leafs[1];
        Edge desiredEdge(id1, side1, ConnectorId::ZMinus, Orientation::North, ConnectorId::ZMinus, side2, id2);
        std::cout << "************ Another one ************" << std::endl;

        if (canConnect(side1, colours[id1], side2, colours[id2])) {
            res = connectArm(config, desiredEdge, config.getFixedId(), config.getFixedId());
            if (res.empty()) {
                std::cout << "We sad, but we computed last this:\n";
                std::cout << IO::toString(straightRes.back()) << std::endl;
                std::cout << IO::toString(desiredEdge) << std::endl;
                return {};
            }
            auto toRemove = getEdgeToStrictDisjoinArm(res.back(), desiredEdge).first;
            Action::Reconnect disc(false, toRemove);
            Action act(disc);
            auto a = executeIfValid(res.back(), act);
            if (!a.has_value()) {
                std::cout << "HMMMMMMMMXX" << std::endl;
                std::cout << IO::toString(res.back()) << std::endl << std::endl;
                std::cout << IO::toString(toRemove) << std::endl;
            }
            res.emplace_back(a.value());
        } else {
            std::cout << "We doin this" << std::endl;
            auto [parityBack, len1] = getEdgeToStrictDisjoinArm(config, desiredEdge);
            auto [otherPar, len2] = getEdgeToStrictDisjoinArm(config, reverse(desiredEdge));
            if (len1 > len2) {
                parityBack = otherPar;
                id1 = id2;
                side1 = side2;
            }
            ShoeId wside = parityBack.side1() == A ? B : A;
            ConnectorId wconn = getEmptyConn(config, parityBack.id1(), wside);
            auto realDesire = Edge(id1, side1, ZMinus, North, wconn, wside, parityBack.id1());
            res = connectArm(config, realDesire, config.getFixedId(), config.getFixedId());
            if (res.empty()) {
                std::cout << "We sad, but we computed last this:\n";
                std::cout << IO::toString(straightRes.back()) << std::endl;
                std::cout << IO::toString(realDesire) << std::endl;
                return {};
            }
            Action::Reconnect disc(false, parityBack);
            Action act(disc);
            auto a = executeIfValid(res.back(), act);
            if (!a.has_value()) {
                std::cout << "HMMMMMMMM2" << std::endl;
                std::cout << IO::toString(res.back()) << std::endl << std::endl;
                std::cout << IO::toString(parityBack) << std::endl;
            }
            res.emplace_back(a.value());
        }
        for (const auto& conf : straightRes) {
            path.emplace_back(conf);
        }
        for (const auto& conf : res) {
            path.emplace_back(conf);
        }
    }
    return {};
}

Edge missingCircle(const Configuration& config) {
    std::vector<std::pair<ID, ShoeId>> leafs;
    std::queue<std::pair<ID, ShoeId>> bag;
    bag.emplace(config.getFixedId(), config.getFixedSide());
    const auto& spannSucc = config.getSpanningSucc();
    while(!bag.empty()) {
        auto [currId, currShoe] = bag.front();
        bag.pop();
        bool isLeaf = true;
        for (const auto& optEdge : spannSucc.at(currId)) {
            if (!optEdge.has_value())
                continue;
            isLeaf = false;
            bag.emplace(optEdge.value().id2(), optEdge.value().side2());
        }
        if (!isLeaf)
            continue;

        leafs.emplace_back(currId, currShoe == A ? B : A);
    }
    if (leafs.size() == 1 && config.getSpanningSuccCount().at(config.getFixedId()) == 1) {
        for (const auto& optEdge : spannSucc.at(config.getFixedId())) {
            if (!optEdge.has_value())
                continue;
            leafs.emplace_back(config.getFixedId(), optEdge.value().side1() == A ? B : A);
        }
    }
    if (leafs.size() != 2) {
        std::cout << "Bruh " << leafs.size() << std::endl;
        throw 1234;
    }
    return Edge(leafs[0].first, leafs[0].second, ZMinus, North, ZMinus, leafs[1].second, leafs[1].first);
}

std::vector<Configuration> fixDocks(const Configuration& init) {
    auto path = straightenSnake(init);
    auto missing = missingCircle(path.back());
    auto circle = connectArm(path.back(), missing, path.back().getFixedId(), path.back().getFixedId());
    std::vector<Configuration> res;
    if (circle.empty()) {
        std::cout << "We weri saad" << std::endl;
        std::cout << IO::toString(path.back()) << std::endl;
        std::cout << IO::toString(missing) << std::endl;
        return {};
    }
    path.reserve(path.size() + circle.size());
    for (const auto& conf : circle) {
        path.emplace_back(conf);
    }
    while (true) {
        res.clear();
        const auto& preConfig = path.back();
        std::optional<Edge> optInvalid = getInvalidEdge(path.back());
        if (!optInvalid.has_value())
            return path;
        Edge& invalid = optInvalid.value();
        std::cout << IO::toString(invalid) << std::endl;
        Action::Reconnect disc(false, invalid);
        Action act(disc);
        auto a = executeIfValid(preConfig, act);
        if (!a.has_value()) {
            std::cout << "HMMMMMMMMCIRCLE" << std::endl;
            std::cout << IO::toString(res.back()) << std::endl << std::endl;
            std::cout << IO::toString(invalid) << std::endl;
        }
        const auto& config = a.value();
        Edge replaceEdg(invalid.id1(), invalid.side1(), ZMinus, North, ZMinus, invalid.side2(), invalid.id2());
        res = connectArm(config, replaceEdg, config.getFixedId(), config.getFixedId());
        if (res.empty()) {
            std::cout << "We weri saadXXX" << std::endl;
            std::cout << IO::toString(config) << std::endl;
            std::cout << IO::toString(replaceEdg) << std::endl;
            return {};
        }
        path.reserve(path.size() + res.size() + 1);
        path.emplace_back(config);
        for (const auto& conf : res) {
            path.emplace_back(conf);
        }
    }
}

Configuration fixRots(const Configuration& init) {
    std::vector<Action::Rotate> rots;
    for (const auto& [id, module] : init.getModules()) {
        for (auto j : {Alpha, Beta, Gamma}) {
            double angle = module.getJoint(j);
            if (angle == 0)
                continue;
            rots.emplace_back(id, j, -angle);
        }
    }
    Action act(rots, {});
    return executeIfValid(init, act).value();
}

std::vector<Configuration> flattenCircle(const Configuration& init) {
    std::optional<Edge> toRemove;
    for (const auto& optEdge : init.getSpanningSucc().at(init.getFixedId())) {
        if (!optEdge.has_value())
            continue;
        toRemove = optEdge;
        break;
    }
    Action::Reconnect disc(false, toRemove.value());
    Action act(disc);
    auto a = executeIfValid(init, act);
    if (!a.has_value()) {
        std::cout << "HMMMMMMMMFLAT" << std::endl;
        std::cout << IO::toString(init) << std::endl << std::endl;
        std::cout << IO::toString(toRemove.value()) << std::endl;
    }
    auto res = aerateConfig(a.value());

    std::vector<Configuration> path = {a.value()};
    path.reserve(res.size());
    for (const auto& conf : res) {
        path.emplace_back(conf);
    }
    path.emplace_back(fixRots(path.back()));
    return path;
}

std::vector<Configuration> reconfigToSnake(const Configuration& init) {
    std::vector<Configuration> path = aerateConfig(init);
    std::cout << "Finish aerate" << std::endl;
    if (path.empty())
        return path;
    path.push_back(treefy<MakeStar>(path.back()));

    auto toSnake = treeToSnake(path.back());
    std::cout << "Finish treeToSnake" << std::endl;
    if (toSnake.empty())
        return {};
    vectorAppend(path, toSnake);

    auto fixedSnake = fixParity(path.back());
    std::cout << "Finish fixParity" << std::endl;
    if (fixedSnake.empty())
        return {};
    vectorAppend(path, fixedSnake);

    auto dockSnake = fixDocks(path.back());
    std::cout << "Finish fixDocks" << std::endl;
    if (dockSnake.empty())
        return {};
    vectorAppend(path, dockSnake);

    auto flatCircle = flattenCircle(path.back());
    std::cout << "Finish flattenCircle" << std::endl;
    if (flatCircle.empty())
        return {};
    vectorAppend(path, flatCircle);

    std::cout << "Finish ALL" << std::endl;
    return path;
}