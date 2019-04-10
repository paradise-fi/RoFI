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
            alpha(alpha), beta(beta), gamma(gamma), id(id) {}

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
                if (gamma >= 180)
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
               (std::abs(gamma - other.gamma) < threshold);
    }

private:
    ID id;
    double alpha, beta, gamma;
    //std::array<Matrix, 2> matrix;
    friend Configuration;
    friend ConfigurationHash;
};

class Edge
{
public:
    Edge(ID id1, Side side1, Dock dock1, unsigned int ori, Dock dock2, Side side2, ID id2, double onCoeff = 1) :
            id1_(id1), id2_(id2), side1_(side1), side2_(side2), dock1_(dock1), dock2_(dock2), ori_(ori), onCoeff_(onCoeff) {}

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

    ID id1() const
    {
        return id1_;
    }

    ID id2() const
    {
        return id2_;
    }

    Side side1() const
    {
        return side1_;
    }

    Side side2() const
    {
        return side2_;
    }

    Dock dock1() const
    {
        return dock1_;
    }

    Dock dock2() const
    {
        return dock2_;
    }

    unsigned int ori() const
    {
        return ori_;
    }

    double onCoeff() const
    {
        return onCoeff_;
    }

    void setOnCoeff(double value)
    {
        onCoeff_ = value;
    }

private:
    ID id1_, id2_;
    Side side1_, side2_;
    Dock dock1_, dock2_;
    unsigned int ori_;
    double onCoeff_ = 1;     //for visualizer
};

inline Vector getCenter(const Matrix& m)
{
    return Vector{m(0,3), m(1,3), m(2,3), m(3,3)};
}

inline Matrix transformJoint(double alpha, double beta, double gamma)
{
    return rotate(alpha, X) * rotate(gamma, Z) * translate(Z) * rotate(M_PI, Y) * rotate(-beta, X);
}

inline Matrix transformConnection(Dock d1, int ori, Dock d2)
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

    return faceToDock[d1] * rotate(ori * M_PI/2, Z) * translate(-Z) * dockFaceUp[d2] * rotate(M_PI, Y);
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
    return {edge.id2(), edge.side2(), edge.dock2(), edge.ori(), edge.dock1(), edge.side1(), edge.id1(), edge.onCoeff()};
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

