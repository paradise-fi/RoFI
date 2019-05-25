//
// Created by xmichel on 23.3.19.
//

#include <mpi.h>
#include <iostream>
#include <fstream>

#include "AlgorithmFullConfiguration.h"
#include "AlgorithmPartialConfiguration.h"

int main(int argc, char **argv) {
    if (argc != 4) {
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

    //std::cout << argv[2] << "- -" << argv[2][0] << "-" << std::endl;
    if (argv[2][0] == '1') {
        EvalFunction* eval = Eval::trivial;
        std::string val = argv[3];
        bool valid = false;
        if (val == "dCenter") {
            eval = &Eval::centerDiff;
            valid = true;
        }
        if (val == "dJoint") {
            eval = &Eval::jointDiff;
            valid = true;
        }
        if (val == "dMatrix") {
            eval = &Eval::matrixDiff;
            valid = true;
        }
        if (val == "dAction") {
            eval = &Eval::actionDiff;
            valid = true;
        }
        if (val == "trivial") {
            eval = &Eval::trivial;
            valid = true;
        }
        if (!valid) {
            std::cerr << "Not a valid argument for option '-e', '--eval': " << val << ".\n";
            exit(0);
        }

        AlgorithmFullConfiguration module(static_cast<unsigned int>(rank), initFile, trgFile);
        std::cout << module.printCurrModule(0);
        module.reconfigurate(eval);
    } else {
        AlgorithmPartialConfiguration module(static_cast<unsigned int>(rank), initFile, trgFile);
        std::cout << module.printCurrModule(0);
        bool rv = module.reconfigurate();
        if (rank == 0 && !rv) {
            std::cout << "don't end correctly" << std::endl;
        }
    }

    initFile.close();
    trgFile.close();

    MPI_Finalize();
    return 0;
}
