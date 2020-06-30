#include "Configuration.h"
#include <stdexcept>
#include <queue>

static const double threshold = 0.0001;

/* * * * * *
 * MODULE  *
 * * * * * */

double Module::getJoint(Joint a) const {
    switch (a) {
        case Joint::Alpha:
            return alpha;
        case Joint::Beta:
            return beta;
        case Joint::Gamma:
            return gamma;
        default:
            throw std::invalid_argument("Passed unknown joint to `Module::getJoint`");
    }
}

bool Module::rotateJoint(Joint joint, double val) {
    double res = 0;
    switch (joint) {
        case Joint::Alpha:
            res = alpha + val;
            break;
        case Joint::Beta:
            res = beta + val;
            break;
        case Joint::Gamma:
            res = gamma + val;
            break;
        default:
            throw std::invalid_argument("Passed unknown joint to `Module::rotateJoint`");
    }
    return setJoint(joint, res);
}

double clampGamma(double val) {
    if (val > 180)
        return 360 - val;

    if (val <= -180)
        return 360 + val;

    return val;
}

bool Module::setJoint(Joint joint, double val) {
    switch (joint) {
        case Joint::Alpha:
            if ((val > 90) || (val < -90))
                return false;

            alpha = val;
            return true;

        case Joint::Beta:
            if ((val > 90) || (val < -90))
                return false;

            beta = val;
            return true;

        case Joint::Gamma:
            gamma = clampGamma(val);
            return true;

        default:
            throw std::invalid_argument("Passed unknown joint to `Module::setJoint`");
    }
}

bool Module::operator==(const Module& other) const {
    return (id == other.id) &&
            (std::abs(alpha - other.alpha) < threshold) &&
            (std::abs(beta - other.beta) < threshold) &&
            (std::abs(gamma - other.gamma) < threshold);
}

/* * * * *
 * EDGE  *
 * * * * */

bool Edge::operator==(const Edge& other) const {
    return (id1_ == other.id1_) &&
            (id2_ == other.id2_) &&
            (side1_ == other.side1_) &&
            (side2_ == other.side2_) &&
            (dock1_ == other.dock1_) &&
            (dock2_ == other.dock2_) &&
            (ori_ == other.ori_);
}

inline std::array<unsigned, 5> nextEdgeArray(const std::array<unsigned, 5> &edge) {
    const static std::array<unsigned, 5> size = {2,3,4,3,2};
    std::array<unsigned, 5> res = edge;

    for (unsigned i = 0; i < 5; ++i) {
        if (res[i] + 1 < size[i]) {
            res[i] += 1;
            break;
        }
        res[i] = 0;
    }

    return res;
}

std::optional<Edge> nextEdge(const Edge& edge) {
    const static std::array<unsigned, 5> init = {0, 0, 0, 0, 0};
    std::array<unsigned, 5> arr = {edge.side1(), edge.dock1(), edge.ori(), edge.dock2(), edge.side2()};
    auto next = nextEdgeArray(arr);

    if (next == init)
        return std::nullopt;

    return {Edge(edge.id1(),
                static_cast<ShoeId>(next[0]),
                static_cast<ConnectorId>(next[1]),
                next[2],
                static_cast<ConnectorId>(next[3]),
                static_cast<ShoeId>(next[4]),
                edge.id2())};
}

unsigned edgeIndex(const Edge& edge) {
    return edge.side1() * 3 + edge.dock1();
}

unsigned edgeIndex(ShoeId shoe, ConnectorId dock) {
    return shoe * 3 + dock;
}

/* * * * * *
 * ACTIONS *
 * * * * * */

bool Action::Rotate::operator==(const Rotate &other) const {
    return _id == other._id &&
            _joint == other._joint &&
            std::abs(_angle - other._angle) < threshold;
}

bool Action::Reconnect::operator==(const Reconnect &other) const {
    return _add == other._add &&
            _edge == other._edge;
}

Action Action::divide(double factor) const {
    std::vector<Rotate> division;
    for (const Action::Rotate& rotate : _rotations)
        division.emplace_back(rotate._id, rotate._joint, rotate._angle * factor);

    return {division, _reconnections};
}

