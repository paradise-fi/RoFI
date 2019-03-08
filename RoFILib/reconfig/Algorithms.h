//
// Created by xvozarov on 2/18/19.
//

#ifndef ROBOTS_BFS_H
#define ROBOTS_BFS_H

#include "../Configuration.h"
#include <queue>
#include <cmath>

const int step = 90;

using ConfigPool = std::unordered_set<Configuration, ConfigurationHash>;
using ConfigPred = std::unordered_map<const Configuration*, const Configuration*>;
using EvalFunction = double(const Configuration&, const Configuration&);
using EvalPair = std::pair<double, const Configuration*>;

struct EvalCompare
{
public:
    bool operator()(const EvalPair& a, const EvalPair& b)
    {
        return a.first > b.first;
    }
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
                result += std::abs(mod.getJoint(j) - other.getJoint(j));
            }
        }
        return result;
    }

    inline double centerDiff(const Configuration& curr, const Configuration& goal)
    {
        double result = 0;
        for ( auto& [id, mod] : curr.getModules() )
        {
            const Module& other = goal.getModules().at(id);
            for (Side s : {A, B})
            {
                result += distance(mod.getCenter(s), other.getCenter(s));
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

inline std::vector<Configuration> BFS(const Configuration& init, const Configuration& goal)
{
    //Assume both configs are consistent and valid.
    ConfigPred pred;
    ConfigPool pool;

    if (init == goal)
    {
        return {init};
    }

    auto [config, _] = pool.insert(init);
    const Configuration* pointer = &(*config);
    pred.insert({pointer, pointer});

    std::queue<const Configuration*> queue;
    queue.push(pointer);

    while (!queue.empty())
    {
        const auto current = queue.front();
        queue.pop();

        for (const auto& next : current->next(step))
        {
            if (pool.find(next) == pool.end())
            {
                auto [configNext, _] = pool.insert(next);
                const Configuration* pointerNext = &(*configNext);
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

inline std::vector<Configuration> AStar(const Configuration& init, const Configuration& goal, EvalFunction& eval = Eval::trivial)
{
    ConfigPred pred;
    ConfigPool pool;

    if (init == goal)
    {
        return {init};
    }

    auto [config, _] = pool.insert(init);
    const Configuration* pointer = &(*config);
    pred.insert({pointer, pointer});

    std::priority_queue<EvalPair, std::vector<EvalPair>, EvalCompare> queue;
    queue.push( {eval(init, goal), pointer} );

    while (!queue.empty())
    {
        const auto [val, current] = queue.top();
        queue.pop();

        //std::cout << "Fitness: " << val << std::endl;

        for (const auto& next : current->next(step))
        {
            if (pool.find(next) == pool.end())
            {
                auto [configNext, _] = pool.insert(next);
                const Configuration* pointerNext = &(*configNext);
                pred.insert({pointerNext, current});

                if (next == goal)
                {
                    return createPath(pred, pointerNext);
                }
                queue.push( {eval(next, goal),  pointerNext} );
            }

        }
    }
    return {};
}

#endif //ROBOTS_BFS_H
