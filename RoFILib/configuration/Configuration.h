//
// Created by xvozarov on 2/18/19.
//

#ifndef ROBOTS_CONFIGURATION_H
#define ROBOTS_CONFIGURATION_H

#include "Matrix.h"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <array>
#include <optional>
#include <cmath>
#include <algorithm>
#include <random>
#include <set>

using ID = unsigned int;
const double threshold = 0.0001;

class Configuration;
class ConfigurationHash;

/* MODULE
 *
 * Module is an object with ID and three joints alpha, beta, gamma.
 * The joint values are given in degrees and are in range:
 *   alpha, beta: <-90, 90> (range is limited)
 *   gamma: <-180, 179> (range is unlimited, but the value is kept in limits)
 *
 * The joint values can be either rotated by or set to a specified value.
 * */

enum Joint { Alpha, Beta, Gamma };

enum Value { True, False, Unknown };

class Module
{
public:
    Module(double alpha, double beta, double gamma, ID id) :
            alpha(alpha), beta(beta), gamma(gamma), id(id) {}

    ID getId() const { return id; }

    double getJoint(Joint a) const
    {
        switch (a)
        {
            case Joint::Alpha:
                return alpha;
            case Joint::Beta:
                return beta;
            case Joint::Gamma:
                return gamma;
        }
        return 0;
    }

    bool rotateJoint(Joint joint, double val)
    {
        double res = 0;
        switch (joint)
        {
            case Joint::Alpha:
                res = alpha + val;
                break;
            case Joint::Beta:
                res = beta + val;
                break;
            case Joint::Gamma:
                res = gamma + val;
        }
        return setJoint(joint, res);
    }

    bool setJoint(Joint joint, double val)
    {
        switch (joint)
        {
            case Joint::Alpha:
                if ((val > 90) || (val < -90))
                {
                    return false;
                }
                alpha = val;
                break;
            case Joint::Beta:
                if ((val > 90) || (val < -90))
                {
                    return false;
                }
                beta = val;
                break;
            case Joint::Gamma:
                gamma = val;
                if (gamma > 180)
                {
                    gamma -= 360;
                }
                if (gamma <= -180)
                {
                    gamma += 360;
                }
        }
        return true;
    }

    bool operator==(const Module& other) const
    {
        return (id == other.id) &&
               (std::abs(alpha - other.alpha) < threshold) &&
               (std::abs(beta - other.beta) < threshold) &&
               (std::abs(gamma - other.gamma) < threshold);
    }

private:
    ID id;
    double alpha, beta, gamma;
    friend Configuration;
    friend ConfigurationHash;
};

/* EDGE
 *
 * Edge is an object with seven parameters specifying connectors and orientation.
 * Each connector is defined by module ID, side and dock.
 *
 * The edge is immutable and has only getters.
 *
 * (Flag onCoeff is used for visualization purposes.)
 * */

enum Side { A, B };

enum Dock { Xp, Xn, Zn };

class Edge
{
public:
    Edge(ID id1, Side side1, Dock dock1, unsigned int ori, Dock dock2, Side side2, ID id2, double onCoeff = 1) :
            id1_(id1), id2_(id2), side1_(side1), side2_(side2), dock1_(dock1), dock2_(dock2), ori_(ori), onCoeff_(onCoeff) {}

    ID id1() const { return id1_; }
    ID id2() const { return id2_; }
    Side side1() const { return side1_; }
    Side side2() const { return side2_; }
    Dock dock1() const { return dock1_; }
    Dock dock2() const { return dock2_; }
    unsigned int ori() const { return ori_; }

    double onCoeff() const { return onCoeff_; }
    void setOnCoeff(double value)
    {
        onCoeff_ = value;
    }

    bool operator==(const Edge& other) const
    {
        return (id1_ == other.id1_) &&
               (id2_ == other.id2_) &&
               (side1_ == other.side1_) &&
               (side2_ == other.side2_) &&
               (dock1_ == other.dock1_) &&
               (dock2_ == other.dock2_) &&
               (ori_ == other.ori_);
    }