bool unique(const std::vector<Action::Rotate>& a) {
    std::unordered_map<ID, std::array<bool, 3>> moved;

    for (const auto& rot : a) {
        if (moved.find(rot.id()) == moved.end())
            moved[rot.id()] = {false, false, false};

        if (moved[rot.id()][rot.joint()])
            return false;

        moved[rot.id()][rot.joint()] = true;
    }

    return true;
}

/* * * * * * * * * * * * * *
 * TRANSFORMATION MATRICES *
 * * * * * * * * * * * * * */

Matrix transformJoint(double alpha, double beta, double gamma) {
    return rotate(alpha, X) * rotate(gamma, Z) * translate(Z) * rotate(M_PI, Y) * rotate(-beta, X);
}

Matrix transformConnection(ConnectorId d1, int ori, ConnectorId d2) {
    static const std::array<Matrix, 3> dockFaceUp = {
            rotate(M_PI, Z) * rotate(-M_PI/2, Y), // XPlus
            rotate(M_PI, Z) * rotate(M_PI/2, Y),  // XMinus
            identity  // ZMinus
    };

    static const std::array<Matrix, 3> faceToDock = {
            rotate(M_PI, Z) * rotate(M_PI/2, Y), // XPlus
            rotate(M_PI, Z) * rotate(-M_PI/2, Y),   // XMinus
            identity // ZMinus
    };

    return faceToDock[d1] * rotate(ori * M_PI/2, Z) * translate(-Z) * dockFaceUp[d2] * rotate(M_PI, X);
}

/* * * * * * * * *
 * CONFIGURATION *
 * * * * * * * * */

Module& Configuration::getModule(ID id) {
    auto it = modules.find(id);
    if(it == modules.end())
        throw std::out_of_range("No module with this id: " + std::to_string(id));

    return it->second;
}

const Module& Configuration::getModule(ID id) const {
    auto it = modules.find(id);
    if(it == modules.end())
        throw std::out_of_range("No module with this id: " + std::to_string(id));

    return it->second;
}

std::vector<ID> Configuration::getIDs() const {
    std::vector<ID> ids;
    ids.reserve(modules.size());
    for (auto& [id, m] : modules)
        ids.push_back(id);

    return ids;
}

std::vector<Edge> Configuration::getEdges(ID id) const {
    std::vector<Edge> res;
    for (auto& e : edges.at(id)) {
        if (e.has_value())
            res.push_back(e.value());
    }
    return res;
}

std::vector<Edge> Configuration::getEdges(ID id, const std::unordered_set<ID>& exclude) const {
    std::vector<Edge> res;
    for (auto& e : edges.at(id)) {
        if (e.has_value() && exclude.find(e->id2()) == exclude.end())
            res.push_back(e.value());
    }
    return res;
}


const std::unordered_map<ID, std::array<std::optional<Edge>, 6>> Configuration::getSpanningSucc() const {
    return spanningSucc;
}

const std::unordered_map<ID, unsigned int>& Configuration::getSpanningSuccCount() const {
    return spanningSuccCount;
}

const std::unordered_map<ID, std::optional<std::pair<ID, ShoeId>>>& Configuration::getSpanningPred() const {
    return spanningPred;
}


void Configuration::addModule(double alpha, double beta, double gamma, ID id) {
    modules.emplace(std::piecewise_construct,
                    std::forward_as_tuple(id),  // args for key
                    std::forward_as_tuple(alpha, beta, gamma, id));
    edges.emplace(std::piecewise_construct,
                    std::forward_as_tuple(id),  // args for key
                    std::forward_as_tuple());
    if ((modules.size() == 1) || (id < fixedId)) {
        fixedId = id;
        matricesVal = Value::False;
    } else {
        matricesVal = Value::Unknown;
    }
    if (modules.size() > 1) {
        connectedVal = Value::False;
    }
    spanningTreeComputed = false;
    static const std::array<std::optional<Edge>, 6> emptyArray = {};
    spanningSucc.emplace(id, emptyArray);
    spanningSuccCount.emplace(id, 0);
    spanningPred.emplace(id, std::nullopt);
    spanningCross.emplace(id, emptyArray);
    isMatrixComputed.emplace(id, std::array<bool,2>{false, false});
    isMatrixUpdated.emplace(id, std::array<bool,2>{false, false});
    isChecked.emplace(id, false);
}

