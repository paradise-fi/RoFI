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

class DistributedModule {
public:
    DistributedModule(unsigned int id, std::ifstream &inStream, std::ifstream &trgStream);

    std::string printCurrModule(int step) const;
    std::string printTrgModule(int step) const;
    std::string printCurrConfiguration(int step) const;
    std::string printTrgConfiguration(int step) const;

    void reconfigurate();

private:
    DistributedModuleProperties currModule;
    DistributedModuleProperties trgModule;
    Configuration currConfiguration;

    Configuration trgConfiguration;

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

};


#endif //ROFI_DISTRIBUTEDMODULE_H
