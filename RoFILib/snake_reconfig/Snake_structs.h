#pragma once

#include <Configuration.h>
#include <IO.h>
#include <cmath>

inline std::tuple<int, int, int> tuple_center(const Matrix &m) {
    auto v = center(m);
    return std::make_tuple(std::round(v(0)), std::round(v(1)), std::round(v(2)));
}


class SpaceGrid {
public:
    using Cell = int;
    const Cell _EMPTY = -1;
    const Cell _COUNTED = -2;

    SpaceGrid(const Configuration& init)
    : _configuration(&init)
    , _module_count(init.getIDs().size())
    , _side_size(4*_module_count+1)
    , _grid_size(_side_size * _side_size * _side_size)
    , _grid(_grid_size, _EMPTY) {
    }

    SpaceGrid(unsigned module_count)
    : _configuration()
    , _module_count(module_count)
    , _side_size(4*module_count+1)
    , _grid_size(_side_size * _side_size * _side_size)
    , _grid(_grid_size, _EMPTY) {
    } 

    Cell getCell(int x, int y, int z) const {
        return _grid[_getIndex(x, y, z)];
    }

    void setCell(int x, int y, int z, Cell cell) {
        _grid[_getIndex(x, y, z)] = cell;
    }

    bool isInGrid(int x, int y, int z) const {
    /*
        Grid accepts [-2*mc+1, 2*mc-1]
    */ 
        return _inGrid(x) && _inGrid(y) && _inGrid(z);
    }

    int getFreeness() {
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

    int getDist() {
        return _module_count * 8 + 2 - getFreeness();
    }

    void printGrid() const {
        int free = 0;
        int full = 0;
        int counted = 0;
        for (const auto& g : _grid) {
            if (g == _EMPTY) {
                ++free;
            } else if (g == _COUNTED) {
                ++counted;
            }
            else {
                ++full;
            }

        }
        std::cout << "Free: " << free << "\tCounted: " << counted << "\tFull: " << full << std::endl;
    }


    void printCenters() const {
        const auto& matrices = _configuration->getMatrices();
        for (const auto& [id, matrix] : matrices) {
            for (const auto& side : matrix) {
                int x,y,z;
                std::tie(x,y,z) = tuple_center(side);
                std::cout << x << "\t" << y << "\t" << z << std::endl;
            }
        }
    }

    void loadConfig(const Configuration& config) {
        _configuration = &config;
    }

private:
    const Configuration* _configuration;
    const int _module_count;
    const unsigned int _side_size;
    const unsigned int _grid_size;
    std::vector<Cell> _grid;

    unsigned int _getIndex(int x, int y, int z) const {
        if(!isInGrid(x, y, z)) {
            throw std::out_of_range("Coordinates out of range");
        }

        x += 2 * _module_count;
        y += 2 * _module_count;
        z += 2 * _module_count;
        return x * _side_size * _side_size + y * _side_size + z;
    }

    std::tuple<int,int,int> _getCoordinates(unsigned int i) const {
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

    bool _inGrid(int c) const {
        return 2 * _module_count > c && 2 * _module_count + c > 0;
    }

    void _fillGrid() {
        const auto& matrices = _configuration->getMatrices();
        for (const auto& [id, matrix] : matrices) {
            for (const auto& side : matrix) {
                int x,y,z;
                std::tie(x,y,z) = tuple_center(side);
                setCell(x, y, z, id);
            }
        }
    }

    void _cleanGrid() {
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

    void _clean(int x, int y, int z) {
        if (_inGrid(x) && _inGrid(y) && _inGrid(z)) {
            setCell(x, y, z, _EMPTY);
        }
    }

    int _add_score(int x, int y, int z) {
        if (!isInGrid(x, y, z)) {
            return 1;
        } 
        if (getCell(x, y, z) == _EMPTY) {
            setCell(x, y, z, _COUNTED);
            return 1;
        }
        return 0;
    }
};