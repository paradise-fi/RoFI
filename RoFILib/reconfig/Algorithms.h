//
// Created by xvozarov on 2/18/19.
//

#ifndef ROBOTS_BFS_H
#define ROBOTS_BFS_H

#include "../Configuration.h"
#include "../IO.h"
#include <queue>
#include <cmath>
#include <memory>

class ConfigurationPtrHash
{
public:
    std::size_t operator()(const std::unique_ptr<Configuration>& ptr) const
    {
        return ConfigurationHash{}(*ptr);
    }
};

inline bool operator==(const std::unique_ptr<Configuration>& a, const std::unique_ptr<Configuration>& b)
{
    return (*a) == (*b);
}

using ConfigPred = std::unordered_map<const Configuration*, const Configuration*>;
using ConfigEdges = std::unordered_map<const Configuration*, std::vector<const Configuration*>>;
using ConfigValue = std::unordered_map<const Configuration*, double>;
using EvalFunction = double(const Configuration&, const Configuration&);
using DistFunction = double(const Configuration&, const Configuration&);
using EvalPair = std::tuple<double, const Configuration*>;

struct EvalCompare
{
public:
    bool operator()(const EvalPair& a, const EvalPair& b)
    {
        auto& [a1, _a] = a;
        auto& [b1, _b] = b;
        return a1 > b1;
    }
};

class ConfigPool
{
public:
    typedef std::unordered_set<std::unique_ptr<Configuration>, ConfigurationPtrHash>::iterator iterator;
    typedef std::unordered_set<std::unique_ptr<Configuration>, ConfigurationPtrHash>::const_iterator const_iterator;

    iterator begin() { return pool.begin(); }
    const_iterator begin() const { return pool.begin(); }
    iterator end() { return pool.end(); }
    const_iterator end() const { return pool.end(); }

    size_t size() const
    {
        return pool.size();
    }

    Configuration* insert(const Configuration& config)
    {
        auto tmp = std::make_unique<Configuration>(config);
        auto [ptr, _] = pool.insert(std::move(tmp));
        return ptr->get();
    }

    bool find(const Configuration& config) const
    {
        auto tmp = std::make_unique<Configuration>(config);
        return (pool.find(tmp) != pool.end());
    }

    const Configuration* get(const Configuration& config) const
    {
        auto tmp = std::make_unique<Configuration>(config);
        return pool.find(tmp)->get();
    }

private:
    std::unordered_set<std::unique_ptr<Configuration>, ConfigurationPtrHash> pool;
};


namespace Eval
{
    inline double trivial(const Configuration& conf, const Configuration& goal)
    {
        return 1;
    }

    inline double jointDiff(const Configuration& curr, const Configuration& goal)
    {
        double result = 0;
        for ( auto& [id, mod] : curr.getModules() )
        {
            const Module& other = goal.getModules().at(id);
            for (Joint j : {Alpha, Beta, Gamma})
            {
                double diff = mod.getJoint(j) - other.getJoint(j);
                //result += sqrt(diff * diff);
                result += std::abs(diff) / 90;
            }
        }
        return result;
    }

    inline double centerDiff(const Configuration& curr, const Configuration& goal)
    {
        double result = 0;
        for ( auto& [id, ms] : curr.getMatrices() )
        {
            const auto& other = goal.getMatrices().at(id);
            for (Side s : {A, B})
            {
                result += distance(center(ms[s]), center(other[s]));
            }
        }
        return result;
    }

    inline double matrixDiff(const Configuration& curr, const Configuration& goal)
    {
        double result = 0;
        for ( auto& [id, ms] : curr.getMatrices() )
        {
            const auto& other = goal.getMatrices().at(id);
            for (Side s : {A, B})
            {
                result += distance(ms[s], other[s]);
            }
        }
        return result;
    }

    inline double actionDiff(const Configuration& curr, const Configuration& goal)
    {
        Action action = curr.diff(goal);
        return action.rotations().size() + action.reconnections().size() ;
    }
}

namespace Distance
{
    inline double reconnections(const Configuration& curr, const Configuration& goal)
    {
        auto diff = curr.diff(goal);
        return diff.reconnections().size();
    }

    inline double rotations(const Configuration& curr, const Configuration& goal)
    {
        auto diff = curr.diff(goal);
        return diff.rotations().size();
    }

