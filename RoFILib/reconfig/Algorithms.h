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
using EvalFunction = double(const Configuration&, const Configuration&);
using DistFunction = double(const Configuration&, const Configuration&);
using EvalPair = std::tuple<double, double, const Configuration*>;

struct EvalCompare
{
public:
    bool operator()(const EvalPair& a, const EvalPair& b)
    {
        auto& [a1, a2, _a] = a;
        auto& [b1, b2, _b] = b;
        return a1 + a2 > b1 + b2;
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
}

namespace Distance
{
    inline double matrixDist(const Configuration& curr, const Configuration& goal)
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
    return *nearest(to, from.next(step), Distance::matrixDist);
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

    if (init == goal)
    {
        return {init};
    }

    const Configuration* pointer = pool.insert(init);
    pred.insert({pointer, pointer});

    std::priority_queue<EvalPair, std::vector<EvalPair>, EvalCompare> queue;
    queue.push( {0, eval(init, goal), pointer} );

    while (!queue.empty())
    {
        const auto [d, val, current] = queue.top();
        queue.pop();

        // std::cout << "Fitness: " << val << std::endl;

        for (const auto& next : current->next(step, bound))
        {
            const Configuration* pointerNext = pool.insert(next);
            pred.insert({pointerNext, current});

            if (next == goal)
            {
                return createPath(pred, pointerNext);
            }
            queue.push( {d + 1, eval(next, goal),  pointerNext} );
        }
    }
    return {};
}

inline std::vector<Configuration> RRT(const Configuration& init, const Configuration& goal, unsigned step = 90)
{
    Printer p;
    ConfigPool pool;
    ConfigEdges edges;

    auto initPtr = pool.insert(init);

    for (int i = 0; i < 10000; ++i)
    {
        auto rand = sampleFree(init.getIDs());
        while (!rand.has_value())
        {
            rand = sampleFree(init.getIDs());
        }
        auto near = nearest(rand.value(), pool, Distance::matrixDist);
        auto next = steer(*near, *rand, step);

        auto nextPtr = near;
        if (!pool.find(next))
        {
            nextPtr = pool.insert(next);
            edges[nextPtr] = {};
            //std::cout << p.print(next);
        }

        edges[near].push_back(nextPtr);
        edges[nextPtr].push_back(near);
    }
    //std::cout << pool.size() << " " << edges.size() << std::endl;
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