bool Configuration::addEdge(const Edge& edge) {
    // Finds edges for both modules.
    EdgeList& set1 = edges.at(edge.id1());
    EdgeList& set2 = edges.at(edge.id2());

    unsigned setIndex1 = edgeIndex(edge);
    unsigned setIndex2 = edgeIndex(reverse(edge));
    if (set1[setIndex1].has_value() || set2[setIndex2].has_value())
        return false;

    set1[setIndex1] = edge;
    auto revEdge = reverse(edge);
    set2[setIndex2] = revEdge;
    if (connectedVal == Value::False)
        connectedVal = Value::Unknown;
    if (spanningTreeComputed) {
        EdgeList& spanningSet1 = spanningCross.at(edge.id1());
        EdgeList& spanningSet2 = spanningCross.at(edge.id2());

        unsigned spanningSetIndex1 = edgeIndex(edge);
        unsigned spanningSetIndex2 = edgeIndex(reverse(edge));

        spanningSet1[spanningSetIndex1] = edge;
        spanningSet2[spanningSetIndex2] = revEdge;
    }
    return true;
}

bool Configuration::removeEdge(const Edge& edge) {
    // Finds edges for both modules.
    EdgeList& set1 = edges.at(edge.id1());
    EdgeList& set2 = edges.at(edge.id2());

    unsigned setIndex1 = edgeIndex(edge);
    unsigned setIndex2 = edgeIndex(reverse(edge));
    if (!set1[setIndex1].has_value() || !set2[setIndex2].has_value())
        return false;

    if (set1[setIndex1].value() != edge || set2[setIndex2].value() != reverse(edge))
        return false;

    set1[setIndex1] = {};
    set2[setIndex2] = {};

    if (!spanningTreeComputed) {
        if (connectedVal == Value::True)
            connectedVal = Value::Unknown;
        return true;
    }

    if (spanningPred[edge.id1()].has_value()
        && spanningPred[edge.id1()].value().first == edge.id2()) {
        removeSpanningEdge(reverse(edge));
    } else if (spanningPred[edge.id2()].has_value()
        && spanningPred[edge.id2()].value().first == edge.id1()) {
        removeSpanningEdge(edge);
    } else {
        EdgeList& spanningSet1 = spanningCross.at(edge.id1());
        EdgeList& spanningSet2 = spanningCross.at(edge.id2());

        unsigned spanningSetIndex1 = edgeIndex(edge);
        unsigned spanningSetIndex2 = edgeIndex(reverse(edge));

        spanningSet1[spanningSetIndex1] = {};
        spanningSet2[spanningSetIndex2] = {};
    }

    return true;
}

void Configuration::removeSpanningEdge(const Edge& edge) {
    unsigned setIndex = edgeIndex(edge);
    spanningSucc[edge.id1()][setIndex] = {};
    spanningSuccCount[edge.id1()] -= 1;

    spanningPred[edge.id2()] = {};

    connectedVal = Value::Unknown;
    spanningTreeComputed = false;
    return;
}

bool Configuration::findEdge(const Edge& edge) const {
    unsigned idx = edgeIndex(edge);
    return edges.at(edge.id1())[idx].has_value();
}

bool Configuration::findConnection(const Edge& edge) const {
    unsigned idx = edgeIndex(edge);
    const auto& edgeCandidate = edges.at(edge.id1())[idx];
    return edgeCandidate.has_value() && edgeCandidate.value() == edge;
}

void Configuration::setFixed(ID initID, ShoeId initSide, const Matrix& initRotation) {
    fixedId = initID;
    fixedSide = initSide;
    fixedMatrix = initRotation;
    spanningTreeComputed = false;
    matricesVal = Value::False;
}

bool Configuration::isValid() {
    if (connectedVal == Value::Unknown)
        connectedVal = connected() ? Value::True : Value::False;

    return (connectedVal == Value::True) && computeMatrices() && collisionFree();
}

