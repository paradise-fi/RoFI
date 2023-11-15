#include "legacy/configuration/Generators.h"
#include <queue>

using namespace rofi::configuration::matrices;

std::optional<Configuration> executeIfValid(const Configuration& config, const Action &action) {
    int steps = 10;
    Configuration next = config;

    std::vector<Action::Reconnect> connections;
    std::vector<Action::Reconnect> disconnections;
    for (const auto& res : action.reconnections()) {
        if (res.add())
            connections.push_back(res);
        else
            disconnections.push_back(res);
    }

    Action connect({}, connections);
    Action disconnect({}, disconnections);

    if (!next.execute(disconnect) || !next.connected() || !next.execute(connect))
        return std::nullopt;

    Action rotate(action.rotations(), {});
    Action divided = rotate.divide(1.0/steps);
    for (int i = 1; i <= steps; ++i) {
        if (!next.isValid() || !next.execute(divided))
            return std::nullopt;
    }
    if (!next.isValid())
        return std::nullopt;

    return next;
}

void generateActions(const Configuration& config, std::vector<Action>& res, unsigned step, unsigned bound /*=1*/) {
    // TODO maybe improve this
    std::vector<Action::Rotate> rotations;
    generateRotations(config, rotations, step);
    std::vector<Action::Reconnect> reconnections;
    generateReconnect(config, reconnections);

    std::vector<std::vector<Action::Rotate>> resRot;
    std::vector<std::vector<Action::Reconnect>> resRec;

    getAllSubsets(reconnections, resRec, bound);
    getAllSubsets(rotations, resRot, bound);

    for (auto& rotation : resRot) {
        for (auto& reconnection : resRec) {
            if (rotation.size() + reconnection.size() <= bound)
                res.emplace_back(rotation, reconnection);
        }
    }
}

void generateSimpleActions(const Configuration& config, std::vector<Action>& res, unsigned step) {
    std::vector<Action::Rotate> rotations;
    generateRotations(config, rotations, step);
    std::vector<Action::Reconnect> reconnections;
    generateReconnect(config, reconnections);

    for (auto& rotation : rotations)
        res.emplace_back(rotation);

    for (auto& reconnection : reconnections)
        res.emplace_back(reconnection);
}

void generateSimpleOnlyRotActions(const Configuration& config, std::vector<Action>& res, unsigned step) {
    std::vector<Action::Rotate> rotations;
    generateRotations(config, rotations, step);

    for (auto& rotation : rotations)
        res.emplace_back(rotation);
}

void generateBisimpleActions(const Configuration& config, std::vector<Action>& res, unsigned step) {
    std::vector<Action::Rotate> rotations;
    generateRotations(config, rotations, step);
    std::vector<Action::Reconnect> reconnections;
    generateReconnect(config, reconnections);

    for (auto it1 = rotations.begin(); it1 != rotations.end(); ++it1) {
        res.emplace_back(*it1);
        for (auto it2 = it1; it2 != rotations.end(); ++it2) {
            if (it1 == it2)
                continue;
            res.emplace_back(std::vector<Action::Rotate>{*it1, *it2}, std::vector<Action::Reconnect>{});
        }
    }
    for (auto& reconnection : reconnections)
        res.emplace_back(reconnection);
}

void generateBisimpleOnlyRotActions(const Configuration& config, std::vector<Action>& res, unsigned step) {
    std::vector<Action::Rotate> rotations;
    generateRotations(config, rotations, step);

    for (auto it1 = rotations.begin(); it1 != rotations.end(); ++it1) {
        res.emplace_back(*it1);
        for (auto it2 = it1; it2 != rotations.end(); ++it2) {
            if (it1 == it2)
                continue;
            res.emplace_back(std::vector<Action::Rotate>{*it1, *it2}, std::vector<Action::Reconnect>{});
        }
    }
}

void generateParalyzedActions(const Configuration& config, std::vector<Action>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices)
{
    std::vector<Action::Rotate> rotations;
    generateParalyzedRotations(config, rotations, step, allowed_indices);
    std::vector<Action::Reconnect> reconnections;
    generateParalyzedReconnect(config, reconnections, allowed_indices);

    for (auto& rotation : rotations)
        res.emplace_back(rotation);

    for (auto& reconnection : reconnections)
        res.emplace_back(reconnection);
}

void generateBiParalyzedOnlyRotActions(const Configuration& config, std::vector<Action>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices)
{
    std::vector<Action::Rotate> rotations;
    generateParalyzedRotations(config, rotations, step, allowed_indices);

    for (auto it1 = rotations.begin(); it1 != rotations.end(); ++it1) {
        res.emplace_back(*it1);
        for (auto it2 = it1; it2 != rotations.end(); ++it2) {
            if (it1 == it2)
                continue;
            res.emplace_back(std::vector<Action::Rotate>{*it1, *it2}, std::vector<Action::Reconnect>{});
        }
    }
}

