//
// Created by xvozarov on 2/18/19.
//

#ifndef ROBOTS_BFS_H
#define ROBOTS_BFS_H

#include <Configuration.h>
#include <Generators.h>
#include <IO.h>
#include <queue>
#include <memory>

struct AlgorithmStat {
    unsigned long pathLength = 0;
    unsigned long queueSize = 0;
    unsigned long seenCfgs = 0;

    std::string toString() const {
        std::stringstream out;
        out << std::setw(8) << std::left << "length " << pathLength << std::endl;
        out << std::setw(8) << std::left << "queue " << queueSize << std::endl;
        out << std::setw(8) << std::left << "cfgs " << seenCfgs << std::endl;
        return out.str();
    }
};

class ConfigurationPtrHash {
public:
    std::size_t operator()(const std::unique_ptr<Configuration>& ptr) const {
        return ConfigurationHash{}(*ptr);
    }
};

inline bool operator==(const std::unique_ptr<Configuration>& a, const std::unique_ptr<Configuration>& b) {
    return (*a) == (*b);
}

using ConfigPred = std::unordered_map<const Configuration*, const Configuration*>;
using ConfigEdges = std::unordered_map<const Configuration*, std::vector<const Configuration*>>;
using ConfigValue = std::unordered_map<const Configuration*, double>;
using EvalFunction = double(const Configuration&, const Configuration&);
using DistFunction = double(const Configuration&, const Configuration&);
using EvalPair = std::tuple<double, const Configuration*>;

struct EvalCompare {
public:
    bool operator()(const EvalPair& a, const EvalPair& b) {
        auto& [a1, _a] = a;
        auto& [b1, _b] = b;
        return a1 > b1;
    }
};

class ConfigPool {
public:
    typedef std::unordered_set<std::unique_ptr<Configuration>, ConfigurationPtrHash>::iterator iterator;
    typedef std::unordered_set<std::unique_ptr<Configuration>, ConfigurationPtrHash>::const_iterator const_iterator;

    iterator begin() { return pool.begin(); }
    const_iterator begin() const { return pool.begin(); }
    iterator end() { return pool.end(); }
    const_iterator end() const { return pool.end(); }

    size_t size() const {
        return pool.size();
    }

    Configuration* insert(const Configuration& config) {
        auto tmp = std::make_unique<Configuration>(config);
        auto [ptr, _] = pool.insert(std::move(tmp));
        return ptr->get();
    }

    bool has(const Configuration& config) const {
        auto tmp = std::make_unique<Configuration>(config);
        return (pool.find(tmp) != pool.end());
    }

    const Configuration* get(const Configuration& config) const {
        auto tmp = std::make_unique<Configuration>(config);
        return pool.find(tmp)->get();
    }

private:
    std::unordered_set<std::unique_ptr<Configuration>, ConfigurationPtrHash> pool;
};


namespace Eval {
    inline double trivial(const Configuration& conf, const Configuration& goal) {
        return 1;
    }

    inline double jointDiff(const Configuration& curr, const Configuration& goal) {
        double result = 0;
        for(auto& [id, mod] : curr.getModules()) {
            const Module& other = goal.getModules().at(id);
            for (Joint j : {Alpha, Beta, Gamma}) {
                double diff = mod.getJoint(j) - other.getJoint(j);
                result += std::abs(diff) / 90;
            }
        }
        return result;
    }

    inline double centerDiff(const Configuration& curr, const Configuration& goal) {
        double result = 0;
        for (auto& [id, ms] : curr.getMatrices()) {
            const auto& other = goal.getMatrices().at(id);
            for (ShoeId s : {A, B}) {
                result += distance(center(ms[s]), center(other[s]));
            }
        }
        return result;
    }

    inline double matrixDiff(const Configuration& curr, const Configuration& goal) {
        double result = 0;
        for (auto& [id, ms] : curr.getMatrices()) {
            const auto& other = goal.getMatrices().at(id);
            for (ShoeId s : {A, B}) {
                result += distance(ms[s], other[s]);
            }
        }
        return result;
    }

    inline double actionDiff(const Configuration& curr, const Configuration& goal) {
        Action action = curr.diff(goal);
        return action.rotations().size() + action.reconnections().size() ;
    }
}


inline std::vector<Configuration> createPath(ConfigPred& pred, const Configuration* goal) {
    std::vector<Configuration> res;
    const Configuration* current = goal;

    while (current != pred.at(current)) {
        res.push_back(*current);
        current = pred.at(current);
    }
    res.push_back(*current);

    std::reverse(res.begin(), res.end());
    return res;
}

inline std::vector<Configuration> createPath(ConfigEdges& edges, const Configuration* init, const Configuration* goal) {
    ConfigPred pred;
    std::unordered_set<const Configuration*> seen;
    if (init == goal)
        return {*init};

    seen.insert(init);
    pred.insert({init, init});

    std::queue<const Configuration*> queue;
    queue.push(init);

    while (!queue.empty()) {
        const auto curr = queue.front();
        queue.pop();

        for (const auto* next : edges[curr]) {
            if (seen.find(next) == seen.end()) {
                seen.insert(next);
                pred.insert({next, curr});

                if (next == goal)
                    return createPath(pred, goal);

                queue.push(next);
            }
        }
    }

    return {};
}

std::vector<Configuration> BFS(const Configuration& init, 
    const Configuration& goal, unsigned step = 90, 
    unsigned bound = 1, AlgorithmStat* stat = nullptr);

std::vector<Configuration> AStar(const Configuration& init, 
    const Configuration& goal, unsigned step = 90, 
    unsigned bound = 1, EvalFunction& eval = Eval::trivial, AlgorithmStat* stat = nullptr);

std::vector<Configuration> RRT(const Configuration& init, 
    const Configuration& goal, unsigned step = 90, AlgorithmStat* stat = nullptr);

#endif //ROBOTS_BFS_H
