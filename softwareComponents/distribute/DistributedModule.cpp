//
// Created by xmichel on 4/12/19.
//

#include "DistributedModule.h"

DistributedModule::DistributedModule(unsigned int id, std::ifstream &inStream, std::ifstream &trgStream)
        : currModule(id), trgModule(id) {
    if (!inStream.is_open() || !trgStream.is_open()) {
        return;
    }

    DistributedReader::readDistributedModule(inStream, currModule);
    DistributedReader::readDistributedModule(trgStream, trgModule);
}

std::string DistributedModule::printCurrModule(int step) const {
    return printModule(currModule, step);
}

std::string DistributedModule::printTrgModule(int step) const {
    return printModule(trgModule, step);
}

std::string DistributedModule::printModule(const DistributedModuleProperties &module, int step) const {
    std::stringstream out;
    out << DistributedPrinter::toString(module, step);

    for (auto &optEdge : module.getEdges()) {
        if (!optEdge.has_value()) {
            continue;
        }

        Edge &edge = optEdge.value();
        int id1 = edge.id1();
        int id2 = edge.id2();

        if ((id1 < id2 && module.getId() == id1) || (id1 > id2 && module.getId() == id2)) {
            out << DistributedPrinter::toString(edge, step);
        }
    }

    return out.str();
}