void generateSmartParalyzedOnlyRotActions(const Configuration& config, std::vector<Action>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices)
{
    std::unordered_map<ID, std::vector<Action::Rotate>> rotations;
    generateMappedParalyzedRotations(config, rotations, step, allowed_indices);

    const auto& spannSucc = config.getSpanningSucc();
    for (const auto& [id, rots] : rotations) {
        for (auto it1 = rots.begin(); it1 != rots.end(); ++it1) {
            res.emplace_back(*it1);
            for (auto it2 = it1; it2 != rots.end(); ++it2) {
                if (it1 == it2)
                    continue;
                res.emplace_back(std::vector<Action::Rotate>{*it1, *it2}, std::vector<Action::Reconnect>{});
            }
            std::queue<ID> bag;
            for (const auto& optEdge : spannSucc.at(id)) {
                if (!optEdge.has_value())
                    continue;
                bag.emplace(optEdge.value().id2());
            }
            while (!bag.empty()) {
                ID currId = bag.front();
                bag.pop();
                if (rotations.find(currId) != rotations.end()) {
                    for (const auto& rot2 : rotations.at(currId))
                        res.emplace_back(std::vector<Action::Rotate>{*it1, rot2}, std::vector<Action::Reconnect>{});
                }
                for (const auto& optEdge : spannSucc.at(currId)) {
                    if (!optEdge.has_value())
                        continue;
                    bag.emplace(optEdge.value().id2());
                }
            }
        }
    }
}

void generateRotations(const Configuration& config, std::vector<Action::Rotate>& res, unsigned step) {
    for (auto [id, mod] : config.getModules()) {
        for (int d : {-step, step}) {
            for (Joint joint : {Alpha, Beta, Gamma}) {
                if (mod.rotateJoint(joint, d))
                    res.emplace_back(id, joint, d);
            }
        }
    }
}

void generateParalyzedRotations(const Configuration& config, std::vector<Action::Rotate>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices)
{
    for (auto [id, mod] : config.getModules()) {
        if (allowed_indices.find(id) == allowed_indices.end())
            continue;

        for (int d : {-step, step}) {
            for (Joint joint : {Alpha, Beta, Gamma}) {
                if (mod.rotateJoint(joint, d))
                    res.emplace_back(id, joint, d);
            }
        }
    }
}

void generateMappedParalyzedRotations(const Configuration& config, std::unordered_map<ID, std::vector<Action::Rotate>>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices)
{
    for (auto [id, mod] : config.getModules()) {
        if (allowed_indices.find(id) == allowed_indices.end())
            continue;

        for (int d : {-step, step}) {
            for (Joint joint : {Alpha, Beta, Gamma}) {
                if (mod.rotateJoint(joint, d)) {
                    res.try_emplace(id, std::vector<Action::Rotate>{});
                    res[id].emplace_back(id, joint, d);
                }
            }
        }
    }
}

void generateConnections(const Configuration& config, std::vector<Action::Reconnect>& res) {
    for (const auto& [id1, ms1] : config.getMatrices()) {
        for (const auto& [id2, ms2] : config.getMatrices()) {
            if (id1 >= id2)
                continue;

            Edge edge(id1, A, XPlus, 0, XPlus, A, id2);
            auto edgeOpt = nextEdge(edge);

            while (edgeOpt.has_value()) {
                Vector center1 = center(ms1[edge.side1()]);
                Vector center2 = center(ms2[edge.side2()]);
                if (distance(center1, center2) != 1) {
                    edge = edgeOpt.value();
                    edgeOpt = nextEdge(edge);
                    continue;
                }

                const Matrix& matrix = ms2[edge.side2()];
                if (equals(matrix, config.computeConnectedMatrix(edge)) && !config.findEdge(edge))
                    res.emplace_back(true, edge);

                edge = edgeOpt.value();
                edgeOpt = nextEdge(edge);
            }
        }
    }
}

void generateParalyzedConnections(const Configuration& config, std::vector<Action::Reconnect>& res,
    const std::unordered_set<ID>& allowed_indices)
{
    for (const auto& [id1, ms1] : config.getMatrices()) {
        if (allowed_indices.find(id1) == allowed_indices.end())
            continue;

        for (const auto& [id2, ms2] : config.getMatrices()) {
            if (id1 >= id2 && allowed_indices.find(id2) != allowed_indices.end())
                continue;

            Edge edge(id1, A, XPlus, 0, XPlus, A, id2);
            auto edgeOpt = nextEdge(edge);

            while (edgeOpt.has_value()) {
                Vector center1 = center(ms1[edge.side1()]);
                Vector center2 = center(ms2[edge.side2()]);
                if (distance(center1, center2) != 1) {
                    edge = edgeOpt.value();
                    edgeOpt = nextEdge(edge);
                    continue;
                }

                const Matrix& matrix = ms2[edge.side2()];
                if (equals(matrix, config.computeConnectedMatrix(edge)) && !config.findEdge(edge))
                    res.emplace_back(true, edge);

                edge = edgeOpt.value();
                edgeOpt = nextEdge(edge);
            }
        }
    }
}