bool Configuration::computeMatrices() {
    if (empty())
        return true;
    if (matricesVal == Value::True)
        return true;

    if (!spanningTreeComputed && !computeSpanningTree())
        return false;

    bool recomputeAll = false;
    if (matricesVal == Value::False || !isMatrixComputed[fixedId][fixedSide]) {
        matrices[fixedId][fixedSide] = fixedMatrix;
        recomputeAll = true;
        isMatrixComputed[fixedId][fixedSide] = true;
        isMatrixUpdated[fixedId][fixedSide] = true;
    } else {
        isMatrixUpdated[fixedId][fixedSide] = false;
    }

    std::queue<std::tuple<ID, ShoeId, bool>> bag;
    bag.emplace(fixedId, fixedSide, recomputeAll);
    isChecked[fixedId] = false;

    ID currId;
    ShoeId currShoe;
    bool recompute;
    std::array<bool, 2> recomputeShoe;
    while (!bag.empty()) {
        std::tie(currId, currShoe, recompute) = bag.front();
        recomputeShoe[currShoe] = recompute;
        bag.pop();
        auto& matrixCurr = matrices.at(currId);
        ShoeId otherShoe = currShoe == A ? B : A;
        if (recompute || !isMatrixComputed[currId][otherShoe]) {
            matrixCurr[otherShoe] = computeOtherSideMatrix(currId, currShoe);
            isMatrixComputed[currId][otherShoe] = true;
            isMatrixUpdated[currId][otherShoe] = true;
            recomputeShoe[otherShoe] = true;
        } else {
            isMatrixUpdated[currId][otherShoe] = false;
            recomputeShoe[otherShoe] = false;
        }

        for (const auto& edgeOpt : spanningSucc.at(currId)) {
            if (!edgeOpt.has_value())
                continue;

            const Edge& edge = edgeOpt.value();
            ID nextId = edge.id2();
            ShoeId nextShoe = edge.side2();
            bool currRecompute = recomputeShoe[edge.side1()] || !isMatrixComputed[nextId][nextShoe];
            if (currRecompute) {
                matrices[nextId][nextShoe] = computeConnectedMatrix(edge);
                isMatrixComputed[nextId][nextShoe] = true;
                isMatrixUpdated[nextId][nextShoe] = true;
            } else {
                isMatrixUpdated[nextId][nextShoe] = false;
            }

            bag.emplace(nextId, nextShoe, currRecompute);
            isChecked[nextId] = false;
        }
    }
    return checkConsistency();
}

bool Configuration::checkConsistency() {
    for (const auto& [id, updatedShoe] : isMatrixUpdated) {
        for (int side = 0; side < 2; ++side) {
            if (!updatedShoe[side])
                continue;
            for (const auto& crossOpt : spanningCross[id]) {
                if (!crossOpt.has_value())
                    continue;
                const auto& edge = crossOpt.value();
                if (edge.side1() != side)
                    continue;
                if (isChecked[edge.id2()])
                    continue;
                if (!isMatrixUpdated[edge.id2()][edge.side2()]) {
                    // here I assume that if one shoe updated its matrix,
                    // all other shoes has to as well -- but is it true?
                    matricesVal = Value::False;
                    return false;
                }
                auto matrixCheck = computeConnectedMatrix(edge);
                const auto& matrixComputed = matrices[edge.id2()][edge.side2()];
                if (equals(matrixCheck, matrixComputed))
                    continue;
                matricesVal = Value::False;
                return false;
            }
            isChecked[id] = true;
        }
    }
    matricesVal = Value::True;
    return true;
}

Matrix Configuration::computeOtherSideMatrix(ID id, ShoeId side) const {
    auto const& mod = modules.at(id);
    auto const& matrix = matrices.at(id)[side];
    double alpha = 2 * M_PI * mod.alpha / 360;
    double beta = 2 * M_PI * mod.beta / 360;
    double gamma = 2 * M_PI * mod.gamma / 360;

    if (side == B)
        std::swap(alpha, beta);

    return matrix * transformJoint(alpha, beta, gamma);
}

