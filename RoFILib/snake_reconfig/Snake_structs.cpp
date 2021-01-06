#include "Snake_structs.h"

/* * * * * *
 * Scores  *
 * * * * * */

double AwayFromMassScore::operator()(const Configuration& config) const {
    Vector mass = config.massCenter();
    double sum = 0;
    for (const auto& [id, _m] : config.getMatrices()) {
        Vector moduleMass = config.getModuleMass(id);
        double dist = distance(mass, moduleMass);
        dist = dist == 0 ? 0.001 : dist;
        sum += 1/dist;
    }
    return sum;
}

double AwayFromRootScore::operator()(const Configuration& config) const {
    ID fixedId = config.getFixedId();
    Vector root = config.getModuleMass(fixedId);
    double sum = 0;
    for (const auto& [id, _m] : config.getMatrices()) {
        if (id == fixedId)
            continue;
        Vector moduleMass = config.getModuleMass(id);
        double dist = distance(root, moduleMass);
        dist = dist == 0 ? 0.001 : dist;
        sum += 1/dist;
    }
    return sum;
}

double EdgeSpaceScore::operator()(const Configuration& config) const {
    double score = 0;
    for (const auto& [id, ms] : config.getMatrices()) {
        if (_subtrees.find(id) != _subtrees.end())
            continue;
        for (const auto& m : ms) {
            auto dist = distFromVec(m, _mass);
            dist = dist == 0 ? 0.001 : dist;
            score += 1 / dist;
        }
    }
    return score;
}

double FurthestPointsScore::operator()(const Configuration& config) {
    double max = -1;
    const auto& matrices = config.getMatrices();
    for (auto it1 = matrices.begin(); it1 != matrices.end(); ++it1) {
        for (auto it2 = matrices.begin(); it2 != matrices.end(); ++it2) {
            if (it1 == it2)
                continue;
            Vector mass1 = config.getModuleMass(it1->first);
            Vector mass2 = config.getModuleMass(it2->first);
            double dist = sqDistance(mass1, mass2);
            if (dist > max)
                max = dist;
        }
    }
    return 1/max;
};

unsigned DistFromConnScore::getPenalty(const Configuration& config, const Matrix& conn, ID ignore) {
    unsigned penalty = 0;
    for (const auto& [id, ms] : config.getMatrices()) {
        if (id == ignore)
            continue;
        for (const auto& m : ms) {
            auto dist = centerSqDistance(m, conn);
            if (dist < 1)
                penalty += 10;
            else if (dist <= 3)
                penalty += 1;
        }
    }
    return penalty;
}

double DistFromConnScore::distFromConn(const Configuration& config, const Edge& connection) {
    if (config.findEdge(connection))
        return 0;
    const auto& realPos = config.getMatrices().at(connection.id2()).at(connection.side2());
    auto wantedPos = config.computeConnectedMatrix(connection);
    unsigned penalty = getPenalty(config, wantedPos, connection.id2());
    return penalty * sqDistance(realPos, wantedPos);
}

/* * * * * * * * * *
 * SpaceGridScore  *
 * * * * * * * * * */

SpaceGridScore::SpaceGridScore(const Configuration& init)
: _configuration(&init)
, _module_count(init.getIDs().size())
, _side_size(4*_module_count+1)
, _grid_size(_side_size * _side_size * _side_size)
, _grid(_grid_size, _EMPTY) {
}

SpaceGridScore::SpaceGridScore(unsigned module_count)
: _configuration()
, _module_count(module_count)
, _side_size(4*module_count+1)
, _grid_size(_side_size * _side_size * _side_size)
, _grid(_grid_size, _EMPTY) {
}

int SpaceGridScore::getFreeness() {
    _fillGrid();
    const auto& matrices = _configuration->getMatrices();
    static const std::vector<int> diff = {-1, 1};
    int freeness = 0;
    for (const auto& [id, matrix] : matrices) {
        for (const auto& side : matrix) {
            int x,y,z;
            std::tie(x,y,z) = tuple_center(side);
            for (int d : diff) {
                freeness += _add_score(x+d,y,z);
                freeness += _add_score(x,y+d,z);
                freeness += _add_score(x,y,z+d);
            }
        }
    }
    _cleanGrid();
    return freeness;
}

