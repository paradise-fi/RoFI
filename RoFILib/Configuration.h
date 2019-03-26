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

using ID = unsigned int;
const double threshold = 0.0001;

enum Side
{
    A, B
};

enum Dock
{
    Xp, Xn, Zn
};

enum Joint
{
    Alpha, Beta, Gamma
};

class Configuration;
class ConfigurationHash;

class Module
{
public:
    Module(double alpha, double beta, double gamma, ID id) :
            alpha(alpha), beta(beta), gamma(gamma), id(id)
    {
        matrix[Side::A] = identity;
        matrix[Side::B] = identity;
    }

    Matrix shoeMatrix(Side side) const
    {
        return matrix[side] * rotate(M_PI/2, X);
    }

    Matrix bodyMatrix(Side side) const
    {
        double diff = side == Side::A ? alpha : beta;
        diff *= M_PI/180.0;
        return matrix[side] * rotate(M_PI/2 + diff, X);
    }

    Matrix dockMatrix(Side side, Dock dock, bool on, double onCoeff = -1) const
    {
        double d;
        if (onCoeff < 0){
            d = on ? 0.05 : 0;
        } else  {
            d = onCoeff * 0.05;
        }
        const Matrix docks[3] = {
                translate(Vector{d,0,0}) * rotate(M_PI, Z), // Xp
                translate(Vector{-d,0,0}) * identity, // Xn
                translate(Vector{0,0,-d}) * rotate(-M_PI/2, Y) // Zn
        };
        return matrix[side] * docks[dock];
    }

    Vector getCenter(Side side) const
    {
        const Matrix& matrixref =  matrix[side];
        return Vector{matrixref(0,3), matrixref(1,3), matrixref(2,3), matrixref(3,3)};
    }

    ID getId() const
    {
        return id;
    }

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
        switch (joint)
        {
            case Joint::Alpha:
                alpha += val;
                if ((alpha > 90) || (alpha < -90))
                {
                    return false;
                }
                break;
            case Joint::Beta:
                beta += val;
                if ((beta > 90) || (beta < -90))
                {
                    return false;
                }
                break;
            case Joint::Gamma:
                gamma += val;
                if (gamma > 180)
                {
                    gamma -= 360;
                }
                if (gamma < -180)
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
               (std::abs(gamma - other.gamma) < threshold) &&
               equals(matrix[0], other.matrix[0]) &&
               equals(matrix[1], other.matrix[1]);
    }

private:
    ID id;
    double alpha, beta, gamma;
    std::array<Matrix, 2> matrix;
    friend Configuration;
    friend ConfigurationHash;
};

class Edge
{
public:
    Edge(ID id1, Side side1, Dock dock1, unsigned int ori, Dock dock2, Side side2, ID id2, double onCoeff = 1) :
            id1(id1), id2(id2), side1(side1), side2(side2), dock1(dock1), dock2(dock2), ori(ori), onCoeff(onCoeff) {}

    Edge& operator=(const Edge& e)
    {
        return *this;
    }

    bool operator==(const Edge& other) const
    {
        return (id1 == other.id1) &&
               (id2 == other.id2) &&
               (side1 == other.side1) &&
               (side2 == other.side2) &&
               (dock1 == other.dock1) &&
               (dock2 == other.dock2) &&
               (ori == other.ori);
    }

    const ID id1, id2;
    const Side side1, side2;
    const Dock dock1, dock2;
    const unsigned int ori;
    double onCoeff = 1;     //for visualizer
};

inline Edge arrayToEdge(ID id1, ID id2, const std::array<unsigned, 5>& res)
{
    return Edge(id1,
                static_cast<Side>(res[0]),
                static_cast<Dock>(res[1]),
                res[2],
                static_cast<Dock>(res[3]),
                static_cast<Side>(res[4]),
                id2);
}

inline std::array<unsigned, 5> getNextArray(const std::array<unsigned, 5> &edge)
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

inline Edge reverse(const Edge& edge)
{
    return {edge.id2, edge.side2, edge.dock2, edge.ori, edge.dock1, edge.side1, edge.id1, edge.onCoeff};
}

template<typename T>
inline void getAllSubsets(std::vector<T>& set, std::vector<std::vector<T>>& res, std::vector<T> accum, unsigned index, unsigned count)
{
    if (count == 0)
    {
        res.push_back(accum);
        return;
    }
    for (unsigned i = index; i < set.size(); ++i)
    {
        auto next = accum;
        next.push_back(set[i]);
        getAllSubsets(set, res, next, i + 1, count - 1);
    }
}

class Action
{
public:
    class Rotate
    {
    public:
        Rotate(ID id, Joint joint, double angle) :
                id(id), joint(joint), angle(angle) {}

        const ID id;
        const Joint joint;
        const double angle;
    };

