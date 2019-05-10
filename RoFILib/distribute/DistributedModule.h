//
// Created by xmichel on 4/12/19.
//

#ifndef ROFI_DISTRIBUTEDMODULE_H
#define ROFI_DISTRIBUTEDMODULE_H

#include <mpi.h>

#include "DistributedModuleProperties.h"
#include "DistributedPrinter.h"
#include "DistributedReader.h"
#include "../reconfig/Algorithms.h"
#include "../IO.h"

enum Coordinates {
    Xcoor,
    Ycoor,
    Zcoor
};

class DistributedModule {
public:
    DistributedModule(unsigned int id, std::ifstream &inStream, std::ifstream &trgStream);

    std::string printCurrModule(int step) const;
    std::string printTrgModule(int step) const;
    std::string printCurrConfiguration(int step) const;
    std::string printTrgConfiguration(int step) const;

    void reconfigurate();

    void reconfigurate2();

    void setCoordinates(const std::array<int, 6> &coordinates) {
        currCoordinates = coordinates;
    }

    std::array<int, 6> getCoordinates() const {
        return currCoordinates;
    };

    int getCoordinate(Side side, Coordinates coordinate) {
        return currCoordinates.at(side * 3 + coordinate);
    }

private:
    DistributedModuleProperties currModule;
    DistributedModuleProperties trgModule;
    Configuration currConfiguration;
    Configuration trgConfiguration;

    std::array<int, 6> currCoordinates;
    bool coordinatesInit = false;

    void shareCurrConfigurations() { shareConfigurations(currModule, currConfiguration); }
    void shareTrgConfigurations() { shareConfigurations(trgModule, trgConfiguration); }

    EdgeList createEdgeListFromEdges(const std::vector<Edge> &edges, unsigned int id) const;

    void shareConfigurations(const DistributedModuleProperties &module, Configuration &configuration);

    std::string printModule(const DistributedModuleProperties &module, int step) const;

    void executeDiff(const Action &action, int step);

    std::vector<Action> createDiffs(const std::vector<Configuration> &path) const;

    void optimizePath(std::vector<Action> &path) const;

    bool joinActions(std::vector<Action> &path) const;

    void swapActions(std::vector<Action> &path) const;

    void shareCoordinates();

};

inline bool compareEdge(const Edge edge1, const Edge edge2) {
    return edge1.id2() < edge2.id2() || (edge1.id2() == edge2.id2() && edge1.side2() < edge2.side2());
}

#endif //ROFI_DISTRIBUTEDMODULE_H
