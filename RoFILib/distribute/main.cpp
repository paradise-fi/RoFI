//
// Created by xmichel on 23.3.19.
//

#include <mpi.h>
#include <iostream>
#include <fstream>

#include "AlgorithmFullConfiguration.h"
#include "AlgorithmPartialConfiguration.h"
#include <cxxopts.hpp>

enum Algo {
    Full,
    Partial
};
std::string dir;
Algo alg = Algo::Full;
EvalFunction* eval = Eval::trivial;

void parse(int argc, char* argv[])
{
    cxxopts::Options options("distribute/rofi-distribute", "RoFI Distributed Reconfiguration: Tool for distributed computation of a reconfiguration path.");
    options.positional_help("[optional args]").show_positional_help();

    options.add_options()
            ("h,help", "Print help")
            ("d,directory", "Directory for module's configurations", cxxopts::value<std::string>())
            ("a,alg", "Algorithm for reconfiguration: full, partial", cxxopts::value<std::string>())
            ("e,eval", "Evaluation function for full A* algorithm: trivial, dMatrix, dCenter, dJoint, dAction", cxxopts::value<std::string>())
            ;

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help({"", "Group"}) << std::endl;
            exit(0);
        }

        if (result.count("directory") == 1) {
            dir = result["directory"].as< std::string >();
        } else {
            std::cerr << "There must be exactly one '-d' or '--directory' option." << std::endl;
            exit(0);
        }

        if (result.count("alg") == 1) {
            std::string val = result["alg"].as< std::string >();
            if (val == "full" || val == "FULL") {
                alg = Algo::Full;
            } else if (val == "partial" || val == "PARTIAL") {
                alg = Algo::Partial;
            } else {
                std::cerr << "Not a valid argument for option '-a', '--alg': " << val << std::endl;
                exit(0);
            }
        } else {
            if (result.count("alg") > 1) {
                std::cerr << "There can be at most one '-a' or '--alg' option." << std::endl;
                exit(0);
            }

        }

        if (result.count("eval") == 1) {
            std::string val = result["eval"].as< std::string >();
            if (val == "dCenter") {
                eval = &Eval::centerDiff;
            } else if (val == "dJoint") {
                eval = &Eval::jointDiff;
            } else if (val == "dMatrix") {
                eval = &Eval::matrixDiff;
            } else if (val == "dAction") {
                eval = &Eval::actionDiff;
            } else if (val == "trivial") {
                eval = &Eval::trivial;
            } else {
                std::cerr << "Not a valid argument for option '-e', '--eval': " << val << ".\n";
                exit(0);
            }
        } else {
            if (result.count("eval") > 1) {
                std::cerr << "There can be at most one '-e' or '--eval' option.\n";
                exit(0);
            }
        }
    } catch (cxxopts::OptionException& e) {
        std::cerr << e.what() << std::endl;
        exit(0);
    }
}

int main(int argc, char **argv) {
    parse(argc, argv);

    int size, rank;
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const std::string inFileName = dir + std::to_string(rank) + ".in";
    const std::string trgFileName = dir + std::to_string(rank) + ".out";
    std::ifstream initFile, trgFile;
    initFile.open(inFileName);
    trgFile.open(trgFileName);

    if (alg == Algo::Full) {
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