Matrix Configuration::computeConnectedMatrix(Edge edge) const {
    auto const& matrix = matrices.at(edge.id1())[edge.side1()];
    return matrix * transformConnection(edge.dock1(), edge.ori(), edge.dock2());
}

bool Configuration::connected() {
    if (spanningTreeComputed || connectedVal == Value::True)
        return true;
    if (connectedVal == Value::False)
        return false;
    return computeSpanningTree();
}

bool Configuration::connected() const {
    if (spanningTreeComputed || connectedVal == Value::True) {
        return true;
    }
    if (connectedVal == Value::False)
        return false;
    std::unordered_set<ID> seen;
    seen.insert(fixedId);
    dfsID(fixedId, seen);
    return seen.size() == modules.size();
}

void Configuration::dfsID(ID id, std::unordered_set<ID>& seen) const {
    for (const std::optional<Edge> edgeOpt : edges.at(id)) {
        if (!edgeOpt.has_value())
            continue;
        ID nextId = edgeOpt.value().id2();
        if (seen.find(nextId) == seen.end()) {
            seen.insert(nextId);
            dfsID(nextId, seen);
        }
    }
}

bool Configuration::collisionFree() {
    if (matricesVal != Value::True && !computeMatrices())
        return false;
    for (auto it1 = matrices.begin(); it1 != matrices.end(); ++it1) {
        for (auto it2 = it1; it2 != matrices.end(); ++it2) {
            const auto& ms1 = it1->second;
            const auto& ms2 = it2->second;
            if (it1 == it2) {
                if (centerSqDistance(ms1[A], ms1[B]) < 1)
                    return false;
                continue;
            }
            if (centerSqDistance(ms1[A], ms2[A]) < 1 ||
                centerSqDistance(ms1[B], ms2[B]) < 1 ||
                centerSqDistance(ms1[B], ms2[A]) < 1 ||
                centerSqDistance(ms1[A], ms2[B]) < 1)
                return false;
        }
    }
    return true;
}

bool Configuration::collisionFree() const {
    if (matricesVal != Value::True)
        return false;
    for (auto it1 = matrices.begin(); it1 != matrices.end(); ++it1) {
        for (auto it2 = it1; it2 != matrices.end(); ++it2) {
            const auto& ms1 = it1->second;
            const auto& ms2 = it2->second;
            if (it1 == it2) {
                if (centerSqDistance(ms1[A], ms1[B]) < 1)
                    return false;
                continue;
            }
            if (centerSqDistance(ms1[A], ms2[A]) < 1 ||
                centerSqDistance(ms1[B], ms2[B]) < 1 ||
                centerSqDistance(ms1[B], ms2[A]) < 1 ||
                centerSqDistance(ms1[A], ms2[B]) < 1)
                return false;
        }
    }
    return true;
}

Vector Configuration::massCenter() const {
    Vector mass({0,0,0,1});
    for (const auto& [id, ms] : matrices) {
        mass += center(ms[A]);
        mass += center(ms[B]);
    }
    mass /= modules.size()*2;
    return mass;
}

bool Configuration::execute(const Action& action) {
    bool ok = true;
    for (const Action::Rotate rot : action.rotations())
        ok &= execute(rot);

    for (const Action::Reconnect rec : action.reconnections())
        ok &= execute(rec);

    return ok;
}

void Configuration::jointDiff(std::vector<Action::Rotate>& rotations, ID id, const Module& otherModule) const {
    const Module &module = modules.at(id);
    for (Joint joint : { Alpha, Beta, Gamma }) {
        auto val = otherModule.getJoint(joint) - module.getJoint(joint);
        if (joint == Gamma) {
            val = clampGamma(val);
        }
        if (val != 0) {
            rotations.emplace_back(id, joint, val);
        }
    }
}