    bool operator!=(const Edge& other) const {
        return !(*this == other);
    }

private:
    ID id1_, id2_;
    Side side1_, side2_;
    Dock dock1_, dock2_;
    unsigned int ori_;
    double onCoeff_ = 1;     //for visualizer
};

inline Edge reverse(const Edge& edge)
{
    return {edge.id2(), edge.side2(), edge.dock2(), edge.ori(), edge.dock1(), edge.side1(), edge.id1(), edge.onCoeff()};
}

inline std::array<unsigned, 5> nextEdgeArray(const std::array<unsigned, 5> &edge)
{
    const static std::array<unsigned, 5> size = {2,3,4,3,2};
    std::array<unsigned, 5> res = edge;
    for (unsigned i = 0; i < 5; ++i)
    {
        if (res[i] + 1 >= size[i])
        {
            res[i] = 0;
            continue;
        }
        res[i] += 1;
        break;
    }
    return res;
}

inline std::optional<Edge> nextEdge(const Edge& edge)
{
    std::array<unsigned, 5> arr = {edge.side1(), edge.dock1(), edge.ori(), edge.dock2(), edge.side2()};
    std::array<unsigned, 5> init = {0, 0, 0, 0, 0};
    auto next = nextEdgeArray(arr);
    if (next == init)
        return std::nullopt;
    return Edge(edge.id1(),
                static_cast<Side>(next[0]),
                static_cast<Dock>(next[1]),
                next[2],
                static_cast<Dock>(next[3]),
                static_cast<Side>(next[4]),
                edge.id2());
}

/* ACTIONS
 *
 * Action is an object representing a set of atomic actions occurring in parallel.
 * Atomic actions are rotations and reconnections (connect -- disconnect).
 *
 * Function divide creates a new action with smaller rotations, used for simulating
 * the continuous movement in discrete steps.
 *
 * */

class Action
{
public:
    class Rotate
    {
    public:
        Rotate() : _id(0), _joint(Alpha), _angle(0) {};
        Rotate(ID id, Joint joint, double angle) :
                _id(id), _joint(joint), _angle(angle) {}

        ID id() const { return _id; }
        Joint joint() const { return _joint; }
        double angle() const { return _angle; }

        bool operator==(const Rotate &other) const {
            return _id == other._id &&
                   _joint == other._joint &&
                   std::abs(_angle - other._angle) < threshold;
        }

        bool operator!=(const Rotate &other) const {
            return !(*this == other);
        }
    private:
        ID _id;
        Joint _joint;
        double _angle;
        friend Action;
    };

    class Reconnect
    {
    public:

        Reconnect() : _add(false), _edge({0,A,Zn,0,Zn,A,0}) {};
        Reconnect(bool add, const Edge& edge) :
                _add(add), _edge(edge) {}

        bool add() const { return _add; }
        Edge edge() const { return _edge; }

        bool operator==(const Reconnect &other) const {
            return _add == other._add &&
                   _edge == other._edge;
        }

        bool operator!=(const Reconnect &other) const {
            return !(*this == other);
        }

    private:
        bool _add;
        Edge _edge;
        friend Action;
    };

    Action(std::vector<Rotate> rot, std::vector<Reconnect> rec) :
            _rotations(std::move(rot)), _reconnections(std::move(rec)) {}

    Action divide(double factor) const
    {
        std::vector<Rotate> division;
        for (const Action::Rotate& rotate : _rotations)
        {
            division.emplace_back(rotate._id, rotate._joint, rotate._angle * factor);
        }
        return {division, _reconnections};
    }

    std::vector<Rotate> rotations() const {
        return _rotations;
    }

    std::vector<Reconnect> reconnections() const {
        return _reconnections;
    }

