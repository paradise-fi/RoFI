//
// Created by xmichel on 23.3.19.
//

#include <mpi.h>
#include <iostream>
#include <fstream>

#include "DistributedModule.h"

void tmpReconfiguration() {
    std::vector<Action> actions;
    std::vector<Action::Rotate> rotations;
    std::vector<Action::Reconnect> reconnections;

    rotations.emplace_back(1, Joint::Beta, 90);
    actions.emplace_back(rotations, reconnections);

    rotations.clear();
    reconnections.clear();
    rotations.emplace_back(1, Joint::Gamma, 90);
    rotations.emplace_back(2, Joint::Alpha, 90);
    rotations.emplace_back(2, Joint::Gamma, 90);
    actions.emplace_back(rotations, reconnections);

    rotations.clear();
    reconnections.clear();
    reconnections.emplace_back(true, Edge(static_cast<ID>(1), static_cast<Side>(0), static_cast<Dock>(0), 0,
                                          static_cast<Dock>(0), static_cast<Side>(1), static_cast<ID>(2)));
    reconnections.emplace_back(true, Edge(static_cast<ID>(0), static_cast<Side>(0), static_cast<Dock>(2), 1,
                                          static_cast<Dock>(2), static_cast<Side>(1), static_cast<ID>(2)));
    actions.emplace_back(rotations, reconnections);

    rotations.clear();
    reconnections.clear();
    reconnections.emplace_back(false, Edge(static_cast<ID>(1), static_cast<Side>(1), static_cast<Dock>(2), 0,
                                          static_cast<Dock>(2), static_cast<Side>(0), static_cast<ID>(2)));
    reconnections.emplace_back(false, Edge(static_cast<ID>(0), static_cast<Side>(0), static_cast<Dock>(2), 1,
                                          static_cast<Dock>(2), static_cast<Side>(1), static_cast<ID>(2)));
    actions.emplace_back(rotations, reconnections);

    rotations.clear();
    reconnections.clear();
    rotations.emplace_back(1, Joint::Beta, -90);
    actions.emplace_back(rotations, reconnections);

    std::cout << std::endl << IO::toString(actions);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Input or output file is missing. " << std::endl;
        return 1;
    }

    int size, rank;
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const std::string inFileName = argv[1] + std::to_string(rank) + ".in";
    const std::string trgFileName = argv[1] + std::to_string(rank) + ".out";
    std::ifstream initFile, trgFile;
    initFile.open(inFileName);
    trgFile.open(trgFileName);

    DistributedModule module(static_cast<unsigned int>(rank), initFile, trgFile);

    initFile.close();
    trgFile.close();

    std::cout << module.printCurrModule(0);

    module.reconfigurate();

    MPI_Finalize();
    return 0;
}
