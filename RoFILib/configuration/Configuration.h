//
// Created by xvozarov on 2/18/19.
//

#ifndef ROBOTS_CONFIGURATION_H
#define ROBOTS_CONFIGURATION_H

#include "Matrix.h"
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <optional>

using ID = int;

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

class Module {
public:
    Module(double alpha, double beta, double gamma, ID id) :
            alpha(alpha), beta(beta), gamma(gamma), id(id) {}

    ID getId() const { return id; }
    double getJoint(Joint a) const;

    /**
     * \brief Rotates joint
     * 
     * Failure (returning `false`) in this method leaves the joint unchanged.
     * 
     * \return `false` if adding \p val makes value of \p joint 
     * overflow out of accepted range.
     * 
     * Throws error if \p joint is not in enum
     */
    bool rotateJoint(Joint joint, double val);

    /**
     * \brief Sets joint
     * 
     * \return `false` if \p val wasn't in accepted range and thus
     * value of joint wasn't changed
     * 
     * Throws error if \p joint is not in enum
     */
    bool setJoint(Joint joint, double val);

    bool operator==(const Module& other) const;

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

enum ShoeId { A = 0, B = 1 };
enum ConnectorId { XPlus = 0, XMinus = 1, ZMinus = 2 };
enum Orientation { North = 0, East = 1, South = 2, West = 3 };

class Edge {
public:
    Edge(ID id1, ShoeId side1, ConnectorId dock1, unsigned int ori, 
        ConnectorId dock2, ShoeId side2, ID id2, double onCoeff = 1) :
            id1_(id1), id2_(id2), side1_(side1), side2_(side2), 
            dock1_(dock1), dock2_(dock2), ori_(ori), onCoeff_(onCoeff) {}

    ID id1() const { return id1_; }
    ID id2() const { return id2_; }
    ShoeId side1() const { return side1_; }
    ShoeId side2() const { return side2_; }
    ConnectorId dock1() const { return dock1_; }
    ConnectorId dock2() const { return dock2_; }
    unsigned int ori() const { return ori_; }
    double onCoeff() const { return onCoeff_; }

    void setOnCoeff(double value) { onCoeff_ = value; }

    bool operator==(const Edge& other) const;
    bool operator!=(const Edge& other) const { return !(*this == other); }

private:
    ID id1_, id2_;
    ShoeId side1_, side2_;
    ConnectorId dock1_, dock2_;
    unsigned int ori_;
    double onCoeff_ = 1;     //for visualizer
};

inline Edge reverse(const Edge& edge) {
    return {edge.id2(), edge.side2(), edge.dock2(), edge.ori(), 
        edge.dock1(), edge.side1(), edge.id1(), edge.onCoeff()};
}

/**
 * \brief Given \p edge returns next possible connection of \p edge.id1 
 * and \p edge.id2
 * 
 * Calling this function until it returns std::nullopt will generate all
 * possible edges between \p edge.id1 and \p edge.id2 -- all possible
 * combinations of shoes, docks and orientations.
 * 
 * \return Edge according to thenext possible connection of \p edge.id1 
 * and \p edge.id2
 */
std::optional<Edge> nextEdge(const Edge& edge);

/* ACTIONS
 *
 * Action is an object representing a set of atomic actions occurring in parallel.
 * Atomic actions are rotations and reconnections (connect -- disconnect).
 *
 * Function divide creates a new action with smaller rotations, used for simulating
 * the continuous movement in discrete steps.
 *
 * */

class Action {
public:
    class Rotate {
    public:
        Rotate() : _id(0), _joint(Alpha), _angle(0) {};
        Rotate(ID id, Joint joint, double angle) :
                _id(id), _joint(joint), _angle(angle) {}

        ID id() const { return _id; }
        Joint joint() const { return _joint; }
        double angle() const { return _angle; }

        bool operator==(const Rotate &other) const;
        bool operator!=(const Rotate &other) const { return !(*this == other); }

    private:
        ID _id;
        Joint _joint;
        double _angle;
        friend Action;
    };