    bool empty() const {
        return _reconnections.empty() && _rotations.empty();
    }

private:
    std::vector<Rotate> _rotations;
    std::vector<Reconnect> _reconnections;

};

inline bool unique(const std::vector<Action::Rotate>& a)
{
    std::unordered_map<ID, std::array<bool, 3>> moved;
    for (const auto& rot : a)
    {
        if (moved.find(rot.id()) == moved.end())
        {
            moved[rot.id()] = {false, false, false};
            moved[rot.id()][rot.joint()] = true;
        }
        else
        {
            if (moved[rot.id()][rot.joint()])
            {
                return false;
            }
        }
    }
    return true;
}

template<typename T>
void filter(const std::vector<T>& data, std::vector<T>& res, bool (*pred)(const T&))
{
    for (const T& datum : data)
    {
        if ((*pred)(datum))
        {
            res.push_back(datum);
        }
    }
}

template<typename T>
inline void getAllSubsetsRec(const std::vector<T>& set, std::vector<std::vector<T>>& res, std::vector<T> accum, unsigned index, unsigned count)
{
    if ((accum.size() == count) || (index >= set.size()))
    {
        res.push_back(accum);
        return;
    }
    auto next = accum;
    next.push_back(set[index]);
    getAllSubsetsRec(set, res, next, index + 1, count);
    getAllSubsetsRec(set, res, accum, index + 1, count);
}


template<typename T>
inline void getAllSubsets(const std::vector<T>& from, std::vector<std::vector<T>>& res, unsigned count)
{
    getAllSubsetsRec(from, res, {}, 0, count);
}

using Rots = std::vector<Action::Rotate>;
inline void getAllRotsRec(const Rots& set, std::vector<Rots>& res, const Rots& accum, unsigned count, unsigned index)
{
    if ((accum.size() == count) || (index >= set.size()))
    {
        res.push_back(accum);
        return;
    }
    auto act = set[index];
    bool ok = true;
    for (auto& act2 : accum)
    {
        if ((act.id() == act2.id()) && (act.joint() == act2.joint()))
            ok = false;
    }
    if (ok)
    {
        auto next = accum;
        next.push_back(set[index]);
        getAllRotsRec(set, res, next, count, index + 1);
    }
    getAllRotsRec(set, res, accum, count, index + 1);
}

inline void getAllRots(const Rots& from, std::vector<Rots>& res, unsigned count)
{
    getAllRotsRec(from, res, {}, count, 0);
}

/* TRANSFORMATION MATRICES
 *
 * Functions transformJoint and transformConnection define the relative positions
 * of module sides and of connected modules. The relative positions are defined in
 * the thesis of Honza Mrazek.
 *
 * */

inline Matrix transformJoint(double alpha, double beta, double gamma)
{
    return rotate(alpha, X) * rotate(gamma, Z) * translate(Z) * rotate(M_PI, Y) * rotate(-beta, X);
}

inline Matrix transformConnection(Dock d1, int ori, Dock d2)
{
    static const std::array<Matrix, 3> dockFaceUp = {
            rotate(M_PI, Z) * rotate(-M_PI/2, Y), // Xp
            rotate(M_PI, Z) * rotate(M_PI/2, Y),  // Xn
            identity  // Zn
    };

    static const std::array<Matrix, 3> faceToDock = {
            rotate(M_PI, Z) * rotate(M_PI/2, Y), // Xp
            rotate(M_PI, Z) * rotate(-M_PI/2, Y),   // Xn
            identity // Zn
    };

    return faceToDock[d1] * rotate(ori * M_PI/2, Z) * translate(-Z) * dockFaceUp[d2] * rotate(M_PI, X);
};

/* CONFIGURATION
 *
 * */

using ModuleMap = std::unordered_map<ID, Module>;
using EdgeList = std::array<std::optional<Edge>, 6>;
using EdgeMap = std::unordered_map<ID, EdgeList>;
using MatrixMap = std::unordered_map<ID, std::array<Matrix, 2>>;