inline bool unique(const std::vector<Action::Rotate>& a)
{
    std::unordered_map<ID, std::array<bool, 3>> moved;
    for (const auto& rot : a)
    {
        if (moved.find(rot.id) == moved.end())
        {
            moved[rot.id] = {false, false, false};
            moved[rot.id][rot.joint] = true;
        }
        else
        {
            if (moved[rot.id][rot.joint])
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

using ModuleMap = std::unordered_map<ID, Module>;
using EdgeList = std::array<std::optional<Edge>, 6>;
using EdgeMap = std::unordered_map<ID, EdgeList>;
using MatrixMap = std::unordered_map<ID, std::array<Matrix, 2>>;

class Configuration
{
public:

    const ModuleMap& getModules() const
    {
        return modules;
    }

    ModuleMap& getModules()
    {
        return modules;
    }

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

    const MatrixMap & getMatrices() const
    {
        return matrices;
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
        if (!connected())
        {
            return false;
        }
        if (!computeMatrices())
        {
            return false;
        }
        return collisionFree();
    }

    // Fill in the rotation attribute according to the first fixed module.
    bool computeMatrices()
    {
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
            centers.push_back(getCenter(ms[A]));
            centers.push_back(getCenter(ms[B]));
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
            mass += getCenter(ms[A]);
            mass += getCenter(ms[B]);
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
        for (const auto& res : action.reconnections)
        {
            if (res.add)
            {
                connections.push_back(res);
            }
            else
            {
                disconnections.push_back(res);
            }
        }

        Action connect({}, connections);
        Action disconnect({}, disconnections);

        if (!next.execute(disconnect))
        {
            return std::nullopt;
        }

        if (!next.connected())
        {
            return std::nullopt;
        }

        if (!next.execute(connect))
        {
            return std::nullopt;
        }


        Action rotate(action.rotations, {});
        Action divided = rotate.divide(1.0/steps);
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
                std::vector<std::vector<Action::Rotate>> resRotFiltered;
                std::vector<std::vector<Action::Reconnect>> resRec;

                getAllSubsets(rotations, resRot, {}, 0, r);
                getAllSubsets(reconnections, resRec, {}, 0, count - r);

                filter(resRot, resRotFiltered, unique);

                for (auto& rotation : resRotFiltered)
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
            auto cfgOpt = executeIfValid(action);
            if (cfgOpt.has_value())
            {
                res.push_back(cfgOpt.value());
            }
        }
        return res;
    }

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

    MatrixMap matrices;

    // Fixed module side: all other modules are rotated with respect to this one.
    ID fixedId = 0;
    Side fixedSide = A;
    Matrix fixedMatrix = identity;

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
            if (!edgeOpt.has_value())
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
                if (edge.id1() < edge.id2())
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
        for (auto& [id1, ms1] : matrices)
        {
            for (auto&[id2, ms2] : matrices)
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

                    Vector center1 = getCenter(ms1[edge.side1()]);
                    Vector center2 = getCenter(ms2[edge.side2()]);
                    if (distance(center1, center2) != 1)
                        continue;

                    const Matrix& matrix = ms2[edge.side2()];
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
        return static_cast<size_t>(mod.id * (13 * (mod.alpha + 90) + 17 * (mod.beta + 90) + 19 * mod.gamma));
    }
};

inline int findFreeIndex(int index, const std::array<bool, 6>& A)
{
    int count = 6;
    while (!A[index] && count > 0)
    {
        index = (index + 1) % 6;
        --count;
    }
    if (count == 0)
    {
        return -1;
    }
    return index;
}

inline std::optional<Edge> generateEdge(ID id1, ID id2, std::unordered_map<ID, std::array<bool, 6>>& occupied)
{
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 11);

    int index1 = findFreeIndex(dist(rd) % 6, occupied[id1]);
    int index2 = findFreeIndex(dist(rd) % 6, occupied[id2]);

    occupied[id1][index1] = false;
    occupied[id2][index2] = false;

    if ((index1 < 0) || (index2 < 0))
    {
        return std::nullopt;
    }
    int ori = dist(rd) % 4;
    auto s1 = Side(index1 % 3 % 2);
    auto d1 = Dock(index1 / 3);
    auto s2 = Side(index2 % 3 % 2);
    auto d2 = Dock(index2 / 3);
    return Edge(id1, s1, d1, ori, d2, s2, id2);
}

inline std::optional<Configuration> generateAngles(const std::vector<ID>& ids, const std::vector<Edge>& edges)
{
    // Assume the edges form a connected graph and that edges contain only IDs from the 'ids' vector.
    std::random_device rd;
    std::uniform_int_distribution<int> ab(-1,1);
    std::uniform_int_distribution<int> c(0, 3);
    double step = 90;
    Configuration cfg;
    for (ID id : ids)
    {
        double alpha = step * ab(rd);
        double beta = step * ab(rd);
        double gamma = step * c(rd);
        cfg.addModule(alpha,beta,gamma,id);
    }
    for (const Edge& edge : edges)
    {
        cfg.addEdge(edge);
    }

    int count = 1000;
    while (!cfg.isValid() && count > 0)
    {
        // Generate random angles for all modules and joints.
        for (auto& [id, mod] : cfg.getModules())
        {
            double alpha = step * ab(rd);
            double beta = step * ab(rd);
            double gamma = step * c(rd);
            mod.setJoint(Joint::Alpha, alpha);
            mod.setJoint(Joint::Beta, beta);
            mod.setJoint(Joint::Gamma, gamma);
        }
        --count;
    }
    if (count == 0)
        return std::nullopt;
    return cfg;
}

inline std::optional<Configuration> sampleFree(const std::vector<ID>& ids)
{
    std::default_random_engine e;
    std::random_device rd;
    std::uniform_int_distribution<unsigned long> dist(0, ids.size());


    std::vector<Edge> edges;
    std::vector<ID> shuffled = ids;
    std::shuffle(shuffled.begin(), shuffled.end(), e);
    std::unordered_map<ID, std::array<bool, 6>> occupied;
    occupied[shuffled[0]] = {true, true, true, true, true, true};

    for (unsigned i = 1; i < shuffled.size(); ++i)
    {
        ID id1 = shuffled[i];
        ID id2 = shuffled[dist(rd) % i];
        occupied[id1] = {true, true, true, true, true, true};
        std::optional<Edge> edgeOpt = generateEdge(id1, id2, occupied);
        if (!edgeOpt.has_value())
        {
            return std::nullopt;
        }

        edges.emplace_back(edgeOpt.value());
    }
    return generateAngles(ids, edges);
}

#endif //ROBOTS_CONFIGURATION_H