    class Reconnect {
    public:

        Reconnect() : _add(false), _edge({0,A,ZMinus,0,ZMinus,A,0}) {};
        Reconnect(bool add, const Edge& edge) :
                _add(add), _edge(edge) {}

        bool add() const { return _add; }
        Edge edge() const { return _edge; }

        bool operator==(const Reconnect &other) const;
        bool operator!=(const Reconnect &other) const { return !(*this == other); }

    private:
        bool _add;
        Edge _edge;
        friend Action;
    };

    Action(std::vector<Rotate> rot, std::vector<Reconnect> rec) :
            _rotations(std::move(rot)), _reconnections(std::move(rec)) {}

    Action(const Rotate& rot) :
            _rotations(), _reconnections() { _rotations.push_back(rot); }

    Action(const Reconnect& rec) :
            _rotations(), _reconnections() { _reconnections.push_back(rec); }

    Action divide(double factor) const;

    std::vector<Rotate> rotations() const { return _rotations; }
    std::vector<Reconnect> reconnections() const { return _reconnections; }

    bool empty() const { return _reconnections.empty() && _rotations.empty(); }

private:
    std::vector<Rotate> _rotations;
    std::vector<Reconnect> _reconnections;

};

/**
 * \brief Checks if given vector of rotations rotates every joint
 * at most once.
 * 
 * \return `false` if there are at least two rotations in \p rotations 
 * that rotate same joint of the same module
 */
bool unique(const std::vector<Action::Rotate>& rotations);

/**
 * \brief Copies elements of \p data to \p res if they satisfy predicate
 * \p pred.
 */
template<typename T>
void filter(const std::vector<T>& data, std::vector<T>& res, bool (*pred)(const T&)) {
    for (const T& datum : data) {
        if ((*pred)(datum)) {
            res.push_back(datum);
        }
    }
}

/* TRANSFORMATION MATRICES
 *
 * Functions transformJoint and transformConnection define the relative positions
 * of module sides and of connected modules. The relative positions are defined in
 * the thesis of Honza Mrazek.
 *
 * */

Matrix transformJoint(double alpha, double beta, double gamma);

Matrix transformConnection(ConnectorId d1, int ori, ConnectorId d2);

/* CONFIGURATION
 *
 * */

using ModuleMap = std::unordered_map<ID, Module>;
using EdgeList = std::array<std::optional<Edge>, 6>;
using EdgeMap = std::unordered_map<ID, EdgeList>;
using MatrixMap = std::unordered_map<ID, std::array<Matrix, 2>>;

enum Value { True, False, Unknown };

class Configuration
{
public:

    bool empty() const { return modules.empty(); }

    ModuleMap& getModules() { return modules; }
    const ModuleMap& getModules() const { return modules; }
    const EdgeMap& getEdges() const { return edges; }
    const MatrixMap & getMatrices() const { return matrices; }

    Module& getModule(ID id);
    const Module& getModule(ID id) const;
    std::vector<ID> getIDs() const;
    std::vector<Edge> getEdges(ID id) const;
    std::vector<Edge> getEdges(ID id, const std::unordered_set<ID>& exclude) const;

    // Creates new module with given ID and angles. Creates an empty set of edges corresponding to the module.
    void addModule(double alpha, double beta, double gamma, ID id);

    // Adds given edge to given modules if both docks are free.
    bool addEdge(const Edge& edge);
    bool removeEdge(const Edge& edge);

    bool findEdge(const Edge& edge) const;
    bool findConnection(const Edge& edge) const;

    void setFixed(ID initID, ShoeId initSide, const Matrix& initRotation);

    bool isValid();

    /**
     * \brief Computes matrices for all shoes.
     * 
     * This method is called in method isValid.
     * 
     * User should call this if they create new configuration or they change 
     * some angles and they don't call method isValid.
     * 
     * \return `false` if there is inconsistency in computing matrices,
     * i.e. matrix for one shoe was computed two different ways and 
     * the results aren't equal
     */
    bool computeMatrices();
    