class Configuration
{
public:

    bool empty() const { return modules.empty(); }

    ModuleMap& getModules() { return modules; }
    const ModuleMap& getModules() const { return modules; }
    const EdgeMap& getEdges() const { return edges; }
    const MatrixMap & getMatrices() const { return matrices; }

    std::vector<ID> getIDs() const
    {
        std::vector<ID> ids;
        ids.reserve(modules.size());
        for (auto& [id, m] : modules)
        {
            ids.push_back(id);
        }
        return ids;
    }

    std::vector<Edge> getEdges(ID id) const
    {
        std::vector<Edge> res;
        for (auto& e : edges.at(id))
        {
            if (e.has_value())
            {
                res.push_back(e.value());
            }
        }
        return res;
    }

    // Creates new module with given ID and angles. Creates an empty set of edges corresponding to the module.
    void addModule(double alpha, double beta, double gamma, ID id)
    {
        modules.emplace(std::piecewise_construct,
                       std::forward_as_tuple(id),  // args for key
                       std::forward_as_tuple(alpha, beta, gamma, id));
        edges.emplace(std::piecewise_construct,
                      std::forward_as_tuple(id),  // args for key
                      std::forward_as_tuple());
        if ((modules.size() == 1) || (id < fixedId))
        {
            fixedId = id;
        }
    }

    // Adds given edge to given modules if both docks are free.
    bool addEdge(Edge edge)
    {
        // Finds edges for both modules.
        EdgeList& set1 = edges.at(edge.id1());
        EdgeList& set2 = edges.at(edge.id2());

        int index1 = edge.side1() * 3 + edge.dock1();
        int index2 = edge.side2() * 3 + edge.dock2();
        if (set1[index1].has_value() || set2[index2].has_value())
        {
            return false;
        }

        set1[index1] = edge;
        set2[index2] = reverse(edge);
        return true;
    }

    bool findEdge(const Edge& edge) const
    {
        return edges.at(edge.id1())[edge.side1() * 3 + edge.dock1()].has_value();
    }

    void setFixed(ID initID, Side initSide, const Matrix& initRotation)
    {
        fixedId = initID;
        fixedSide = initSide;
        fixedMatrix = initRotation;
    }

    bool isValid()
    {
        if (connectedVal == Unknown)
        {
            bool res = connected();
            connectedVal = res ? True : False;
            if (!res)
                return false;
        }
        bool res = computeMatrices();
        return (connectedVal == True) && res && collisionFree();
    }

    // Fill in the rotation attribute according to the first fixed module.
    bool computeMatrices()
    {
        if (empty())
        {
            return true;
        }
        matrices = {};
        matrices[fixedId][fixedSide] = fixedMatrix;

        return computeMatricesRec(fixedId, fixedSide);
    }

    bool connected() const
    {
        std::unordered_set<ID> seen;
        seen.insert(fixedId);
        dfsID(fixedId, seen);
        return seen.size() == modules.size();
    }

    bool collisionFree() const
    {
        std::vector<Vector> centers;
        for (const auto& [id, ms] : matrices)
        {
            centers.push_back(center(ms[A]));
            centers.push_back(center(ms[B]));
        }
        for (unsigned i = 0; i < centers.size(); ++i)
        {
            for (unsigned j = i + 1; j < centers.size(); ++j)
            {
                if (distance(centers[i], centers[j]) < 1)
                {
                    return false;
                }
            }
        }
        return true;
    }

    Vector massCenter() const
    {
        Vector mass({0,0,0,1});
        for (const auto& [id, ms] : matrices)
        {
            mass += center(ms[A]);
            mass += center(ms[B]);
        }
        mass /= modules.size()*2;
        return mass;
    }