void SpaceGridScore::printGrid() const {
    int free = 0;
    int full = 0;
    int counted = 0;
    for (const auto& g : _grid) {
        if (g == _EMPTY)
            ++free;
        else if (g == _COUNTED)
            ++counted;
        else
            ++full;
    }
    std::cout << "Free: " << free << "\tCounted: " << counted << "\tFull: " << full << std::endl;
}

void SpaceGridScore::printCenters() const {
    const auto& matrices = _configuration->getMatrices();
    for (const auto& [id, matrix] : matrices) {
        for (const auto& side : matrix) {
            int x,y,z;
            std::tie(x,y,z) = tuple_center(side);
            std::cout << x << "\t" << y << "\t" << z << std::endl;
        }
    }
}

unsigned int SpaceGridScore::_getIndex(int x, int y, int z) const {
    if(!isInGrid(x, y, z)) {
        throw std::out_of_range("Coordinates out of range");
    }

    x += 2 * _module_count;
    y += 2 * _module_count;
    z += 2 * _module_count;
    return x * _side_size * _side_size + y * _side_size + z;
}

std::tuple<int,int,int> SpaceGridScore::_getCoordinates(unsigned int i) const {
    if(i >= _grid_size) {
        throw std::out_of_range("Index out of range");
    }

    int z = i % _side_size;
    i = (i - z) / _side_size;
    int y = i % _side_size;
    i = (i - y) / _side_size;
    int x = i;

    x -= 2 * _module_count;
    y -= 2 * _module_count;
    z -= 2 * _module_count;
    return std::make_tuple(x,y,z);
}

void SpaceGridScore::_fillGrid() {
    const auto& matrices = _configuration->getMatrices();
    for (const auto& [id, matrix] : matrices) {
        for (const auto& side : matrix) {
            int x,y,z;
            std::tie(x,y,z) = tuple_center(side);
            setCell(x, y, z, id);
        }
    }
}

void SpaceGridScore::_cleanGrid() {
    const auto& matrices = _configuration->getMatrices();
    static const std::vector<int> diff = {-1, 1};
    for (const auto& [id, matrix] : matrices) {
        for (const auto& side : matrix) {
            int x,y,z;
            std::tie(x,y,z) = tuple_center(side);
            for (int d : diff) {
                _clean(x+d,y,z);
                _clean(x,y+d,z);
                _clean(x,y,z+d);
            }
        }
    }
}

int SpaceGridScore::_add_score(int x, int y, int z) {
    if (!isInGrid(x, y, z))
        return 1;

    if (getCell(x, y, z) == _EMPTY) {
        setCell(x, y, z, _COUNTED);
        return 1;
    }
    return 0;
}

/* * * * * * *
 * MakeStar  *
 * * * * * * */

MakeStar::MakeStar(const Configuration& init, ID root)
: mass(init.massCenter())
, config(init)
, dists()
, seen() {
    seen.insert(root);
    for (const auto& [id, ms] : init.getMatrices()) {
        dists.emplace(id, std::array<double,2>{0, 0});
        double currDist0 = sqDistVM(ms[0], mass);
        double currDist1 = sqDistVM(ms[1], mass);
        dists[id][0] = currDist0;
        dists[id][1] = currDist1;
    }
}

std::vector<Edge> MakeStar::operator()(std::stack<ID>& stack, ID curr) {
    std::vector<Edge> nEdges = config.getEdges(curr, seen);
    std::sort(nEdges.begin(), nEdges.end(), [&](const Edge& a, const Edge& b){
        return dists[a.id2()][a.side2()] > dists[b.id2()][a.side2()];
    });
    std::vector<Edge> fEdges;
    for (const auto& e : nEdges) {
        if (seen.find(e.id2()) != seen.end())
            continue;
        seen.insert(e.id2());
        stack.push(e.id2());
        fEdges.emplace_back(e);
    }
    return fEdges;
}
