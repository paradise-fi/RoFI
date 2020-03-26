#include <Configuration.h>
#include <optional>
#include <unordered_set>
#include <vector>

#ifndef ROBOTS_GENERATORS_H
#define ROBOTS_GENERATORS_H

    std::optional<Configuration> executeIfValid(const Configuration& config, const Action &action);

    void generateActions(const Configuration& config, std::vector<Action>& res, unsigned step, unsigned bound=1);

    void generateSimpleActions(const Configuration& config, std::vector<Action>& res, unsigned step);

    void generateParalyzedActions(const Configuration& config, std::vector<Action>& res, unsigned step, 
        const std::unordered_set<unsigned>& allowed_indices);

    void generateConnections(const Configuration& config, std::vector<Action::Reconnect>& res);

    void generateRotations(const Configuration& config, std::vector<Action::Rotate>& res, unsigned step);

    void generateParalyzedRotations(const Configuration& config, std::vector<Action::Rotate>& res, unsigned step, 
        const std::unordered_set<unsigned>& allowed_indices);

    void generateDisconnections(const Configuration& config, std::vector<Action::Reconnect>& res);

    inline void generateReconnect(const Configuration& config, std::vector<Action::Reconnect>& res) {
        generateConnections(config, res);
        generateDisconnections(config, res);
    }

    void next(const Configuration& config, std::vector<Configuration>& res, unsigned step, unsigned bound=1);

    void simpleNext(const Configuration& config, std::vector<Configuration>& res, unsigned step);

    void paralyzedNext(const Configuration& config, std::vector<Configuration>& res, unsigned step, 
        const std::unordered_set<unsigned>& allowed_indices);

    template<typename T>
    inline void getAllSubsetsRec(const std::vector<T>& set, std::vector<std::vector<T>>& res, 
        const std::vector<T>& accum, unsigned index, unsigned count) 
    {
        if ((accum.size() == count) || (index >= set.size())) {
            res.push_back(accum);
            return;
        }
        auto next = accum;
        next.push_back(set[index]);
        getAllSubsetsRec(set, res, next, index + 1, count);
        getAllSubsetsRec(set, res, accum, index + 1, count);
    }

    using Rots = std::vector<Action::Rotate>;
    template<>
    inline void getAllSubsetsRec(const Rots& set, std::vector<Rots>& res, const Rots& accum, unsigned index, unsigned count) {
        if ((accum.size() == count) || (index >= set.size())) {
            res.push_back(accum);
            return;
        }

        auto act = set[index];
        bool ok = true;
        for (auto& act2 : accum) {
            if ((act.id() == act2.id()) && (act.joint() == act2.joint()))
                ok = false;
        }
        if (ok) {
            auto next = accum;
            next.push_back(set[index]);
            getAllSubsetsRec(set, res, next, index + 1, count);
        }
        getAllSubsetsRec(set, res, accum, index + 1, count);
    }

    template<typename T>
    void getAllSubsets(const std::vector<T>& from, std::vector<std::vector<T>>& res, unsigned count) {
        getAllSubsetsRec<T>(from, res, {}, 0, count);
    }

 #endif //ROBOTS_GENERATORS_H