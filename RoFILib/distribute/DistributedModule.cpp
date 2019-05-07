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
    std::vector<Action> generatedActions = createDiffs(generatePath);
    optimizePath(generatedActions);

    for (unsigned int i = 0; i < generatedActions.size(); i++) {
        Action diff = generatedActions.at(i);
        executeDiff(diff, i + 1);
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
    for (const auto &rotation : action.rotations()) {
        if (currModule.getId() == rotation.id()) {
            currModule.execute(rotation);
            std::cout << DistributedPrinter::toString(rotation, step);
        }
    }

    for (const auto &reconnection : action.reconnections()) {
        if (currModule.getId() == reconnection.edge().id1()) {
            currModule.execute(reconnection);
            std::cout << DistributedPrinter::toString(reconnection, step);
        } else if (currModule.getId() == reconnection.edge().id2()) {
            currModule.execute(reconnection);
        }
    }
}

std::vector<Action> DistributedModule::createDiffs(const std::vector<Configuration> &path) const {
    std::vector<Action> rv;
    for (unsigned int i = 1; i < path.size(); i++) {
        Action diff = path.at(i - 1).diff(path.at(i));
        rv.push_back(diff);
    }

    return rv;
}

void DistributedModule::optimizePath(std::vector<Action> &path) const {
    joinActions(path);
    bool doMoreOpt = true;

    while (doMoreOpt) {
        swapActions(path);
        doMoreOpt = joinActions(path);
    }
}

bool DistributedModule::joinActions(std::vector<Action> &path) const {
    if (path.empty()) {
        return false;
    }

    bool rv = false;
    Configuration curr = currConfiguration;
    std::vector<Action> newActions;
    newActions.push_back(path.at(0));
    unsigned long currIndex = 0;

    for (unsigned long i = 1; i < path.size(); i++) {
        Configuration correct = curr;
        correct.execute(newActions.at(currIndex));
        correct.execute(path.at(i));

        std::vector<Action::Rotate> rotate = newActions.at(currIndex).rotations();
        std::vector<Action::Reconnect> reconnect = newActions.at(currIndex).reconnections();
        std::vector<Action::Rotate> rotate2 = path.at(i).rotations();
        std::vector<Action::Reconnect> reconnect2 = path.at(i).reconnections();
        rotate.insert(rotate.end(), rotate2.begin(), rotate2.end());
        reconnect.insert(reconnect.end(), reconnect2.begin(), reconnect2.end());

        auto optionalCfg = curr.executeIfValid({rotate, reconnect});

        if (!optionalCfg.has_value() || optionalCfg.value() != correct) {
            curr.execute(newActions.at(currIndex));
            currIndex++;
            continue;
        }

        rv = true;
        newActions[currIndex] = {rotate, reconnect};
    }

    path.clear();
    path = newActions;
    return rv;
}

void DistributedModule::swapActions(std::vector<Action> &path) const {
    Configuration beforeFst = currConfiguration;

    for (unsigned long i = 1; i < path.size(); i++) {
        Configuration correctOrder = beforeFst;

        correctOrder.execute(path.at(i - 1));

        std::optional<Configuration> swapOrder = beforeFst.executeIfValid(path.at(i));
        if (!swapOrder.has_value()) {
            beforeFst = correctOrder;
            continue;
        }

        swapOrder = swapOrder.value().executeIfValid(path.at(i - 1));
        if (!swapOrder.has_value()) {
            beforeFst = correctOrder;
            continue;
        }

        correctOrder.execute(path.at(i));
        if (correctOrder == swapOrder.value()) {
            std::swap(path[i], path[i - 1]);
        }

        beforeFst.execute(path.at(i - 1));
    }

}
