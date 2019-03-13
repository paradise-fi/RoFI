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

using ID = unsigned int;

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
        rotation[A] = identity;
        rotation[B] = identity;
    }

    Matrix frameMatrix(Side side) const
    {
        return rotation.at(side) * rotate(M_PI/2, X);
    }

    Matrix jointMatrix(Side side) const
    {
        double diff = side == A ? alpha : beta;
        diff *= M_PI/180.0;
        return rotation.at(side) * rotate(M_PI/2 + diff, X);
    }

    Matrix dockMatrix(Side side, Dock dock, bool on) const
    {
        double d = on ? 0.05 : 0;
        Matrix docks[3] = {
                translate(Vector{d,0,0}) * rotate(M_PI, Z), // Xp
                translate(Vector{-d,0,0}), // Xn
                translate(Vector{0,0,-d}) * rotate(-M_PI/2, Y) // Zn
        };
        return rotation.at(side) * docks[dock];
    }

    Vector getCenter(Side side) const
    {
        const Matrix& matrix=  rotation.at(side);
        return Vector{matrix(0,3), matrix(1,3), matrix(2,3), matrix(3,3)};
    }

    ID getId() const
    {
        return id;
    }

    double getJoint(Joint a) const
    {
        switch (a)
        {
            case Alpha:
                return alpha;
            case Beta:
                return beta;
            default:
                return gamma;
        }
    }

    void rotateJoint(Joint joint, int val)
    {
        switch (joint)
        {
            case Alpha:
                alpha += val;
                alpha = std::max(std::min(alpha, 90.0), -90.0);
                break;
            case Beta:
                beta += val;
                beta = std::max(std::min(beta, 90.0), -90.0);
                break;
            case Gamma:
                gamma += val;
                gamma = fmod(gamma, 360.0);
        }
    }

    bool operator==(const Module& other) const
    {
        return (id == other.id) &&
               (alpha == other.alpha) &&
               (beta == other.beta) &&
               (gamma == other.gamma) &&
               equals(rotation.at(A), other.rotation.at(A)) &&
               equals(rotation.at(B), other.rotation.at(B));
    }

private:
    ID id;
    double alpha, beta, gamma;
    std::unordered_map<Side, Matrix> rotation;
    friend Configuration;
    friend ConfigurationHash;
};

class Edge
{
public:
    Edge(ID id1, Side side1, Dock dock1, unsigned int ori, Dock dock2, Side side2, ID id2) :
            id1(id1), id2(id2), side1(side1), side2(side2), dock1(dock1), dock2(dock2), ori(ori) {}

    Side getSide1() const
    {
        return side1;
    }

    Dock getDock1() const
    {
        return dock1;
    }

    Side getSide2() const
    {
        return side2;
    }

    Dock getDock2() const
    {
        return dock2;
    }

    unsigned getOri() const
    {
        return ori;
    }

    ID getId1() const
    {
        return id1;
    }

    ID getId2() const
    {
        return id2;
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

private:
    ID id1, id2;
    Side side1, side2;
    Dock dock1, dock2;
    unsigned int ori;
    friend Configuration;
};

inline std::vector<Edge> sort(const std::vector<Edge>& vec)
{
    int edges[6] = {-1,-1,-1,-1,-1,-1};
    for (int i = 0; i < vec.size(); ++i)
    {
        int index = vec[i].getSide1() * 3 + vec[i].getDock1();
        edges[index] = i;
    }
    std::vector<Edge> res;
    for (int edge : edges) {
        if (edge != -1)
        {
            res.push_back(vec[edge]);
        }
    }
    return res;
}


using ModuleMap = std::unordered_map<ID, Module>;
using EdgeMap = std::unordered_map<ID, std::vector<Edge>>;

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
    bool addEdge(ID id1, Side side1, Dock dock1, unsigned int ori, Dock dock2, Side side2, ID id2)
    {
        // Finds edges for both modules.
        std::vector<Edge>& set1 = edges.at(id1);
        std::vector<Edge>& set2 = edges.at(id2);

        // If both docks are free, edges are added.
        if ((findMatch(id1, side1, dock1, set1) == -1) && (findMatch(id2, side2, dock2, set2) == -1))
        {
            set1.emplace_back(id1, side1, dock1, ori, dock2, side2, id2);
            set2.emplace_back(id2, side2, dock2, ori, dock1, side1, id1);
            return true;
        }
        return false;
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
        modules.at(fixedId).rotation.at(fixedSide) = fixedMatrix;

        return computeRotationsRec(fixedId, fixedSide, fixed);
    }