    inline double diff(const Configuration& curr, const Configuration& goal)
    {
        return reconnections(curr, goal) * 1000 + rotations(curr, goal);
    }
}

inline std::vector<Configuration> createPath(ConfigPred& pred, const Configuration* goal)
{
    std::vector<Configuration> res;
    const Configuration* current = goal;

    while (current != pred.at(current))
    {
        res.push_back(*current);
        current = pred.at(current);
    }
    res.push_back(*current);

    std::reverse(res.begin(), res.end());
    return res;
}

inline std::vector<Configuration> createPath(ConfigEdges& edges, const Configuration* init, const Configuration* goal)
{
    ConfigPred pred;
    std::set<const Configuration*> seen;
    if (init == goal)
    {
        return {*init};
    }

    seen.insert(init);
    pred.insert({init, init});

    std::queue<const Configuration*> queue;
    queue.push(init);

    while (!queue.empty())
    {
        const auto curr = queue.front();
        queue.pop();

        for (const auto* next : edges[curr])
        {
            if (seen.find(next) == seen.end())
            {
                seen.insert(next);
                pred.insert({next, curr});

                if (next == goal)
                {
                    return createPath(pred, goal);
                }

                queue.push(next);
            }
        }
    }

    return {};
}

inline std::vector<Configuration> BFS(const Configuration& init, const Configuration& goal, unsigned step = 90, unsigned bound = 1)
{
    //Assume both configs are consistent and valid.
    ConfigPred pred;
    ConfigPool pool;

    if (init == goal)
    {
        return {init};
    }

    const Configuration* pointer = pool.insert(init);
    pred.insert({pointer, pointer});

    std::queue<const Configuration*> queue;
    queue.push(pointer);

    while (!queue.empty())
    {
        const auto current = queue.front();
        queue.pop();

        std::vector<Configuration> nextCfgs;
        current->next(nextCfgs, step, bound);
 //       std::cout << queue.size() << " " << nextCfgs.size() << std::endl;

        for (const auto& next : nextCfgs)
        {
            if (!pool.find(next))
            {
                const Configuration* pointerNext = pool.insert(next);
                pred.insert({pointerNext, current});

                if (next == goal)
                {
                    return createPath(pred, pointerNext);
                }
                queue.push(pointerNext);
            }

        }
    }
    return {};
}

inline std::vector<Configuration> AStar(const Configuration& init, const Configuration& goal, unsigned step = 90, unsigned bound = 1, EvalFunction& eval = Eval::trivial)
{
    ConfigPred pred;
    ConfigPool pool;

    // Already computed shortest distance from init to configuration.
    ConfigValue initDist;

    // Shortest distance from init through this configuration to goal.
    ConfigValue goalDist;

    if (init == goal)
    {
        return {init};
    }

    std::priority_queue<EvalPair, std::vector<EvalPair>, EvalCompare> queue;

    const Configuration* pointer = pool.insert(init);
    initDist[pointer] = 0;
    goalDist[pointer] = eval(init, goal);
    pred[pointer] = pointer;

    queue.push( {goalDist[pointer], pointer} );

    while (!queue.empty())
    {
        const auto [d, current] = queue.top();
        double currDist = initDist[current];
        queue.pop();


        std::vector<Configuration> nextCfgs;
        current->next(nextCfgs, step, bound);
        for (const auto& next : nextCfgs)
        {
            const Configuration* pointerNext;
            double newDist = currDist + 1 + eval(next, goal);
            bool update = false;

            if (!pool.find(next))
            {
                pointerNext = pool.insert(next);
                initDist[pointerNext] = currDist + 1;
                update = true;
            }
            else
            {
                pointerNext = pool.get(next);
            }

            if ((currDist + 1 < initDist[pointerNext]) || update)
            {
                initDist[pointerNext] = currDist + 1;
                goalDist[pointerNext] = newDist;
                pred[pointerNext] = current;
                queue.push({newDist, pointerNext});
            }

            if (next == goal)
            {
                return createPath(pred, pointerNext);
            }
        }
    }
    return {};
}

/* RRT Functions for random configuration generation
 *
 * */