void Configuration::edgeDiff(std::vector<Action::Reconnect>& reconnections, ID id, const Configuration &other) const {
    const EdgeList &edgeList = edges.at(id);
    const EdgeList &otherEdgeList = other.edges.at(id);
    for (unsigned int i = 0; i < 6; i++) {
        auto &edge = edgeList.at(i);
        auto &otherEdge = otherEdgeList.at(i);

        if (edge == otherEdge)
            continue;

        if (!edge.has_value()) {
            const Edge &edge1 = otherEdge.value();
            if (edge1.id1() < edge1.id2())
                reconnections.emplace_back(true, otherEdge.value());
        } else if (!otherEdge.has_value()) {
            const Edge &edge1 = edge.value();
            if (edge1.id1() < edge1.id2())
                reconnections.emplace_back(false, edge.value());
        } else {
            const Edge &edge1 = otherEdge.value();
            if (edge1.id1() < edge1.id2()) {
                reconnections.emplace_back(true, otherEdge.value());
                reconnections.emplace_back(false, edge.value());
            }
        }
    }
}

Action Configuration::diff(const Configuration &other) const {
    std::vector<Action::Rotate> rotations;
    std::vector<Action::Reconnect> reconnections;

    ModuleMap moduleMap = other.getModules();
    for (const auto& [id, otherModule] : moduleMap) {
        if (modules.find(id) == modules.end())
            continue;

        jointDiff(rotations, id, otherModule);
        edgeDiff(reconnections, id, other);
    }

    return {rotations, reconnections};
}

void Configuration::clearEdges() {
    for (auto& [id, el] : edges) {
        for (auto& opt : el)
            opt = std::nullopt;
    }
    spanningTreeComputed = false;
    if (modules.size() > 1) {
        connectedVal = Value::False;
        matricesVal = Value::False;
    }
}

bool Configuration::execute(const Action::Rotate& action) {
    if (!spanningTreeComputed && !computeSpanningTree())
        return false;
    bool res = modules.at(action.id()).rotateJoint(action.joint(), action.angle());
    if (res) {
        matricesVal = Value::Unknown;
        const auto& spannPred = spanningPred.at(action.id());
        if (!spannPred.has_value()) {
            auto otherSide = fixedSide == A ? B : A;
            isMatrixComputed[action.id()][otherSide] = false;
        } else {
            auto otherSide = spannPred.value().second == A ? B : A;
            isMatrixComputed[action.id()][otherSide] = false;
        }
    }
    return res;
}

bool Configuration::execute(const Action::Reconnect& action) {
    if (action.add())
        return addEdge(action.edge());
    return removeEdge(action.edge());
}

void Configuration::spanningClearId(ID id) {
    spanningPred.at(id) = std::nullopt;
    for (auto& succOpt : spanningSucc.at(id)) {
        succOpt = {};
    }
    spanningSuccCount.at(id) = 0;
    for (auto& crossOpt : spanningCross.at(id)) {
        crossOpt = {};
    }
}

bool Configuration::computeSpanningTree() {
    if (connectedVal == Value::False) {
        spanningTreeComputed = false;
        return false;
    }
    std::unordered_set<ID> seen;
    std::queue<ID> bag;

    bag.push(fixedId);
    seen.insert(fixedId);
    spanningClearId(fixedId);

    while (!bag.empty()) {
        auto currId = bag.front();
        bag.pop();
        for (const auto& edgeOpt : edges.at(currId)) {
            if (!edgeOpt.has_value())
                continue;
            const auto& edge = edgeOpt.value();
            ID nextId = edge.id2();
            if (seen.find(nextId) != seen.end()) {
                if (!spanningPred[currId].has_value() || spanningPred[currId].value().first != nextId) {
                    unsigned spanningSetIndex = edgeIndex(edge);
                    spanningCross[currId][spanningSetIndex] = edge;
                }
                continue;
            }
            bag.push(nextId);
            seen.insert(nextId);
            spanningClearId(nextId);
            spanningPred.at(nextId) = {std::make_pair(currId, edge.side2())};
            unsigned setIndex = edgeIndex(edge);
            spanningSucc[currId][setIndex] = edgeOpt;
            spanningSuccCount[currId] += 1;
        }
    }

    spanningTreeComputed = seen.size() == modules.size();
    connectedVal = spanningTreeComputed ? Value::True : Value::False;
    return spanningTreeComputed;
}