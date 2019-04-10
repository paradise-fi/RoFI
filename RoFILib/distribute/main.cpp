//
// Created by xmichel on 23.3.19.
//

#include <mpi.h>
#include <iostream>
#include <fstream>

#include "../Configuration.h"
#include "../Reader.h"
#include "distributedModule.h"
#include "../Printer.h"

bool parseFile(const std::string &fileName, DistributedModule &module) {
    std::ifstream file;
    file.open(fileName);
    std::string s;

    if (file.is_open()) {
        getline(file, s);
        Reader reader;
        Configuration config;
        bool rv = reader.read(file, config);

        if (!rv) {
            return false;
        }

        auto &modules = config.getModules();

        const Module &moduleToCopy = modules.at(module.getId());
        module.rotateJoint(Alpha, moduleToCopy.getJoint(Joint::Alpha));
        module.rotateJoint(Beta, moduleToCopy.getJoint(Joint::Beta));
        module.rotateJoint(Gamma, moduleToCopy.getJoint(Joint::Gamma));

        auto edges = config.getEdges(module.getId());
        EdgeList arrayEdges;

        for (Edge &edge : edges) {
            if (edge.id1() == module.getId()) {
                arrayEdges[edge.side1() * 3 + edge.dock1()] = edge;
            } else {
                arrayEdges[edge.side2() * 3 + edge.dock2()] = edge;
            }
        }
        module.setEdges(arrayEdges);
    }

    file.close();
    return true;
}

std::string printDistributedModule(const DistributedModule &module) {
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

void tmpReconfiguration() {
    std::cout << std::endl;
    std::cout << "M 1 0 90 0" << std::endl;
    std::cout << std::endl;
    std::cout << "M 1 0 0 90" << std::endl;
    std::cout << "M 2 90 0 90" << std::endl;
    std::cout << std::endl;
    std::cout << "E + 1 0 0 0 0 1 2" << std::endl;
    std::cout << "E + 0 0 2 1 2 1 2" << std::endl;
    std::cout << std::endl;
    std::cout << "E - 1 1 2 0 2 0 2" << std::endl;
    std::cout << "E - 0 0 2 1 2 1 2" << std::endl;
    std::cout << std::endl;
    std::cout << "M 1 0 -90 0" << std::endl;
    std::cout << std::endl;
}

void shareConfigurations(const DistributedModule &module, Configuration &configuration) {
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    DistributedModule moduleToSend[worldSize];
    for (int i = 0; i < worldSize; i++) {
        moduleToSend[i] = module;
    }

    int sizeOfModule = sizeof(DistributedModule);
    auto moduleToSendChar = reinterpret_cast<const char *>(moduleToSend);
    char moduleToRecvChar[sizeOfModule * worldSize];

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Alltoall(moduleToSendChar, sizeOfModule, MPI_CHAR, moduleToRecvChar, sizeOfModule, MPI_CHAR, MPI_COMM_WORLD);

    auto moduleToRecv = reinterpret_cast<DistributedModule *>(moduleToRecvChar);
    for (int i = 0; i < worldSize; i++) {
        DistributedModule &module1 = moduleToRecv[i];
        configuration.addModule(module1.getJoint(Joint::Alpha), module1.getJoint(Joint::Beta), module1.getJoint(Joint::Gamma), module1.getId());
    }

    for (int i = 0; i < worldSize; i++) {
        DistributedModule &module1 = moduleToRecv[i];
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

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cout << "Input or output file is missing. " << std::endl;
        return 1;
    }

    int size, rank;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::string inputFileName = argv[1];
    DistributedModule currModule(0, 0, 0, static_cast<ID>(rank));
    parseFile(inputFileName, currModule);

    std::string outputFileName = argv[2];
    DistributedModule trgModule(0, 0, 0, static_cast<ID>(rank));
    parseFile(outputFileName, trgModule);

    std::cout << printDistributedModule(currModule);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        tmpReconfiguration();
    }

    MPI_Barrier(MPI_COMM_WORLD);

    Configuration currConfiguration;
    Configuration trgConfiguration;
    shareConfigurations(currModule, currConfiguration);
    shareConfigurations(trgModule, trgConfiguration);

    MPI_Finalize();
    return 0;
}