template<typename T>
inline void getAllSubsetsLess(const std::vector<T>& set, std::vector<std::vector<T>>& res, std::vector<T> accum, unsigned index, unsigned count)
{
    if ((count == 0) || (index == set.size()))
    {
        res.push_back(accum);
        return;
    }
    for (unsigned i = index; i < set.size(); ++i)
    {
        auto next = accum;
        next.push_back(set[i]);
        getAllSubsetsLess(set, res, next, i + 1, count - 1);
        getAllSubsetsLess(set, res, accum, i + 1, count);
    }
}

inline int findFreeIndex(int index, const std::array<bool, 6>& A)
{
    int count = 6;
    while (!A[index] && count > 0)
    {
        index = (index + 1) % 6;
        --count;
    }
    if (count == 0)
    {
        return -1;
    }
    return index;
}

inline std::optional<Edge> generateEdge(ID id1, ID id2, std::unordered_map<ID, std::array<bool, 6>>& occupied)
{
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 11);

    int index1 = findFreeIndex(dist(rd) % 6, occupied[id1]);
    int index2 = findFreeIndex(dist(rd) % 6, occupied[id2]);

    occupied[id1][index1] = false;
    occupied[id2][index2] = false;

    if ((index1 < 0) || (index2 < 0))
    {
        return std::nullopt;
    }
    int ori = dist(rd) % 4;
    auto s1 = Side(index1 % 3 % 2);
    auto d1 = Dock(index1 / 3);
    auto s2 = Side(index2 % 3 % 2);
    auto d2 = Dock(index2 / 3);
    return Edge(id1, s1, d1, ori, d2, s2, id2);
}

inline std::optional<Configuration> generateAngles(const std::vector<ID>& ids, const std::vector<Edge>& edges)
{
    // Assume the edges form a connected graph and that edges contain only IDs from the 'ids' vector.
    std::random_device rd;
    std::uniform_int_distribution<int> ab(-1,1);
    std::uniform_int_distribution<int> c(-1, 2);
    double step = 90;
    Configuration cfg;
    for (ID id : ids)
    {
        double alpha = step * ab(rd);
        double beta = step * ab(rd);
        double gamma = step * c(rd);
        cfg.addModule(alpha,beta,gamma,id);
    }
    for (const Edge& edge : edges)
    {
        cfg.addEdge(edge);
    }

    // Try generating random angles to make the configuration valid.
    // After 1000 iterations give up.
    for (int i = 0; i < 1000; ++i)
    {
        if (cfg.isValid())
            return cfg;
        for (auto& [id, mod] : cfg.getModules())
        {
            double alpha = step * ab(rd);
            double beta = step * ab(rd);
            double gamma = step * c(rd);
            mod.setJoint(Joint::Alpha, alpha);
            mod.setJoint(Joint::Beta, beta);
            mod.setJoint(Joint::Gamma, gamma);
        }
    }
    // Giving up.
    return std::nullopt;
}

inline std::optional<Configuration> generateRandomTree(const std::vector<ID>& ids)
{
    std::default_random_engine e;
    std::random_device rd;
    std::uniform_int_distribution<unsigned long> dist(0, ids.size());


    std::vector<Edge> edges;
    std::vector<ID> shuffled = ids;
    std::shuffle(shuffled.begin(), shuffled.end(), e);
    std::unordered_map<ID, std::array<bool, 6>> occupied;
    occupied[shuffled[0]] = {true, true, true, true, true, true};

    for (unsigned i = 1; i < shuffled.size(); ++i)
    {
        ID id1 = shuffled[i];
        ID id2 = shuffled[dist(rd) % i];
        occupied[id1] = {true, true, true, true, true, true};
        std::optional<Edge> edgeOpt = generateEdge(id1, id2, occupied);
        if (!edgeOpt.has_value())
        {
            return std::nullopt;
        }

        edges.emplace_back(edgeOpt.value());
    }
    return generateAngles(ids, edges);
}

inline Configuration sampleFree(const std::vector<ID>& ids)
{
    auto treeOpt = generateRandomTree(ids);
    while (!treeOpt.has_value())
    {
        treeOpt = generateRandomTree(ids);
    }
    auto cfg = treeOpt.value();
    std::vector<Action::Reconnect> connect;
    cfg.generateConnections(connect);

    std::random_device rd;
    std::uniform_int_distribution<unsigned long> dist(0, 1);
    for (auto& action : connect)
    {
        if (dist(rd) == 1)
        {
            cfg.execute({{},{action}});
        }
    }

    return cfg;
}