    std::optional<Configuration> executeIfValid(const Action &action) const
    {
        int steps = 10;
        Configuration next = *this;

        std::vector<Action::Reconnect> connections;
        std::vector<Action::Reconnect> disconnections;
        for (const auto& res : action.reconnections())
        {
            if (res.add())
                connections.push_back(res);
            else
                disconnections.push_back(res);
        }

        Action connect({}, connections);
        Action disconnect({}, disconnections);

        if (!next.execute(disconnect) || !next.connected() || !next.execute(connect))
        {
            return std::nullopt;
        }

        Action rotate(action.rotations(), {});
        Action divided = rotate.divide(1.0/steps);
        for (int i = 1; i <= steps; ++i)
        {
            if (!next.isValid())
            {
                return std::nullopt;
            }
            if (!next.execute(divided))
            {
                return std::nullopt;
            }
        }
        if (next.isValid())
        {
            return next;
        }
        return std::nullopt;
    }

//    bool execute(const Action& action)
//    {
//        Configuration next = *this;
//        bool ok = true;
//        for (const Action::Rotate rot : action.rotations)
//        {
//            ok &= next.execute(rot);
//        }
//        for (const Action::Reconnect rec : action.reconnections)
//        {
//            ok &= next.execute(rec);
//        }
//        if (ok)
//        {
//            *this = next;
//        }
//        return ok;
//    }

    bool execute(const Action& action)
    {
        bool ok = true;
        for (const Action::Rotate rot : action.rotations())
        {
            ok &= execute(rot);
        }
        for (const Action::Reconnect rec : action.reconnections())
        {
            ok &= execute(rec);
        }
        return ok;
    }

    void generateActions(std::vector<Action>& res, unsigned step, unsigned bound = 1) const
    {
        std::vector<Action::Rotate> rotations;
        generateRotations(rotations, step);
        std::vector<Action::Reconnect> reconnections;
        generateReconnect(reconnections);

        std::vector<std::vector<Action::Rotate>> resRot;
        std::vector<std::vector<Action::Reconnect>> resRec;

        getAllSubsets(reconnections, resRec, bound);
        getAllRots(rotations, resRot, bound);

        for (auto& rotation : resRot)
        {
            for (auto& reconnection : resRec)
            {
                if (rotation.size() + reconnection.size() <= bound)
                    res.emplace_back(rotation, reconnection);
            }
        }

//        for (unsigned count = 1; count <= bound; ++count) // How many actions will be in generated action.
//        {
//            for (unsigned r = 0; r <= count; ++r)
//            {
//                std::vector<std::vector<Action::Rotate>> resRot;
//                std::vector<std::vector<Action::Reconnect>> resRec;
//
//                //getAllSubsets(rotations, resRot, {}, 0, r);
//                getAllSubsets(reconnections, resRec, {}, 0, count - r);
//
//                getAllRots(rotations, resRot, r);
//
//                //filter(resRot, resRotFiltered, unique);
//
//                for (auto& rotation : resRot)
//                {
//                    for (auto& reconnection : resRec)
//                    {
//                        res.emplace_back(rotation, reconnection);
//                    }
//                }
//            }
//        }
    }

    void generateReconnect(std::vector<Action::Reconnect>& res) const
    {
        generateConnections(res);
        generateDisconnections(res);
    }

    void generateConnections(std::vector<Action::Reconnect>& res) const
    {
        for (auto& [id1, ms1] : matrices)
        {
            for (auto&[id2, ms2] : matrices)
            {
                if (id1 >= id2)
                    continue;

                Edge edge(id1, A, Xp, 0, Xp, A, id2);
                auto edgeOpt = nextEdge(edge);

                while (edgeOpt.has_value())
                {
                    Vector center1 = center(ms1[edge.side1()]);
                    Vector center2 = center(ms2[edge.side2()]);
                    if (distance(center1, center2) != 1)
                    {
                        edge = edgeOpt.value();
                        edgeOpt = nextEdge(edge);
                        continue;
                    }

                    const Matrix& matrix = ms2[edge.side2()];
                    if (equals(matrix, computeConnectedMatrix(edge)) && !findEdge(edge))
                    {
                        res.emplace_back(true, edge);
                    }
                    edge = edgeOpt.value();
                    edgeOpt = nextEdge(edge);
                }
            }
        }
    }

