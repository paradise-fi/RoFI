#include "Algorithms.h"

using namespace rofi::configuration::matrices;

std::vector<Configuration> BFS(const Configuration& init, const Configuration& goal, 
    unsigned step /*= 90*/, unsigned bound /*= 1*/, AlgorithmStat* stat /*= nullptr*/)
{
    //Assume both configs are consistent and valid.
    ConfigPred pred;
    ConfigPool pool;

    unsigned long maxQSize = 0;

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
        maxQSize = std::max(maxQSize, queue.size());
        const auto current = queue.front();
        queue.pop();

        std::vector<Configuration> nextCfgs;
        next(*current, nextCfgs, step, bound);

        for (const auto& next : nextCfgs)
        {
            if (!pool.has(next))
            {
                const Configuration* pointerNext = pool.insert(next);
                pred.insert({pointerNext, current});

                if (next == goal)
                {
                    auto path = createPath(pred, pointerNext);
                    if (stat != nullptr)
                    {
                        stat->pathLength = path.size();
                        stat->queueSize = maxQSize;
                        stat->seenCfgs = pool.size();
                    }

                    return path;
                }
                queue.push(pointerNext);
            }

        }
    }
    if (stat != nullptr)
    {
        stat->pathLength = 0;
        stat->queueSize = maxQSize;
        stat->seenCfgs = pool.size();
    }
    return {};
}