inline const Configuration* getCfg(const std::unique_ptr<Configuration>& ptr)
{
    return ptr.get();
}

inline const Configuration* getCfg(const Configuration& cfg)
{
    return &cfg;
}


inline const Configuration* addToTree(ConfigPool& pool, ConfigEdges& edges, const Configuration* from, const Configuration& to)
{
    if (!pool.find(to))
    {
        auto nextPtr = pool.insert(to);
        edges[nextPtr] = {};
        //std::cout << p.print(next) << std::endl;

        edges[from].push_back(nextPtr);
        edges[nextPtr].push_back(from);
        return nextPtr;
    }
    else
    {
        return pool.get(to);
    }
}

template<typename T>
inline const Configuration* nearest(const Configuration& cfg, const T& pool, DistFunction* dist)
{
    const Configuration* near = nullptr;
    double dMin = 0;
    for (auto& next : pool)
    {
        auto nextPtr = getCfg(next);
        if (near == nullptr)
        {
            near = nextPtr;
            dMin = dist(cfg, *near);
            continue;
        }
        double d = dist(cfg, *nextPtr);
        if (d < dMin)
        {
            dMin = d;
            near = nextPtr;
        }
    }
    return near;
}

inline Configuration steer(const Configuration& from, const Configuration& to, DistFunction* dist)
{
    auto diff = from.diff(to);

    auto init = from.executeIfValid(diff);
    if (init.has_value())
        return init.value();

    auto allRot = diff.rotations();
    auto allRec = diff.reconnections();
    std::vector<std::vector<Action::Rotate>> subRot;
    std::vector<std::vector<Action::Reconnect>> subRec;
    getAllSubsetsLess(allRot, subRot, {}, 0, allRot.size());
    getAllSubsetsLess(allRec, subRec, {}, 0, allRec.size());

    double minDistance = dist(from, to);
    Configuration minCfg = from;

    for (auto& rot : subRot)
    {
        for (auto& rec : subRec)
        {
            auto valid = from.executeIfValid({rot, rec});
            if (valid.has_value())
            {
                double newDistance = dist(from, valid.value());
                if (newDistance < minDistance)
                {
                    minDistance = newDistance;
                    minCfg = valid.value();
                }
            }
        }
    }
    return minCfg;
}

inline Configuration steerRotate(const Configuration& from,const Configuration& to, const Action& diff)
{
    unsigned long count = diff.rotations.size();
    auto afterAction = from.executeIfValid({{diff.rotations}, {}});
    if (afterAction.has_value())
    {
        return afterAction.value();
    }
    return from;

    for (unsigned long i = count; i > 0; --i)
    {
        std::vector<std::vector<Action::Rotate>> subRot;
        getAllSubsetsLess(diff.rotations, subRot, {}, 0, count);
        for (auto& rot : subRot)
        {
            Action action = {rot, {}};
            auto afterAction = from.executeIfValid(action);
            if (afterAction.has_value())
            {
                return afterAction.value();
            }
        }
    }
    return from;
}

inline std::vector<Configuration> steerReconnect(const Configuration& from,const Configuration& to, const Action& diff, unsigned step)
{
    auto allRec = diff.reconnections;
    unsigned long count = allRec.size();
    for (unsigned long i = count; i > 0; --i)
    {
        std::vector<std::vector<Action::Reconnect>> subRec;
        getAllSubsetsLess(allRec, subRec, {}, 0, count);
        for (auto& rec : subRec)
        {
            Action action = {{}, rec};
            auto copy = from;
            copy.execute(action);
            std::vector<Edge> edges;
            auto ids = copy.getIDs();
            for (ID id : ids)
            {
                auto newEdges = copy.getEdges(id);
                edges.insert(edges.end(), newEdges.begin(), newEdges.end());
            }
            auto steered = generateAngles(ids, edges);
            if (!steered.has_value())
                continue;
            Action fromSteerDiff = from.diff(steered.value());
            Action fromSteerRot = {{fromSteerDiff.rotations}, {}};
            auto first = from.executeIfValid(fromSteerRot);
            if (!first.has_value())
                continue;
            Action fromSteerRec = {{}, {fromSteerDiff.reconnections}};
            auto second = first.value().executeIfValid(fromSteerDiff);
            if (!second.has_value())
                continue;
            return {first.value(), second.value()};
        }
    }
    return {from};
}

