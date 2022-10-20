#include "Configuration.h"
#include <optional>
#include <unordered_set>
#include <vector>

#ifndef ROBOTS_GENERATORS_H
#define ROBOTS_GENERATORS_H

std::optional<Configuration> executeIfValid(const Configuration& config, const Action &action);

/**
 * \brief Generates all possible actions.
 *
 * Fills \p res with all possible valid actions that contains up to \p bound rotations, connections
 * and disconnections in sum. Only rotations with angle + \p step and - \p step are allowed.
 */
void generateActions(const Configuration& config, std::vector<Action>& res, unsigned step, unsigned bound=1);

/**
 * \brief Generates all possible "simple" actions.
 *
 * Simple action is an action that contains just a single rotation, connection and disconnection.
 * Only rotations with angle + \p step and - \p step are allowed.
 *
 * Fills \p res with all possible valid simple actions
 */
void generateSimpleActions(const Configuration& config, std::vector<Action>& res, unsigned step);

void generateSimpleOnlyRotActions(const Configuration& config, std::vector<Action>& res, unsigned step);

/**
 * \brief Generates all possible "bisimple" actions.
 *
 * Bisimple action is an action that contains up to two rotations, or just a connection or a disconnection.
 * Only rotations with angle + \p step and - \p step are allowed.
 *
 * Fills \p res with all possible valid bisimple actions
 */
void generateBisimpleActions(const Configuration& config, std::vector<Action>& res, unsigned step);

void generateBisimpleOnlyRotActions(const Configuration& config, std::vector<Action>& res, unsigned step);

void generateParalyzedActions(const Configuration& config, std::vector<Action>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices);

void generateBiParalyzedOnlyRotAction(const Configuration& config, std::vector<Action>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices);

void generateSmartParalyzedOnlyRotActions(const Configuration& config, std::vector<Action>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices);

void generateRotations(const Configuration& config, std::vector<Action::Rotate>& res, unsigned step);

void generateParalyzedRotations(const Configuration& config, std::vector<Action::Rotate>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices);

void generateMappedParalyzedRotations(const Configuration& config, std::unordered_map<ID, std::vector<Action::Rotate>>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices);

void generateConnections(const Configuration& config, std::vector<Action::Reconnect>& res);

void generateParalyzedConnections(const Configuration& config, std::vector<Action::Reconnect>& res,
    const std::unordered_set<ID>& allowed_indices);

void generateDisconnections(const Configuration& config, std::vector<Action::Reconnect>& res);

void generateParalyzedDisconnections(const Configuration& config, std::vector<Action::Reconnect>& res,
    const std::unordered_set<ID>& allowed_indices);

inline void generateReconnect(const Configuration& config, std::vector<Action::Reconnect>& res) {
    generateConnections(config, res);
    generateDisconnections(config, res);
}

inline void generateParalyzedReconnect(const Configuration& config, std::vector<Action::Reconnect>& res,
    const std::unordered_set<ID>& allowed_indices)
{
    generateParalyzedConnections(config, res, allowed_indices);
    generateParalyzedDisconnections(config, res, allowed_indices);
}

/**
 * \brief Generates all possible configurations that \p config can become using one action.
 *
 * Fills \p res with all possible configurations that \p config can change into using one action
 * that contains up to \p bound rotations, connections and disconnections in sum. Only rotations
 * with angle + \p step and - \p step are allowed.
 *
 * If \p bound is equal to 1, it uses `generateSimpleActions` instead of `generateActions`.
 */
void next(const Configuration& config, std::vector<Configuration>& res, unsigned step, unsigned bound=1);

/**
 * \brief Generates all possible configurations that \p config can become using one simple action.
 *
 * Fills \p res with all possible configurations that \p config can change into using one simple action
 * Only rotations with angle + \p step and - \p step are allowed.
 */
void simpleNext(const Configuration& config, std::vector<Configuration>& res, unsigned step);

void simpleOnlyRotNext(const Configuration& config, std::vector<Configuration>& res, unsigned step);

/**
 * \brief Generates all possible configurations that \p config can become using one recconect action or
 * up to two simple rotations.
 *
 * Fills \p res with all possible configurations that \p config can change into using one recconect
 * action or up to two simple rotations.
 * Only rotations with angle + \p step and - \p step are allowed.
 */
void bisimpleNext(const Configuration& config, std::vector<Configuration>& res, unsigned step);

void bisimpleOnlyRotNext(const Configuration& config, std::vector<Configuration>& res, unsigned step);

/**
 * \brief Generates all possible configurations that \p config can become using one simple action that
 * can use only modules from \p allowed_indices.
 *
 * Fills \p res with all possible configurations that \p config can change into using one simple action.
 * Only rotations of modules with id in \p allowed_indices are allowed. Only connections adn disconnections,
 * where at least one of the modules has its ID in \p allowed_indices are allowed.
 * Only rotations with angle + \p step and - \p step are allowed.
 */
void paralyzedNext(const Configuration& config, std::vector<Configuration>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices);

void biParalyzedOnlyRotNext(const Configuration& config, std::vector<Configuration>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices);

void smartBisimpleOnlyRotNext(const Configuration& config, std::vector<Configuration>& res, unsigned step);

void smartBisimpleParOnlyRotNext(const Configuration& config, std::vector<Configuration>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices);

template<typename T>
inline void getAllSubsetsRec(const std::vector<T>& set, std::vector<std::vector<T>>& res,
    const std::vector<T>& accum, unsigned index, size_t count)
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
inline void getAllSubsetsRec(const Rots& set, std::vector<Rots>& res, const Rots& accum, unsigned index, size_t count) {
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
void getAllSubsets(const std::vector<T>& from, std::vector<std::vector<T>>& res, size_t count) {
    getAllSubsetsRec<T>(from, res, {}, 0, count);
}

 #endif //ROBOTS_GENERATORS_H
