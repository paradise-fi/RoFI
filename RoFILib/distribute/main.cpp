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
        module.setEdges(edges);
    }

    file.close();
    return true;
}

void printModule(const DistributedModule &module) {
    Printer printer;
    std::cout << printer.print(module);

    for (const Edge &edge : module.getEdges()) {
        int id1 = edge.id1;
        int id2 = edge.id2;

        if ((id1 < id2 && module.getId() == id1) || (id1 > id2 && module.getId() == id2)) {
            std::cout << printer.print(edge);
        }
    }
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

    printModule(currModule);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        tmpReconfiguration();
    }

    MPI_Finalize();
    return 0;
}
