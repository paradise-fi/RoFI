//
// Created by xmichel on 23.3.19.
//

#include <mpi.h>
#include <iostream>
#include <fstream>

#include "../Configuration.h"
#include "../Reader.h"

bool parseInputFile(const std::string &fileName, Module &module) {
    std::ifstream file;
    file.open(fileName);
    std::string s;

    if (file.is_open()) {
        getline(file, s);
        Reader reader;
        Configuration config;
        reader.read(file, config);
        auto &modules = config.getModules();
        module = modules.at(module.getId());
    }

    file.close();
    return true;
}

void printModule(const Module &module) {
    std::cout << "M " << module.getId() << " " << module.getJoint(Joint::Alpha) << " "
    << module.getJoint(Joint::Beta) << " "
    << module.getJoint(Joint::Gamma) << " "
    //<< module.getEdges().size()
    << std::endl;

    /*for (const Edge &edge : module.getEdges()) {
        int id1 = edge.id1;
        int id2 = edge.id2;

        if ((id1 < id2 && module.getId() == id1) || (id1 > id2 && module.getId() == id2)) {
            std::cout << "E " << id1 << " " << static_cast<int>(edge.side1) << " " << static_cast<int>(edge.dock1) << " "
                      << edge.ori << " " << static_cast<int>(edge.dock2) << " " << static_cast<int>(edge.side2) << " " << id2
                      << std::endl;
        }
    }*/
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Input file is missing. " << std::endl;
        return 1;
    }

    int size, rank;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    std::string fileName = argv[1];
    Module module(0, 0, 0, rank);
    parseInputFile(fileName, module);
    printModule(module);

    MPI_Finalize();
    return 0;
}