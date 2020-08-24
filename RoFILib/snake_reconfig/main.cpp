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
    std::vector<Configuration> path = fixLeaf(init, 3);
    std::cout << IO::toString(path[path.size()-1]) << std::endl;
    /*
    AlgorithmStat stat;
    int moduleCount = init.getIDs().size();

    double path_pref = 0.5;
    int limit = 10000/moduleCount;

    if (argc > 3) {
        path_pref = std::atof(argv[2]);
        limit = std::atoi(argv[3]);
    }
*/
/*
    Testing connectArm
    Edge connection(4, A, ConnectorId::ZMinus, Orientation::North, ConnectorId::ZMinus, B, 7);
    path = connectArm(init, connection, &stat);
    std::cout << stat.toString() << std::endl;
    std::cout << IO::toString(path[path.size()-1]);
*/
/*
    Testing treefy
    SpaceGrid debug(path[path.size()-1]);
    std::cout << debug.getFreeness() << std::endl;

    std::cout << stat.toString();

    std::cout << IO::toString(path[path.size()-1]);
    path.push_back(treefy<MakeStar>(path[path.size()-1]));
    std::cout << IO::toString(path[path.size()-1]);
*/
/*  Testing makeSpace */
/*
    std::unordered_set<ID> isolate;
    if (!init.computeSpanningTree())
        std::cout << "Compute failed" << std::endl;

    addSubtree(91, isolate, init.getSpanningSucc());
    std::cout << "Isolate size: " << isolate.size() << std::endl;
    path = makeSpace(init, isolate, &stat);
    std::cout << IO::toString(path[path.size()-1]) << std::endl;
*/
}