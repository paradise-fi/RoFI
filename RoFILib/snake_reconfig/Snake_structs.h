#pragma once

#include <Configuration.h>
#include <Generators.h>
#include <IO.h>
#include <cmath>
#include <stack>


inline std::tuple<int, int, int> tuple_center(const Matrix &m) {
    auto v = center(m);
    return std::make_tuple(std::round(v(0)), std::round(v(1)), std::round(v(2)));
}

class SimpleNextGen {
public:
    void operator()(const Configuration& config, std::vector<Configuration>& res, unsigned step) const {
        simpleNext(config, res, step);
    }
};

class SimpleOnlyRotNextGen {
public:
    void operator()(const Configuration& config, std::vector<Configuration>& res, unsigned step) const {
        simpleOnlyRotNext(config, res, step);
    }
};

class SmartBisimpleOnlyRotGen {
public:
    void operator()(const Configuration& config, std::vector<Configuration>& res, unsigned step) const {
        smartBisimpleOnlyRotNext(config, res, step);
    }
};

class BiParalyzedGen {
public:
    BiParalyzedGen(const std::unordered_set<ID>& allowed)
    : _allowed(allowed) {}

    void operator()(const Configuration& config, std::vector<Configuration>& res, unsigned step) const {
        biParalyzedOnlyRotNext(config, res, step, _allowed);
    }

private:
    const std::unordered_set<ID>& _allowed;
};

class AwayFromMassScore {
public:
    double operator()(const Configuration& config) const;
};

class AwayFromRootScore {
public:
    double operator()(const Configuration& config) const;
};

class EdgeSpaceScore {
public:
    EdgeSpaceScore(const Vector& mass, const std::unordered_set<ID>& subtrees)
    : _mass(mass)
    , _subtrees(subtrees) {}

    double operator()(const Configuration& config) const;

private:
    const Vector& _mass;
    const std::unordered_set<ID>& _subtrees;
};

class FurthestPointsScore {
public:
    double operator()(const Configuration& config);
};

class DistFromConnScore {
public:
    DistFromConnScore(const Edge& conn)
    : _conn(conn)
    , _rev(reverse(conn)) {}

    double operator()(const Configuration& config) const {
        return distFromConn(config, _conn) + distFromConn(config, _rev);
    }

private:
    static unsigned getPenalty(const Configuration& config, const Matrix& conn, ID ignore);
    static double distFromConn(const Configuration& config, const Edge& connection);

    const Edge& _conn;
    const Edge _rev;
};


class SpaceGridScore {
public:
    using Cell = int;
    const Cell _EMPTY = -1;
    const Cell _COUNTED = -2;

    SpaceGridScore(const Configuration& init);
    SpaceGridScore(unsigned module_count);

    Cell getCell(int x, int y, int z) const { return _grid[_getIndex(x, y, z)]; }
    void setCell(int x, int y, int z, Cell cell) { _grid[_getIndex(x, y, z)] = cell; }
    /* Grid accepts [-2*mc+1, 2*mc-1] */
    bool isInGrid(int x, int y, int z) const { return _inGrid(x) && _inGrid(y) && _inGrid(z); }

    int getFreeness();

    int getDist() {
        return _module_count * 8 + 2 - getFreeness();
    }

    void printGrid() const;
    void printCenters() const;

    void loadConfig(const Configuration& config) {
        _configuration = &config;
    }

    double operator()(const Configuration& conf) {
        loadConfig(conf);
        return getDist();
    }

private:
    const Configuration* _configuration;
    const int _module_count;
    const unsigned int _side_size;
    const unsigned int _grid_size;
    std::vector<Cell> _grid;

    unsigned int _getIndex(int x, int y, int z) const;

    std::tuple<int,int,int> _getCoordinates(unsigned int i) const;

    bool _inGrid(int c) const {
        return 2 * _module_count > c && 2 * _module_count + c > 0;
    }

    void _fillGrid();
    void _cleanGrid();

    void _clean(int x, int y, int z) {
        if (_inGrid(x) && _inGrid(y) && _inGrid(z)) {
            setCell(x, y, z, _EMPTY);
        }
    }

    int _add_score(int x, int y, int z);
};

class MakeStar {
public:
    MakeStar(const Configuration& init, ID root);
    std::vector<Edge> operator()(std::stack<ID>& dfs_stack, ID curr);

private:
    const Vector mass;
    const Configuration& config;
    std::unordered_map<ID, std::array<double, 2>> dists;
    std::unordered_set<ID> seen;
};