void generateDisconnections(const Configuration& config, std::vector<Action::Reconnect>& res) {
    for (auto& [id, set] : config.getEdges()) {
        for (const std::optional<Edge>& edgeOpt : set) {
            if (!edgeOpt.has_value())
                continue;

            const Edge& edge = edgeOpt.value();
            if (edge.id1() < edge.id2())
                res.emplace_back(false, edge);
        }
    }
}

void generateParalyzedDisconnections(const Configuration& config, std::vector<Action::Reconnect>& res,
    const std::unordered_set<ID>& allowed_indices)
{
    for (auto& [id, set] : config.getEdges()) {
        if (allowed_indices.find(id) == allowed_indices.end())
            continue;

        for (const std::optional<Edge>& edgeOpt : set) {
            if (!edgeOpt.has_value())
                continue;

            const Edge& edge = edgeOpt.value();
            if (edge.id1() < edge.id2())
                res.emplace_back(false, edge);
        }
    }
}

void next(const Configuration& config, std::vector<Configuration>& res, unsigned step, unsigned bound /*=1*/) {
    std::vector<Action> actions;
    if (bound == 1) {
        generateSimpleActions(config, actions, step);
    } else {
        generateActions(config, actions, step, bound);
    }

    for (auto& action : actions) {
        auto cfgOpt = executeIfValid(config, action);
        if (cfgOpt.has_value())
            res.push_back(cfgOpt.value());
    }
}

void simpleNext(const Configuration& config, std::vector<Configuration>& res, unsigned step) {
    std::vector<Action> actions;
    generateSimpleActions(config, actions, step);
    for (auto& action : actions) {
        auto cfgOpt = executeIfValid(config, action);
        if (cfgOpt.has_value())
            res.push_back(cfgOpt.value());
    }
}

void simpleOnlyRotNext(const Configuration& config, std::vector<Configuration>& res, unsigned step) {
    std::vector<Action> actions;
    generateSimpleOnlyRotActions(config, actions, step);
    for (auto& action : actions) {
        auto cfgOpt = executeIfValid(config, action);
        if (cfgOpt.has_value())
            res.push_back(cfgOpt.value());
    }
}


void bisimpleNext(const Configuration& config, std::vector<Configuration>& res, unsigned step) {
    std::vector<Action> actions;
    generateBisimpleActions(config, actions, step);
    for (const auto& action : actions) {
        auto cfgOpt = executeIfValid(config, action);
        if (cfgOpt.has_value())
            res.push_back(cfgOpt.value());
    }
}

void bisimpleOnlyRotNext(const Configuration& config, std::vector<Configuration>& res, unsigned step) {
    std::vector<Action> actions;
    generateBisimpleOnlyRotActions(config, actions, step);
    for (const auto& action : actions) {
        auto cfgOpt = executeIfValid(config, action);
        if (cfgOpt.has_value())
            res.push_back(cfgOpt.value());
    }
}


void paralyzedNext(const Configuration& config, std::vector<Configuration>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices)
{
    std::vector<Action> actions;
    generateParalyzedActions(config, actions, step, allowed_indices);
    for (auto& action : actions) {
        auto cfgOpt = executeIfValid(config, action);
        if (cfgOpt.has_value())
            res.push_back(cfgOpt.value());
    }
}

void biParalyzedOnlyRotNext(const Configuration& config, std::vector<Configuration>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices)
{
    std::vector<Action> actions;
    generateBiParalyzedOnlyRotActions(config, actions, step, allowed_indices);
    for (auto& action : actions) {
        auto cfgOpt = executeIfValid(config, action);
        if (cfgOpt.has_value())
            res.push_back(cfgOpt.value());
    }
}

void smartBisimpleOnlyRotNext(const Configuration& config, std::vector<Configuration>& res, unsigned step)
{
    std::vector<Action> actions;
    auto ids = config.getIDs();
    std::unordered_set<ID> allowed_indices(ids.begin(), ids.end());
    generateSmartParalyzedOnlyRotActions(config, actions, step, allowed_indices);
    for (auto& action : actions) {
        auto cfgOpt = executeIfValid(config, action);
        if (cfgOpt.has_value())
            res.push_back(cfgOpt.value());
    }
}

void smartBisimpleParOnlyRotNext(const Configuration& config, std::vector<Configuration>& res, unsigned step,
    const std::unordered_set<ID>& allowed_indices)
{
    std::vector<Action> actions;
    generateSmartParalyzedOnlyRotActions(config, actions, step, allowed_indices);
    for (auto& action : actions) {
        auto cfgOpt = executeIfValid(config, action);
        if (cfgOpt.has_value())
            res.push_back(cfgOpt.value());
    }
}
