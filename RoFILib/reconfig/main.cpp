#include <fstream>
#include "../cxxopts.hpp"
#include "../Configuration.h"
#include "../Reader.h"
#include "Algorithms.h"
#include "../Printer.h"

enum class Algorithm
{
    BFS, AStar, RRT
};

std::ifstream initInput, goalInput;
unsigned step = 90;
unsigned bound = 1;
Algorithm alg = Algorithm::BFS;
EvalFunction* eval = Eval::trivial;

void parse(int argc, char* argv[])
{
    cxxopts::Options options("rofi-reconfig", "RoFI Reconfiguration: Tool for computing a reconfiguration path.");
    options.positional_help("[optional args]").show_positional_help();

    options.add_options()
            ("h,help", "Print help")
            ("i,init", "Initial configuration file", cxxopts::value<std::string>())
            ("g,goal", "Goal configuration file", cxxopts::value<std::string>())
            ("s,step", "Rotation angle step size in range <0,90>", cxxopts::value<unsigned>())
            ("a,alg", "Algorithm for reconfiguration: bfs, astar, rrt", cxxopts::value<std::string>())
            ("e,eval", "Evaluation function for A* algorithm: dMatrix, dCenter, dJoint, dAction, trivial", cxxopts::value<std::string>())
            ("p,parallel", "How many parallel actions are allowed: <1,...>", cxxopts::value<unsigned>())
            ;

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help({"", "Group"}) << std::endl;
            exit(0);
        }

        if (result.count("init") == 1) {
            std::string initPath(result["init"].as< std::string >());
            initInput.open(initPath);
            if (!initInput.good()) {
                std::cerr << "Coult not open file " << initPath << ".\n";
                exit(0);
            }
        } else {
            std::cerr << "There must be exactly one '-i' or '--init' option.\n";
            exit(0);
        }

        if (result.count("goal") == 1) {
            std::string goalPath(result["goal"].as< std::string >());
            goalInput.open(goalPath);
            if (!goalInput.good()) {
                std::cerr << "Coult not open file " << goalPath << ".\n";
                exit(0);
            }
        } else {
            std::cerr << "There must be exactly one '-g' or '--goal' option.\n";
            exit(0);
        }

        if (result.count("step") == 1) {
            unsigned val = result["step"].as< unsigned >();
            if (val > 90) {
                std::cerr << "The step size must be in range <0,90>.\n";
                exit(0);
            }
            step = val;
        } else {
            if (result.count("step") > 1) {
                std::cerr << "There can be at most one '-s' or '--step' option.\n";
                exit(0);
            }
        }

        if (result.count("alg") == 1) {
            std::string val = result["alg"].as< std::string >();
            bool valid = false;
            if ((val == "bfs") || (val == "BFS")) {
                alg = Algorithm::BFS;
                valid = true;
            }
            if ((val == "astar") || (val == "AStar")) {
                alg = Algorithm::AStar;
                valid = true;
            }
            if ((val == "rrt") || (val == "RRT")) {
                alg = Algorithm::RRT;
                valid = true;
            }
            if (!valid) {
                std::cerr << "Not a valid argument for option '-a', '--alg': " << val << ".\n";
                exit(0);
            }
        } else {
            if (result.count("alg") > 1) {
                std::cerr << "There can be at most one '-a' or '--alg' option.\n";
                exit(0);
            }

        }

        if ((result.count("eval") == 1)) {
            if (alg != Algorithm::AStar)
            {
                std::cerr << "The option '--eval' is available only with the option '--alg astar'.\n";
                exit(0);
            }
            std::string val = result["eval"].as< std::string >();
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
        } else {
            if (result.count("eval") > 1) {
                std::cerr << "There can be at most one '-e' or '--eval' option.\n";
                exit(0);
            }
        }

        if (result.count("parallel") == 1)
        {
            unsigned val = result["parallel"].as<unsigned>();
            if (val == 0)
            {
                std::cerr << "At least one parallel action must be allowed.\n";
                exit(0);
            }
            bound = val;
        } else {
            if (result.count("parallel") > 1) {
                std::cerr << "There can be at most one '-p' or '--parallel' option.\n";
                exit(0);
            }
        }
    }
    catch (cxxopts::OptionException& e)
    {
        std::cerr << e.what() << "\n";
        exit(0);
    }
}

int main(int argc, char* argv[])
{
    parse(argc, argv);

    Configuration init, goal;
    Reader reader;
    Printer printer;

    bool ok = true;
    ok &= reader.read(initInput, init);
    ok &= reader.read(goalInput, goal);

    if (!ok)
    {
        std::cerr << "Could not parse given files or files are empty.\n";
        exit(0);
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
        exit(0);
    }


    std::vector<Configuration> path;
    switch (alg)
    {
        case Algorithm::BFS:
            path = BFS(init, goal, step, bound);
            break;
        case Algorithm::AStar:
            path = AStar(init, goal, step, bound, *eval);
            break;
        case Algorithm::RRT:
            path = RRT(init, goal, step);
            break;
    }

    std::cout << printer.print(path);

    if (path.empty())
    {
        std::cout << "Could not find a path with given parameters from initial to goal configuration.\n";
        return 0;
    }
}