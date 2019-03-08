#include <iostream>
#include <fstream>
#include "../Reader.h"
#include "../Configuration.h"
#include "../Printer.h"
#include "Algorithms.h"

int main(int argc, char* argv[])
{
    Configuration init, goal;
    Reader reader;
    Printer printer;

    if (argc < 3)
    {
        std::cerr << "Please, include paths to initial and goal configuration files." << std::endl;
        return 0;
    }

    std::fstream inputInit, inputGoal;

    inputInit.open(argv[1]);
    inputGoal.open(argv[2]);

    if (inputInit.good())
    {
        reader.read(inputInit, init);
    }
    else
    {
        std::cerr << "Could not open file: " << argv[1] << ".\n";
        return 0;
    }
    if (inputGoal.good())
    {
        reader.read(inputGoal, goal);
    }
    else
    {
        std::cerr << "Could not open file: " << argv[2] << ".\n";
        return 0;
    }

    bool initValid = init.isValid();
    bool goalValid = goal.isValid();

    if (!initValid)
    {
        std::cerr << "Initial configuration is not valid.\n";
    }
    if (!goalValid)
    {
        std::cerr << "Goal configuration is not valid.\n";
    }
    if (!initValid || !goalValid)
    {
        return 0;
    }

    //std::ofstream output;
    //output.open("../data/res1.out");
    auto path = AStar(init, goal, Eval::centerDiff);
    if (path.empty())
    {
        std::cout << "Could not find a path with given parameters from initial to goal configuration.\n";
        return 0;
    }
    std::cout << printer.print(path);
    //output.close();
}

