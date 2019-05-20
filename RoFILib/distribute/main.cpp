//
// Created by xmichel on 23.3.19.
//

#include <mpi.h>
#include <iostream>
#include <fstream>

#include "AlgorithmFullConfiguration.h"
#include "AlgorithmPartialConfiguration.h"

int main(int argc, char **argv) {
    if (argc != 3) {
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

    if (argv[2] == "1") {
        AlgorithmFullConfiguration module(static_cast<unsigned int>(rank), initFile, trgFile);
        std::cout << module.printCurrModule(0);
        module.reconfigurate();
    } else {
        AlgorithmPartialConfiguration module(static_cast<unsigned int>(rank), initFile, trgFile);
        std::cout << module.printCurrModule(0);
        module.reconfigurate();
    }

    initFile.close();
    trgFile.close();

    MPI_Finalize();
    return 0;
}