    class Reconnect
    {
    public:
        Reconnect(bool add, const Edge& edge) :
                add(add), edge(edge) {}

        const bool add;
        const Edge edge;
    };

    Action(std::vector<Rotate> rot, std::vector<Reconnect> rec) :
            rotations(std::move(rot)), reconnections(std::move(rec)) {}

    const std::vector<Rotate> rotations;
    const std::vector<Reconnect> reconnections;

    Action divide(double factor) const
    {
        std::vector<Rotate> division;
        for (const Action::Rotate& rotate : rotations)
        {
            division.emplace_back(rotate.id, rotate.joint, rotate.angle * factor);
        }
        return {division, reconnections};
    }
};

using ModuleMap = std::unordered_map<ID, Module>;
using EdgeList = std::array<std::optional<Edge>, 6>;
using EdgeMap = std::unordered_map<ID, EdgeList>;

class Configuration
{
public:

    const ModuleMap& getModules() const
    {
        return modules;
    }

    const EdgeMap& getEdges() const
    {
        return edges;
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

    bool empty() const
    {
        return modules.empty();
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
    }

    // Adds given edge to given modules if both docks are free.
    bool addEdge(Edge edge)
    {
        // Finds edges for both modules.
        EdgeList& set1 = edges.at(edge.id1);
        EdgeList& set2 = edges.at(edge.id2);

        int index1 = edge.side1 * 3 + edge.dock1;
        int index2 = edge.side2 * 3 + edge.dock2;
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
        return edges.at(edge.id1)[edge.side1 * 3 + edge.dock1].has_value();
    }

    void setFixed(ID initID, Side initSide, const Matrix& initRotation)
    {
        fixedId = initID;
        fixedSide = initSide;
        fixedMatrix = initRotation;
    }

    bool isValid()
    {
        if (!isConnected())
        {
            return false;
        }
        if (!computeRotations())
        {
            return false;
        }
        return collisionFree();
    }

    // Fill in the rotation attribute according to the first fixed module.
    bool computeRotations()
    {
        std::unordered_set<ID> fixed;
        modules.at(fixedId).matrix[fixedSide] = fixedMatrix;

        return computeRotationsRec(fixedId, fixedSide, fixed);
    }

    bool isConnected() const
    {
        std::unordered_set<ID> seen;
        seen.insert(fixedId);
        dfsID(fixedId, seen);
        return seen.size() == modules.size();
    }

    bool collisionFree() const
    {
        std::vector<Vector> centers;
        for (const auto& [_, mod] : modules)
        {
            centers.push_back(mod.getCenter(A));
            centers.push_back(mod.getCenter(B));
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
        for (const auto& [_, mod] : modules)
        {
            mass += mod.getCenter(A);
            mass += mod.getCenter(B);
        }
        mass /= modules.size()*2;
        return mass;
    }

    std::optional<Configuration> makeValid(const Action& action) const
    {
        int steps = 10;
        Configuration next = *this;
        Action reconnect({}, action.reconnections);
        Action rotate(action.rotations, {});
        Action divided = rotate.divide(1.0/steps);

        if (!next.execute(reconnect))
        {
            return std::nullopt;
        }
        for (int i = 1; i <= steps; ++i)
        {
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

    bool execute(const Action& action)
    {
        Configuration next = *this;
        bool ok = true;
        for (const Action::Rotate rot : action.rotations)
        {
            ok &= next.execute(rot);
        }
        for (const Action::Reconnect rec : action.reconnections)
        {
            ok &= next.execute(rec);
        }
        if (ok)
        {
            *this = next;
        }
        return ok;
    }

    std::vector<Action> generateActions(unsigned step, unsigned bound = 1) const
    {
        auto rotations = generateRotations(step);
        auto reconnections = generateReconnect();

        std::vector<Action> res;
        for (unsigned count = 1; count <= bound; ++count) // How many actions will be in generated action.
        {
            for (unsigned r = 0; r <= count; ++r)
            {
                std::vector<std::vector<Action::Rotate>> resRot;
                std::vector<std::vector<Action::Reconnect>> resRec;

                getAllSubsets(rotations, resRot, {}, 0, r);
                getAllSubsets(reconnections, resRec, {}, 0, count - r);

                for (auto& rotation : resRot)
                {
                    for (auto& reconnection : resRec)
                    {
                        res.emplace_back(rotation, reconnection);
                    }
                }
            }
        }
        return res;
    }

    std::vector<Configuration> next(unsigned step, unsigned bound = 1) const
    {
        std::vector<Configuration> res;
        auto actions = generateActions(step, bound);
        //auto actions = generateActions(step, bound);
        for (auto& action : actions)
        {
            auto cfgOpt = makeValid(action);
            if (cfgOpt.has_value())
            {
                res.push_back(cfgOpt.value());
            }
        }
        return res;
    }

    std::vector<Configuration> nextRotate(unsigned step) const
    {
        std::vector<Configuration> res;
        for (auto& [id, _] : modules)
        {
            for (int d : {-step, step})
            {
                for (Joint joint : {Alpha, Beta, Gamma})
                {
                    Configuration next = *this;
                    Module& mod = next.modules.at(id);
                    bool ok = mod.rotateJoint(joint, d);
                    if (!ok)
                    {
                        continue;
                    }
                    if (next.isValid())
                    {
                        res.push_back(next);
                    }
                }
            }
        }
        return res;
    }

//    std::vector<Configuration> nextReconnect() const
//    {
//        std::vector<Configuration> res;
//        for (auto& [id, set] : edges)
//        {
//            for (const std::optional<Edge>& edgeOpt : set)
//            {
//                if (!edgeOpt.has_value())
//                    continue;
//                const Edge& edge = edgeOpt.value();
//                if (edge.id1 < edge.id2)
//                {
//                    Configuration next = *this;
//                    next.eraseEdge(edge);
//
//                    if (next.isValid())
//                    {
//                        res.push_back(next);
//                    }
//                }
//            }
//        }
//        for (auto& [id1, mod1] : modules)
//        {
//            for (auto& [id2, mod2] : modules)
//            {
//                if (id1 >= id2)
//                    continue;
//                for (auto [side1, side2] : std::vector<std::pair<Side, Side>>{{A, A}, {A, B}, {B, A}, { B, B}})
//                {
//                    Vector center1 = mod1.getCenter(side1);
//                    Vector center2 = mod2.getCenter(side2);
//                    if (distance(center1, center2) != 1)
//                        continue;
//                    for (auto [dock1, dock2] : std::vector<std::pair<Dock, Dock>>{{Xn, Xn}, {Xn, Xp}, {Xn, Zn}, {Xp, Xn}, {Xp, Xp}, {Xp, Zn}, {Zn, Xn}, {Zn, Xp}, {Zn, Zn}})
//                    {
//                        for (const int ori : {0, 1, 2, 3})
//                        {
//                            const Matrix& matrix2 = mod2.matrix[side2];
//                            Edge edge(id1, side1, dock1, (unsigned int) ori, dock2, side2, id2);
//                            if (equals(matrix2, computeConnectedMatrix(edge)))
//                            {
//                                Configuration next = *this;
//                                if (!next.addEdge(edge))
//                                    continue;
//                                if (next.isValid())
//                                {
//                                    res.push_back(next);
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//        }
//        return res;
//    }

    bool operator==(const Configuration& other) const
    {
        return (modules == other.modules) &&
               (edges == other.edges) &&
               (fixedId == other.fixedId) &&
               (fixedSide == other.fixedSide) &&
               (equals(fixedMatrix, other.fixedMatrix));
    }

    friend ConfigurationHash;

private:
    // Maps module ID to data about the module.
    ModuleMap modules;
    // Maps module ID to edges connected to the module.
    EdgeMap edges;

    // Fixed module side: all other modules are rotated with respect to this one.
    ID fixedId = 0;
    Side fixedSide = A;
    Matrix fixedMatrix = identity;

    // If the given edge is in the configuration, delete it from both modules.
    bool eraseEdge(const Edge& edge)
    {
        EdgeList& set1 = edges.at(edge.id1);
        int index1 = edge.side1 * 3 + edge.dock1;

        EdgeList& set2 = edges.at(edge.id2);
        int index2 = edge.side2 * 3 + edge.dock2;

        if (!set1[index1].has_value() || !set2[index2].has_value())
        {
            return false;
        }

        set1[index1] = std::nullopt;
        set2[index2] = std::nullopt;
        return true;
    }

    //
    Matrix computeOtherSideMatrix(ID id, Side side) const
    {
        auto const& mod = modules.at(id);
        double alpha = 2 * M_PI * mod.alpha / 360;
        double beta = 2 * M_PI * mod.beta / 360;
        double gamma = 2 * M_PI * mod.gamma / 360;

        if (side == B)
        {
            std::swap(alpha, beta);
        }

        Matrix fix = mod.matrix[side] * rotate(alpha, X) * rotate(gamma, Z) * translate(Z) * rotate(M_PI, Y) * rotate(-beta, X);

        return fix;
    }

    Matrix computeConnectedMatrix(Edge edge) const
    {
        static const std::array<Matrix, 3> dockFaceUp = {
                rotate(-M_PI/2, Z) * rotate(M_PI/2, Y), // Xp
                rotate(M_PI/2, Z) *rotate(-M_PI/2, Y),  // Xn
                identity  // Zn
        };

        static const std::array<Matrix, 3> faceToDock = {
                rotate(-M_PI/2, Y) * rotate(-M_PI/2, Z), // Xp
                rotate(M_PI/2, Y) * rotate(M_PI/2, Z),   // Xn
                identity // Zn
        };

        auto const& matrix = modules.at(edge.id1).matrix[edge.side1];
        Matrix fix2 = matrix * faceToDock[edge.dock1] * rotate(edge.ori * M_PI/2, Z)
                * translate(-Z) * dockFaceUp[edge.dock2] * rotate(M_PI, Y);

        return fix2;
    }

    void dfsID(ID id, std::unordered_set<ID>& seen) const
    {
        for (const std::optional<Edge> edgeOpt : edges.at(id))
        {
            if (!edgeOpt.has_value())
                continue;
            ID nextId = edgeOpt.value().id2;
            if (seen.find(nextId) == seen.end())
            {
                seen.insert(nextId);
                dfsID(nextId, seen);
            }
        }
    }

    // Fix given module: one side is fixed. Fix all connected.
    bool computeRotationsRec(ID id, Side side, std::unordered_set<ID>& fixed)
    {
        bool result = true;

        // At first, fix other side of the module.
        Side side2 = side == A ? B : A;
        auto& rotation = modules.at(id).matrix;
        rotation[side2] = computeOtherSideMatrix(id, side);
        fixed.insert(id);

        for (const std::optional<Edge>& edgeOpt : edges.at(id))
        {
            if (!edgeOpt.has_value())
                continue;
            const Edge& edge = edgeOpt.value();

            ID idNext = edge.id2;
            Side sideNext = edge.side2;

            // Compute, where next module should be.
            Matrix rotationComputed = computeConnectedMatrix(edge);
            auto& rotationNext = modules.at(idNext).matrix[sideNext];

            // If it's not fixed yet, fix it and continue computation.
            if (fixed.find(idNext) == fixed.end())
            {
                rotationNext = rotationComputed;
                result &= computeRotationsRec(idNext, sideNext, fixed);
            }
            else
            {
                // If it was already fixed, check that the rotations are the same.
                if (!equals(rotationNext, rotationComputed))
                {
                    return false;
                }
            }
        }
        return result;
    }

    std::vector<Action::Rotate> generateRotations(unsigned step) const
    {
        std::vector<Action::Rotate> res;
        for (auto& [id, _] : modules)
        {
            for (int d : {-step, step})
            {
                for (Joint joint : {Alpha, Beta, Gamma})
                {
                    res.emplace_back(id, joint, d);
                }
            }
        }
        return res;
    }

    std::vector<Action::Reconnect> generateReconnect() const
    {
        auto con = generateConnections();
        auto dis = generateDisconnections();
        std::vector<Action::Reconnect> res(con.begin(), con.end());
        std::copy(dis.begin(), dis.end(), std::back_inserter(res));

        return res;
    }

    std::vector<Action::Reconnect> generateDisconnections() const
    {
        std::vector<Action::Reconnect> res;
        for (auto& [id, set] : edges)
        {
            for (const std::optional<Edge>& edgeOpt : set)
            {
                if (!edgeOpt.has_value())
                    continue;
                const Edge& edge = edgeOpt.value();
                if (edge.id1 < edge.id2)
                {
                    res.emplace_back(false, edge);
                }
            }
        }
        return res;
    }

    std::vector<Action::Reconnect> generateConnections() const
    {
        std::vector<Action::Reconnect> res;
        for (auto& [id1, mod1] : modules)
        {
            for (auto&[id2, mod2] : modules)
            {
                if (id1 >= id2)
                    continue;
                const static std::array<unsigned, 5> init = {0,0,0,0,0};
                std::array<unsigned, 5> array = {0,0,0,0,0};
                do
                {
                    Edge edge = arrayToEdge(id1, id2, array);
                    array = getNextArray(array);
//                    for(auto& x : array)
//                    {
//                        std::cout << x;
//                    }
//                    std::cout << std::endl;

                    Vector center1 = mod1.getCenter(edge.side1);
                    Vector center2 = mod2.getCenter(edge.side2);
                    if (distance(center1, center2) != 1)
                        continue;

                    const Matrix& matrix = mod2.matrix[edge.side2];
                    if (equals(matrix, computeConnectedMatrix(edge)) && !findEdge(edge))
                    {
                        res.emplace_back(true, edge);
                    }

                } while (array != init);
            }
        }
        return res;
    }

    bool execute(const Action::Rotate& action)
    {
        return modules.at(action.id).rotateJoint(action.joint, action.angle);
    }

    bool execute(const Action::Reconnect& action)
    {
        if (action.add)
        {
            return addEdge(action.edge);
        }
        else
        {
            return eraseEdge(action.edge);
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
        return mod.id * (13 * (mod.alpha + 90) + 17 * (mod.beta + 90) + 19 * mod.gamma);
    }
};

#endif //ROBOTS_CONFIGURATION_H