    bool isConnected() const
    {
        std::queue<ID> queue;
        std::unordered_set<ID> seen;
        queue.push(fixedId);
        seen.insert(fixedId);
        while (!queue.empty())
        {
            ID id = queue.front();
            queue.pop();
            for (const Edge& edge : edges.at(id))
            {
                ID other = edge.id2;
                if (seen.find(other) == seen.end())
                {
                    queue.push(other);
                    seen.insert(other);
                }
            }
        }
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

    std::vector<Configuration> next(unsigned step) const
    {
        std::vector<Configuration> rec = nextReconnect();
        std::vector<Configuration> rot = nextRotate(step);
        std::vector<Configuration> res;
        res.reserve(rec.size() + rot.size());
        res.insert(res.end(), rec.begin(), rec.end());
        res.insert(res.end(), rot.begin(), rot.end());
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
                    mod.rotateJoint(joint, d);
                    if (next.isValid())
                    {
                        res.push_back(next);
                    }
                }
            }
        }
        return res;
    }

    std::vector<Configuration> nextReconnect() const
    {
        std::vector<Configuration> res;
        for (auto& [id, set] : edges)
        {
            for (const Edge& edge : set)
            {
                if (edge.id1 < edge.id2)
                {
                    Configuration next = *this;
                    next.eraseEdge(edge);

                    if (next.isValid())
                    {
                        res.push_back(next);
                    }
                }
            }
        }
        for (auto& [id1, mod1] : modules)
        {
            for (auto& [id2, mod2] : modules)
            {
                if (id1 >= id2)
                    continue;
                for (Side side1 : {A, B})
                {
                    Vector center1 = mod1.getCenter(side1);
                    for (Side side2 : {A, B})
                    {
                        Vector center2 = mod2.getCenter(side2);
                        if (distance(center1, center2) != 1)
                            continue;
                        for (Dock dock1 : {Xn, Xp, Zn})
                        {
                            for (Dock dock2 : {Xn, Xp, Zn})
                            {
                                for (const int ori : {0, 1, 2, 3})
                                {
                                    const Matrix& matrix2 = mod2.rotation.at(side2);
                                    Edge edge(id1, side1, dock1, (unsigned int) ori, dock2, side2, id2);
                                    if (equals(matrix2, computeConnected(edge)))
                                    {
                                        Configuration next = *this;
                                        if (!next.addEdge(id1, side1, dock1, ori, dock2, side2, id2))
                                            continue;
                                        if (next.isValid())
                                        {
                                            res.push_back(next);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return res;
    }

    bool operator==(const Configuration& other) const
    {
        EdgeMap newEdges;
        for (auto& [id, vec] : edges)
        {
            newEdges[id] = sort(vec);
        }
        EdgeMap newOtherEdges;
        for (auto& [id, vec] : other.edges)
        {
            newOtherEdges[id] = sort(vec);
        }
        return (modules == other.modules) &&
               (newEdges == newOtherEdges) &&
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

    // Finds, whether the given dock is occupied.
    int findMatch(ID id, Side side, Dock dock, const std::vector<Edge>& set) const
    {
        for (int i = 0; i < set.size(); ++i)
        {
            Edge edge = set[i];
            if ((id == edge.id1) && (side == edge.side1) && (dock == edge.dock1))
                return i;
        }
        return -1;
    }

    // If the given edge is in the configuration, delete it from both modules.
    void eraseEdge(const Edge& edge)
    {
        std::vector<Edge>& set1 = edges.at(edge.id1);
        int i = findMatch(edge.id1, edge.side1, edge.dock1, set1);
        if (i > -1)
        {
            set1.erase(set1.begin() + i);
        }

        std::vector<Edge>& set2 = edges.at(edge.id2);
        int j = findMatch(edge.id2, edge.side2, edge.dock2, set2);
        if (j > -1)
        {
            set2.erase(set2.begin() + j);
        }
    }

    //
    Matrix computeOtherSide(ID id, Side side) const
    {
        auto const& mod = modules.at(id);
        double alpha = 2 * M_PI * mod.alpha / 360;
        double beta = 2 * M_PI * mod.beta / 360;
        double gamma = 2 * M_PI * mod.gamma / 360;

        if (side == B)
        {
            std::swap(alpha, beta);
        }

        Matrix fix = mod.rotation.at(side) * rotate(alpha, X) * rotate(gamma, Z) * translate(Z) * rotate(M_PI, Y) * rotate(-beta, X);

        return fix;
    }

    Matrix computeConnected(Edge edge) const
    {
        std::array<Matrix, 3> dockFaceUp = {
                rotate(-M_PI/2, Z) * rotate(M_PI/2, Y), // Xp
                rotate(M_PI/2, Z) *rotate(-M_PI/2, Y),  // Xn
                identity  // Zn
        };

        std::array<Matrix, 3> faceToDock = {
                rotate(-M_PI/2, Y) * rotate(-M_PI/2, Z), // Xp
                rotate(M_PI/2, Y) * rotate(M_PI/2, Z),   // Xn
                identity // Zn
        };

        auto const& matrix = modules.at(edge.id1).rotation.at(edge.side1);
        Matrix fix2 = matrix * faceToDock[edge.dock1] * rotate(edge.ori * M_PI/2, Z)
                * translate(-Z) * dockFaceUp[edge.dock2] * rotate(M_PI, Y);

        return fix2;
    }

    // Fix given module: one side is fixed. Fix all connected.
    bool computeRotationsRec(ID id, Side side, std::unordered_set<ID>& fixed)
    {
        bool result = true;

        // At first, fix other side of the module.
        Side side2 = side == A ? B : A;
        auto& rotation = modules.at(id).rotation;
        rotation.at(side2) = computeOtherSide(id, side);
        fixed.insert(id);

        for (Edge& edge : edges.at(id))
        {
            ID idNext = edge.id2;
            Side sideNext = edge.side2;

            // Compute, where next module should be.
            Matrix rotationComputed = computeConnected(edge);
            auto& rotationNext = modules.at(idNext).rotation.at(sideNext);

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