    void generateRotations(std::vector<Action::Rotate>& res, unsigned step) const
    {
        for (auto [id, mod] : modules)
        {
            for (int d : {-step, step})
            {
                for (Joint joint : {Alpha, Beta, Gamma})
                {
                    if (mod.rotateJoint(joint, d))
                        res.emplace_back(id, joint, d);
                }
            }
        }
    }

    void generateDisconnections(std::vector<Action::Reconnect>& res) const
    {
        for (auto& [id, set] : edges)
        {
            for (const std::optional<Edge>& edgeOpt : set)
            {
                if (!edgeOpt.has_value())
                    continue;
                const Edge& edge = edgeOpt.value();
                if (edge.id1() < edge.id2())
                {
                    res.emplace_back(false, edge);
                }
            }
        }
    }

    void next(std::vector<Configuration>& res, unsigned step, unsigned bound = 1) const
    {
        std::vector<Action> actions;
        generateActions(actions, step, bound);
        //auto actions = generateActions(step, bound);
        for (auto& action : actions)
        {
            auto cfgOpt = executeIfValid(action);
            if (cfgOpt.has_value())
            {
                res.push_back(cfgOpt.value());
            }
        }
    }

    Action diff(const Configuration &other) const {
        std::vector<Action::Rotate> rotations;
        std::vector<Action::Reconnect> reconnections;

        ModuleMap moduleMap = other.getModules();
        for (const auto &[id, otherModule] : moduleMap) {
            if (modules.find(id) == modules.end()) {
                continue;
            }

            const Module &module = modules.at(id);
            for (Joint joint : { Alpha, Beta, Gamma }) {
                auto val = otherModule.getJoint(joint) - module.getJoint(joint);
                if (joint == Gamma)
                {
                    if (val > 180)
                    {
                        val = 360 - val;
                    }
                    if (val <= -180)
                    {
                        val = 360 + val;
                    }
                }
                if (val != 0) {
                    rotations.emplace_back(id, joint, val);
                }
            }

            const EdgeList &edgeList = edges.at(id);
            const EdgeList &otherEdgeList = other.edges.at(id);
            for (unsigned int i = 0; i < 6; i++) {
                auto &edge = edgeList.at(i);
                auto &otherEdge = otherEdgeList.at(i);

                if (edge == otherEdge) {
                    continue;
                }

                if (!edge.has_value()) {
                    const Edge &edge1 = otherEdge.value();
                    if (edge1.id1() < edge1.id2()) {
                        reconnections.emplace_back(true, otherEdge.value());
                    }
                } else if (!otherEdge.has_value()) {
                    const Edge &edge1 = edge.value();
                    if (edge1.id1() < edge1.id2()) {
                        reconnections.emplace_back(false, edge.value());
                    }
                } else {
                    const Edge &edge1 = otherEdge.value();
                    if (edge1.id1() < edge1.id2()) {
                        reconnections.emplace_back(true, otherEdge.value());
                        reconnections.emplace_back(false, edge.value());
                    }
                }
            }
        }

        return {rotations, reconnections};
    }

    bool operator==(const Configuration& other) const
    {
        return (modules == other.modules) &&
               (edges == other.edges);
    }

    bool operator!=(const Configuration &other) const {
        return !(*this == other);
    }

    friend ConfigurationHash;

private:
    // Maps module ID to data about the module.
    ModuleMap modules;
    // Maps module ID to edges connected to the module.
    EdgeMap edges;

    MatrixMap matrices;

    // Fixed module side: all other modules are rotated with respect to this one.
    ID fixedId = 0;
    Side fixedSide = A;
    Matrix fixedMatrix = identity;

