//
// Created by xmichel on 4/12/19.
//

#include "DistributedModule.h"

DistributedModule::DistributedModule(unsigned int id, std::ifstream &initConfigStream,
                                     std::ifstream &trgConfigStream) {
    if (!initConfigStream.is_open() || !trgConfigStream.is_open()) {
        return;
    }

    Configuration initConfig;
    IO::readConfiguration(initConfigStream, initConfig);

    const Module &initModuleToCopy = initConfig.getModules().at(id);
    currModule = DistributedModuleProperties(initModuleToCopy.getJoint(Joint::Alpha)
            , initModuleToCopy.getJoint(Joint::Beta)
            , initModuleToCopy.getJoint(Joint::Gamma)
            , static_cast<ID>(id));

    currModule.setEdges(createEdgeListFromEdges(initConfig.getEdges(id), id));

    Configuration trgConfig;
    IO::readConfiguration(trgConfigStream, trgConfig);

    const Module &trgModuleToCopy = trgConfig.getModules().at(id);
    trgModule = DistributedModuleProperties(trgModuleToCopy.getJoint(Joint::Alpha)
            , trgModuleToCopy.getJoint(Joint::Beta)
            , trgModuleToCopy.getJoint(Joint::Gamma)
            , static_cast<ID>(id));

    trgModule.setEdges(createEdgeListFromEdges(trgConfig.getEdges(id), id));
}

std::string DistributedModule::printCurrModule(int step) const {
    return printModule(currModule, step);
}

std::string DistributedModule::printTrgModule(int step) const {
    return printModule(trgModule, step);
}

std::string DistributedModule::printCurrConfiguration(int step) const {
    return DistributedPrinter::toString(currConfiguration, step);
}

std::string DistributedModule::printTrgConfiguration(int step) const {
    return DistributedPrinter::toString(trgConfiguration, step);
}

void DistributedModule::reconfigurate() {
    shareCurrConfigurations();
    shareTrgConfigurations();

    auto generatePath = AStar(currConfiguration, trgConfiguration, 90, 1, Eval::actionDiff);

    for (unsigned int i = 1; i < generatePath.size(); i++) {
        Action diff = generatePath.at(i - 1).diff(generatePath.at(i));
        executeDiff(diff, i);
        MPI_Barrier(MPI_COMM_WORLD);
    }
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

void DistributedModule::executeDiff(const Action &action, int step) {
    for (const auto &rotation : action.rotations) {
        if (currModule.getId() == rotation.id()) {
            currModule.execute(rotation);
            std::cout << DistributedPrinter::toString(rotation, step);
        }
    }

    for (const auto &reconnection : action.reconnections) {
        if (currModule.getId() == reconnection.edge().id1()) {
            currModule.execute(reconnection);
            std::cout << DistributedPrinter::toString(reconnection, step);
        } else if (currModule.getId() == reconnection.edge().id2()) {
            currModule.execute(reconnection);
        }
    }
}
