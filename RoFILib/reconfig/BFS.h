//
// Created by xvozarov on 2/18/19.
//

#ifndef ROBOTS_BFS_H
#define ROBOTS_BFS_H

#include "../Configuration.h"
#include <queue>

const int step = 90;

using ConfigPool = std::unordered_set<Configuration, ConfigurationHash>;
using ConfigPred = std::unordered_map<const Configuration*, const Configuration*>;

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
    Printer printer;
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
                //std::cout << printer.print(next) << std::endl;
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

#endif //ROBOTS_BFS_H