    /**
     * \brief Computes matrix for shoe at the end of \p edge from shoe at the
     * begining of \p edge.
     * 
     * For example, module with id = 1 is connected from shoe A to shoe B of 
     * module with id = 2. It takes matrix of shoe A of module 1 and computes
     * matrix of shoe B of module 2 from that.
     * 
     * If neither matrix of shoes of the first module is computed, error is
     * thrown.
     * 
     * \return Matrix of the shoe connected to the end of \p edge
     */
    Matrix computeConnectedMatrix(Edge edge) const;

    /**
     * \brief Given \p id and \p side computes matrix for the other shoe 
     * of the given module.
     * 
     * \return Matrix of the other shoe of the given module
     */
    Matrix computeOtherSideMatrix(ID id, ShoeId side) const;

    bool connected() const;

    bool collisionFree() const;

    Vector massCenter() const;

    /**
     * \brief Tries to execute given \p action.
     * 
     * `False` is returned if either you try to add edge to already occupied
     * dock, erase non-existent edge or perform invalid rotation.
     * 
     * This function does NOT check if two modules collide during rotations.
     * For this functionality use `executeIfValid` from `Generators.h`
     * 
     * \return `True` if \p action was executed without problems.
     */
    bool execute(const Action& action);

    Action diff(const Configuration &other) const;

    bool operator==(const Configuration& other) const {
        return (modules == other.modules) &&
               (edges == other.edges);
    }

    bool operator!=(const Configuration &other) const {
        return !(*this == other);
    }

    friend ConfigurationHash;

    void clearEdges();

private:
    // Maps module ID to data about the module.
    ModuleMap modules;
    // Maps module ID to edges connected to the module.
    EdgeMap edges;

    MatrixMap matrices;

    // Fixed module side: all other modules are rotated with respect to this one.
    ID fixedId = 0;
    ShoeId fixedSide = A;
    Matrix fixedMatrix = identity;

    Value connectedVal = Value::Unknown;
    Value matricesVal = Value::Unknown;

    // If the given edge is in the configuration, delete it from both modules.
    bool eraseEdge(const Edge& edge);

    /**
     * \brief Computes differences between all joints of module with ID \p id
     * and \p otherModule
     * 
     * Given \p id and \p otherModule , `jointDiff` emplaces into \p rotations 
     * rotation for each joint whose value in module with ID \p id and \p otherModule
     * differs. 
     * 
     * If you would execute emplaced rotations on module with ID \p id, all joints
     * would have the same values as the joints of \p otherModule
     */
    void jointDiff(std::vector<Action::Rotate>& rotations, ID id, const Module& otherModule) const;

    /**
     * \brief Computes differences between all edges connected to module with ID \p id
     * in \p self and in \p other Configuration.
     * 
     * Given \p id and \p otherModule , `edgeDiff` emplaces into \p reconnections 
     * reconnection and disconnection for each dock so that if you would (and could)
     * execute emplaced actions, module with ID \p id would have same connections in
     * \p self and \p other -- with same IDs, same orientations and from same docks.
     */
    void edgeDiff(std::vector<Action::Reconnect>& reconnections, ID id, const Configuration &other) const;

    // Fix given module: one side is fixed. Fix all connected.
    bool computeMatricesRec(ID id, ShoeId side);
    void dfsID(ID id, std::unordered_set<ID>& seen) const;

    bool execute(const Action::Rotate& action);
    bool execute(const Action::Reconnect& action);
};

class ConfigurationHash {
public:
    std::size_t operator()(const Configuration& config) const {
        std::size_t res = 0;
        for (const auto& [id, mod] : config.modules)
            res += hashMod(mod);

        return res;
    }
private:
    std::size_t hashMod(const Module& mod) const {
        return static_cast<size_t>(
            mod.id * (13 * (mod.alpha + 90) + 17 * (mod.beta + 90) + 19 * mod.gamma));
    }
};

#endif //ROBOTS_CONFIGURATION_H
