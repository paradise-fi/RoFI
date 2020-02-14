#include <exception>
#include <fstream>
#include <tuple>
#include <stdlib.h>
#include "../configuration/Configuration.h"
#include "../configuration/IO.h"
#include "Snake_algorithms.h"

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
    /*
    {
        std::cout << "Loaded: \n" << IO::toString(init);
        auto sg = SpaceGrid(init);
        std::cout << "Freeness: " << std::endl << sg.getFreeness() << std::endl;
        sg.printGrid();
    }
    */
    std::vector<Configuration> path;
    AlgorithmStat stat;
    int moduleCount = init.getIDs().size();
    
    double path_pref = 0.5;
    int limit = 10000/moduleCount;

    if (argc > 3) {
        path_pref = std::atof(argv[2]);
        limit = std::atoi(argv[3]);
    }
    path = SnakeStar(init, &stat, limit, path_pref);

    SpaceGrid debug(path[path.size()-1]);
    std::cout << debug.getFreeness() << std::endl;

    std::cout << stat.toString();

    std::cout << IO::toString(path[path.size()-1]);
    path.push_back(treefy<MakeStar>(path[path.size()-1]));
    std::cout << IO::toString(path[path.size()-1]);
}