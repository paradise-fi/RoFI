//
// Created by xmichel on 4/12/19.
//

#include "DistributedModule.h"

DistributedModule::DistributedModule(unsigned int id, std::ifstream &initConfigStream,
                                     std::ifstream &trgConfigStream) {
    if (!initConfigStream.is_open() || !trgConfigStream.is_open()) {
        return;
    }

    Reader reader;
    Configuration initConfig;
    reader.read(initConfigStream, initConfig);

    const Module &initModuleToCopy = initConfig.getModules().at(id);
    currModule = DistributedModuleProperties(initModuleToCopy.getJoint(Joint::Alpha)
            , initModuleToCopy.getJoint(Joint::Beta)
            , initModuleToCopy.getJoint(Joint::Gamma)
            , static_cast<ID>(id));

    currModule.setEdges(createEdgeListFromEdges(initConfig.getEdges(id), id));

    Configuration trgConfig;
    reader.read(trgConfigStream, trgConfig);

    const Module &trgModuleToCopy = trgConfig.getModules().at(id);
    trgModule = DistributedModuleProperties(trgModuleToCopy.getJoint(Joint::Alpha)
            , trgModuleToCopy.getJoint(Joint::Beta)
            , trgModuleToCopy.getJoint(Joint::Gamma)
            , static_cast<ID>(id));

    trgModule.setEdges(createEdgeListFromEdges(trgConfig.getEdges(id), id));
}

std::string DistributedModule::printCurrModule() const {
    return printModule(currModule);
}

std::string DistributedModule::printTrgModule() const {
    return printModule(trgModule);
}

std::string DistributedModule::printCurrConfiguration() const {
    Printer printer;
    return printer.print(currConfiguration);
}

std::string DistributedModule::printTrgConfiguration() const {
    Printer printer;
    return printer.print(trgConfiguration);
}

EdgeList DistributedModule::createEdgeListFromEdges(const std::vector<Edge> &edges, unsigned int id) const {
    EdgeList arrayEdges;

    for (const Edge &edge : edges) {
        if (edge.id1() == id) {
            arrayEdges[edge.side1() * 3 + edge.dock1()] = edge;
        } else {
            arrayEdges[edge.side2() * 3 + edge.dock2()] = edge;
        }
    }

    return arrayEdges;
}

void DistributedModule::shareConfigurations(const DistributedModuleProperties &module, Configuration &configuration) {
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    DistributedModuleProperties moduleToSend[worldSize];
    for (int i = 0; i < worldSize; i++) {
        moduleToSend[i] = module;
    }

    int sizeOfModule = sizeof(DistributedModuleProperties);
    auto moduleToSendChar = reinterpret_cast<const char *>(moduleToSend);
    char moduleToRecvChar[sizeOfModule * worldSize];

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Alltoall(moduleToSendChar, sizeOfModule, MPI_CHAR, moduleToRecvChar, sizeOfModule, MPI_CHAR, MPI_COMM_WORLD);

    auto moduleToRecv = reinterpret_cast<DistributedModuleProperties *>(moduleToRecvChar);
    for (int i = 0; i < worldSize; i++) {
        DistributedModuleProperties &module1 = moduleToRecv[i];
        configuration.addModule(module1.getJoint(Joint::Alpha), module1.getJoint(Joint::Beta), module1.getJoint(Joint::Gamma), module1.getId());
    }

    for (int i = 0; i < worldSize; i++) {
        DistributedModuleProperties &module1 = moduleToRecv[i];
        for (auto optEdge : module1.getEdges()) {
            if (!optEdge.has_value()) {
                continue;
            }

            Edge edge = optEdge.value();
            ID id1 = edge.id1();
            ID id2 = edge.id2();

            if ((module1.getId() == id1 && id1 < id2) || (module1.getId() == id2 && id1 > id2)) {
                configuration.addEdge(edge);
            }
        }
    }
}

std::string DistributedModule::printModule(const DistributedModuleProperties &module) const {
    Printer printer;
    std::stringstream out;
    out << printer.print(module);

    for (auto &optEdge : module.getEdges()) {
        if (!optEdge.has_value()) {
            continue;
        }

        Edge &edge = optEdge.value();
        int id1 = edge.id1();
        int id2 = edge.id2();

        if ((id1 < id2 && module.getId() == id1) || (id1 > id2 && module.getId() == id2)) {
            out << printer.print(edge);
        }
    }

    return out.str();
}
