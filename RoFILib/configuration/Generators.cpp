#include "Generators.h"

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

void generateParalyzedActions(const Configuration& config, std::vector<Action>& res, unsigned step, 
    const std::unordered_set<unsigned>& allowed_indices) 
{
    // TODO: improve reconnect to a*n from n*n
    std::vector<Action::Rotate> rotations;
    generateParalyzedRotations(config, rotations, step, allowed_indices);
    std::vector<Action::Reconnect> reconnections;
    generateReconnect(config, reconnections);

    for (auto& rotation : rotations)
        res.emplace_back(rotation);

    for (auto& reconnection : reconnections)
        res.emplace_back(reconnection);
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
    const std::unordered_set<unsigned>& allowed_indices) 
{
    for (const auto& id : allowed_indices) {
        auto it = config.getModules().find(id);
        if (it == config.getModules().end())
            continue;

        Module mod = it->second;
        for (int d : {-step, step}) {
            for (Joint joint : {Alpha, Beta, Gamma}) {
                if (mod.rotateJoint(joint, d))
                    res.emplace_back(id, joint, d);
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

void paralyzedNext(const Configuration& config, std::vector<Configuration>& res, unsigned step, 
    const std::unordered_set<unsigned>& allowed_indices) 
{
    std::vector<Action> actions;
    generateParalyzedActions(config, actions, step, allowed_indices);
    for (auto& action : actions) {
        auto cfgOpt = executeIfValid(config, action);
        if (cfgOpt.has_value())
            res.push_back(cfgOpt.value());
    }
}