//
// Created by xvozarov on 2/18/19.
//

#ifndef ROBOTS_BFS_H
#define ROBOTS_BFS_H

#include "../Configuration.h"
#include "../Printer.h"
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
        Printer p;
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
                result += distance(getCenter(ms[s]), getCenter(other[s]));
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
        return action.rotations.size() + action.reconnections.size() ;
    }
}

namespace Distance
{
    inline double hamming(const Configuration& curr, const Configuration& goal)
    {
        double result = 0;
        double costEdge = 10, costDeg = 1.0/90.0;
        for ( auto& [id, mod] : curr.getModules() )
        {
            const auto& other = goal.getModules().at(id);
            for (Joint j : {Alpha, Beta, Gamma})
            {
                result += std::abs(mod.getJoint(j) - other.getJoint(j));
            }
        }
        result *= costDeg;

        for (auto& [id, edges] : curr.getEdges())
        {
            for (auto& opt : edges)
            {
                if (opt.has_value())
                {
                    if (!goal.findEdge(opt.value()))
                    {
                        result += costEdge;
                    }
                }
            }
        }

        for (auto& [id, edges] : goal.getEdges())
        {
            for (auto& opt : edges)
            {
                if (opt.has_value())
                {
                    if (!curr.findEdge(opt.value()))
                    {
                        result += costEdge;
                    }
                }
            }
        }
        return result;
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

        auto nextCfgs = current->next(step, bound);
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

        for (const auto& next : current->next(step, bound))
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
    std::uniform_int_distribution<int> c(0, 3);
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
    std::random_device rd;
    unsigned long count = 10 * ids.size();
    for (unsigned i = 0; i < count; ++i)
    {
        auto next = cfg.next(90, 1, true);
        if (next.empty())
        {
            next = cfg.next(90);
        }
        if (next.empty())
            break;

        std::uniform_int_distribution<unsigned long> dist(0, next.size() - 1);
        cfg = next[dist(rd)];
    }

    return cfg;
}

const Configuration* getCfg(const std::unique_ptr<Configuration>& ptr)
{
    return ptr.get();
}

const Configuration* getCfg(const Configuration& cfg)
{
    return &cfg;
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

inline Configuration steer(const Configuration& from, const Configuration& to, unsigned step = 90)
{
    return *nearest(to, from.next(step), Distance::hamming);
}

/* RRT Algorithm
 *
 * */

inline std::vector<Configuration> RRT(const Configuration& init, const Configuration& goal, unsigned step = 90)
{
    Printer p;
    ConfigPool pool;
    ConfigEdges edges;

    auto initPtr = pool.insert(init);

    for (int i = 0; i < 5000; ++i)
    {
        Configuration rand = sampleFree(init.getIDs());
        //Configuration rand = goal;
        const Configuration* near = nearest(rand, pool, Distance::hamming);
        Configuration next = steer(*near, rand, step);

        auto nextPtr = near;
        if (!pool.find(next))
        {
            nextPtr = pool.insert(next);
            edges[nextPtr] = {};
            std::cout << p.print(next);
        }

        edges[near].push_back(nextPtr);
        edges[nextPtr].push_back(near);
    }
    std::cout << pool.size() << " " << edges.size() << std::endl;
    const Configuration * goalPtr = nullptr;
    for (auto& cfg : pool)
    {
        if (*cfg == goal)
        {
            goalPtr = cfg.get();
        }
    }
    if (goalPtr == nullptr)
        return {};

    return createPath(edges, initPtr, goalPtr);
}

#endif //ROBOTS_BFS_H
