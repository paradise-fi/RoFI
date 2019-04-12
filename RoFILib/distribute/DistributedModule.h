//
// Created by xmichel on 4/12/19.
//

#ifndef ROFI_DISTRIBUTEDMODULE_H
#define ROFI_DISTRIBUTEDMODULE_H

#include <mpi.h>

#include "DistributedModuleProperties.h"
#include "../Reader.h"
#include "../Printer.h"

class DistributedModule {
public:
    DistributedModule(unsigned int id, std::ifstream &initConfigStream, std::ifstream &trgConfigStream);

    //TODO add constructor with only one module properties

    std::string printCurrModule() const;
    std::string printTrgModule() const;
    std::string printCurrConfiguration() const;
    std::string printTrgConfiguration() const;

    void shareCurrConfigurations() { shareConfigurations(currModule, currConfiguration); }
    void shareTrgConfigurations() { shareConfigurations(trgModule, trgConfiguration); }

private:
    DistributedModuleProperties currModule;
    DistributedModuleProperties trgModule;

    Configuration currConfiguration;
    Configuration trgConfiguration;

    EdgeList createEdgeListFromEdges(const std::vector<Edge> &edges, unsigned int id) const;

    void shareConfigurations(const DistributedModuleProperties &module, Configuration &configuration);

    std::string printModule(const DistributedModuleProperties &module) const;
};


#endif //ROFI_DISTRIBUTEDMODULE_H
