#include "Algorithms.h"

std::vector<Configuration> AStar(const Configuration& init, 
    const Configuration& goal, unsigned step /*= 90*/, unsigned bound /*= 1*/, 
    EvalFunction& eval /*= Eval::trivial */, AlgorithmStat* stat /*= nullptr*/)
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
    unsigned long maxQSize = 0;

    queue.push( {goalDist[pointer], pointer} );

    while (!queue.empty())
    {

        maxQSize = std::max(maxQSize, queue.size());
        const auto [d, current] = queue.top();
        double currDist = initDist[current];
        queue.pop();


        std::vector<Configuration> nextCfgs;
        next(*current, nextCfgs, step, bound);
        for (const auto& next : nextCfgs)
        {
            const Configuration* pointerNext;
            double newDist = currDist + 1 + eval(next, goal);
            bool update = false;

            if (!pool.has(next))
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
                auto path = createPath(pred, pointerNext);
                if (stat != nullptr)
                {
                    stat->pathLength = path.size();
                    stat->queueSize = maxQSize;
                    stat->seenCfgs = pool.size();
                }

                return path;
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
