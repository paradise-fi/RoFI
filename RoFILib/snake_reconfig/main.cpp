#include <exception>
#include <fstream>
#include <tuple>
#include <stdlib.h>
#include "../configuration/Configuration.h"
#include "../configuration/IO.h"
#include "Snake_algorithms.h"

void printVecToFile(const std::vector<Configuration>& configs, const char* path) {
    std::ofstream file;
    file.open(std::string(path));
    for (const auto& conf : configs) {
        file << IO::toString(conf) << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        throw std::invalid_argument("No filepath given");
    }

    std::ifstream initInput;
    initInput.open(argv[1]);
    Configuration init;
    IO::readConfiguration(initInput, init);
    init.computeMatrices();

    std::ofstream log;
    log.open(std::string(argv[2]), std::ios_base::app);
    log << argv[1] << ";";
    auto res = reconfigToSnake(init, &log).first;
    printVecToFile(res, argv[3]);

    return 0;
}