    Value connectedVal = Unknown;
    Value matricesVal = Unknown;

    // If the given edge is in the configuration, delete it from both modules.
    bool eraseEdge(const Edge& edge)
    {
        EdgeList& set1 = edges.at(edge.id1());
        int index1 = edge.side1() * 3 + edge.dock1();

        EdgeList& set2 = edges.at(edge.id2());
        int index2 = edge.side2() * 3 + edge.dock2();

        if (!set1[index1].has_value() || !set2[index2].has_value())
        {
            return false;
        }

        set1[index1] = std::nullopt;
        set2[index2] = std::nullopt;
        connectedVal = Unknown;
        return true;
    }

    Matrix computeOtherSideMatrix(ID id, Side side) const
    {
        auto const& mod = modules.at(id);
        auto const& matrix = matrices.at(id)[side];
        double alpha = 2 * M_PI * mod.alpha / 360;
        double beta = 2 * M_PI * mod.beta / 360;
        double gamma = 2 * M_PI * mod.gamma / 360;

        if (side == B)
        {
            std::swap(alpha, beta);
        }

        return matrix * transformJoint(alpha, beta, gamma);
    }

    Matrix computeConnectedMatrix(Edge edge) const
    {
        auto const& matrix = matrices.at(edge.id1())[edge.side1()];
        return matrix * transformConnection(edge.dock1(), edge.ori(), edge.dock2());
    }

    void dfsID(ID id, std::unordered_set<ID>& seen) const
    {
        for (const std::optional<Edge> edgeOpt : edges.at(id))
        {
            if (!edgeOpt.has_value())
                continue;
            ID nextId = edgeOpt.value().id2();
            if (seen.find(nextId) == seen.end())
            {
                seen.insert(nextId);
                dfsID(nextId, seen);
            }
        }
    }

    // Fix given module: one side is fixed. Fix all connected.
    bool computeMatricesRec(ID id, Side side)
    {
        bool result = true;

        // At first, fix other side of the module.
        Side side2 = side == A ? B : A;
        auto& matrixCurr = matrices.at(id);
        matrixCurr[side2] = computeOtherSideMatrix(id, side);

        for (const std::optional<Edge>& edgeOpt : edges.at(id))
        {
            if (!edgeOpt.has_value() || edgeOpt.value().onCoeff() < 1)
                continue;
            const Edge& edge = edgeOpt.value();

            ID idNext = edge.id2();
            Side sideNext = edge.side2();

            // Compute, where next module should be.
            Matrix matrixCmp = computeConnectedMatrix(edge);

            if (matrices.find(idNext) == matrices.end())
            {
                matrices[idNext][sideNext] = matrixCmp;
                result &= computeMatricesRec(idNext, sideNext);
            }
            else
            {
                const auto& matrixNext = matrices.at(idNext)[sideNext];
                if (!equals(matrixNext, matrixCmp))
                {
                    return false;
                }
            }
        }
        return result;
    }

    bool execute(const Action::Rotate& action)
    {
        bool res = modules.at(action.id()).rotateJoint(action.joint(), action.angle());
        if (res)
            matricesVal = Unknown;
        return res;
    }

    bool execute(const Action::Reconnect& action)
    {
        if (action.add())
        {
            return addEdge(action.edge());
        }
        else
        {
            return eraseEdge(action.edge());
        }
    }
};

class ConfigurationHash
{
public:
    std::size_t operator()(const Configuration& config) const
    {
        std::size_t res = 0;
        for ( const auto& [id, mod] : config.modules )
        {
            res += hashMod(mod);
        }
        return res;
    }
private:
    std::size_t hashMod(const Module& mod) const
    {
        return static_cast<size_t>(mod.id * (13 * (mod.alpha + 90) + 17 * (mod.beta + 90) + 19 * mod.gamma));
    }
};

#endif //ROBOTS_CONFIGURATION_H
