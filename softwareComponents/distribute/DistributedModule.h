//
// Created by xmichel on 4/12/19.
//

#ifndef ROFI_DISTRIBUTEDMODULE_H
#define ROFI_DISTRIBUTEDMODULE_H

#include <mpi.h>

#include "DistributedModuleProperties.h"
#include "DistributedPrinter.h"
#include "DistributedReader.h"

class DistributedModule {
public:
    DistributedModule(unsigned int id, std::ifstream &inStream, std::ifstream &trgStream);

    std::string printCurrModule(int step) const;
    std::string printTrgModule(int step) const;

private:
    friend class AlgorithmFullConfiguration;
    friend class AlgorithmPartialConfiguration;
    DistributedModuleProperties currModule;
    DistributedModuleProperties trgModule;

    std::string printModule(const DistributedModuleProperties &module, int step) const;

};


#endif //ROFI_DISTRIBUTEDMODULE_H
