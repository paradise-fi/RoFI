//
// Created by xmichel on 23.3.19.
//

#include <mpi.h>
#include <iostream>
#include <fstream>

#include "DistributedModule.h"

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

    std::ifstream initFile, trgFile;
    initFile.open(argv[1]);
    trgFile.open(argv[2]);

    DistributedModule module(static_cast<unsigned int>(rank), initFile, trgFile);

    initFile.close();
    trgFile.close();

    std::cout << module.printCurrModule();

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        tmpReconfiguration();
    }

    MPI_Barrier(MPI_COMM_WORLD);

    module.shareCurrConfigurations();
    module.shareTrgConfigurations();

    MPI_Finalize();
    return 0;
}