inline std::vector<Configuration> steerPath(const Configuration& from, const Configuration& to, unsigned step)
{
    auto fromToDiff = from.diff(to);
    auto afterDiff = from.executeIfValid(fromToDiff);
    if (afterDiff.has_value())
    {
        return {afterDiff.value()};
    }
    if (fromToDiff.reconnections.empty())
    {
        return {steerRotate(from, to, fromToDiff)};
    }
    return steerReconnect(from, to, fromToDiff, step);
}

inline std::optional<Configuration> steerEdge(const Configuration& from, const Configuration& to, unsigned step)
{
    auto diff = from.diff(to);
    std::vector<Edge> edges;
    auto ids = from.getIDs();
    for (auto& rec : diff.reconnections())
    {
        if (rec.add())
            edges.push_back(rec.edge());
    }
    for (auto id : ids)
    {
        for (auto& edge : from.getEdges(id))
        {
            if (edge.id1() == id)
            {
                edges.push_back(edge);
            }
        }
    }
    return generateAngles(ids, edges);
}

inline void extendEdge(ConfigPool& pool, ConfigEdges& edges, const Configuration& cfg, unsigned step)
{
    const Configuration* near = nearest(cfg, pool, Eval::matrixDiff);
    auto cfgEdge = steerEdge(*near, cfg, step);
    if (!cfgEdge.has_value())
        return;
    auto diff = near->diff(cfgEdge.value());
    auto rot = diff.rotations();
    auto rec = diff.reconnections();

    auto first = near->executeIfValid({rot,{}});
    if (first.has_value())
    {
        auto ptr = addToTree(pool, edges, near, first.value());
        addToTree(pool, edges, ptr, cfgEdge.value());
    }
}

inline void extendPath(ConfigPool& pool, ConfigEdges& edges, const Configuration& cfg, unsigned step, DistFunction* dist = Eval::matrixDiff)
{
    const Configuration* near = nearest(cfg, pool, dist);
    auto newPath = steerPath(*near, cfg, step);
    for (auto& newCfg : newPath)
    {
        near = addToTree(pool, edges, near, newCfg);
    }
}

/* RRT Algorithm
 *
 * */

inline void extend(ConfigPool& pool, ConfigEdges& edges, const Configuration& cfg)
{
    const Configuration* near = nearest(cfg, pool, Distance::reconnections);
    Configuration next = steer(*near, cfg, Distance::rotations);

    addToTree(pool, edges, near, next);
}

inline void extend2(ConfigPool& pool, ConfigEdges& edges, const Configuration& cfg, unsigned step)
{
    const Configuration* near = nearest(cfg, pool, Distance::reconnections);

    auto diff = near->diff(cfg);
    auto value = near->executeIfValid(diff);
    if (value.has_value())
    {
        addToTree(pool, edges, near, value.value());
        return;
    }

    auto cfgEdge = steerEdge(*near, cfg, step);
    if (cfgEdge.has_value())
    {
        auto diff2 = near->diff(cfgEdge.value());
        auto rot = diff2.rotations();

        auto first = near->executeIfValid({rot,{}});
        if (first.has_value())
        {
            auto ptr = addToTree(pool, edges, near, first.value());
            addToTree(pool, edges, ptr, cfgEdge.value());
        }
        return;
    }

    Configuration next = steer(*near, cfg, Distance::rotations);
    addToTree(pool, edges, near, next);
}

inline std::vector<Configuration> RRT(const Configuration& init, const Configuration& goal, unsigned step = 90)
{
    ConfigPool pool;
    ConfigEdges edges;

    auto initPtr = pool.insert(init);

    while(true)
    {
        Configuration rand = sampleFree(init.getIDs());
        //randomWalk(pool, edges, rand, step);
//        extendEdge(pool, edges, rand, step);
//        extend(pool, edges, rand);
//
//        extendEdge(pool, edges, goal, step);
//        extend(pool, edges, goal);

        extendPath(pool, edges, rand, step);
        extendPath(pool, edges, goal, step);

        if (pool.find(goal))
            break;
    }
    //std::cout << pool.size() << " " << edges.size() << std::endl;
    if (pool.find(goal))
    {
        const Configuration * goalPtr = pool.get(goal);
        return createPath(edges, initPtr, goalPtr);
    }
    return {};
}

#endif //ROBOTS_BFS_H
