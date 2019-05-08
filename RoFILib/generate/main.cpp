//
// Created by xvozarov on 5/2/19.
//

#include <cassert>
#include "../Configuration.h"
#include "../IO.h"
#include "../reconfig/Algorithms.h"

using namespace IO;

using ConfigPair = std::pair<Configuration, Configuration>;
std::string folder = "../data/planner-tests/";

void printToFile(const Configuration& cfg, const std::string& path)
{
    std::ofstream file;
    file.open(path);
    file << toString(cfg);
    file.close();
}

void printToFile(const ConfigPair& test, unsigned modules, unsigned path, unsigned step, unsigned bound)
{
    std::stringstream init;
    //init << folder << modules << "-" << path << "-" << step << "-" << bound << "-init.in";
    init << folder << modules << "-" << path << "-init.in";
    printToFile(test.first, init.str());

    std::stringstream goal;
    //goal << folder << modules << "-" << path << "-" << step << "-" << bound << "-goal.in";
    goal << folder << modules << "-" << path  << "-goal.in";
    printToFile(test.second, goal.str());
}

ConfigPair generateTest(unsigned modules, unsigned path, unsigned step, unsigned bound)
{
    std::default_random_engine e{static_cast<long unsigned int>(time(0))};
    std::vector<ID> ids(modules * 5);
    std::iota(ids.begin(), ids.end(), 0);
    std::shuffle(ids.begin(), ids.end(), e);
    ids.resize(modules);

    Configuration init = sampleFree(ids);
    Configuration goal = init;
    for (unsigned i = 0; i < path; ++i)
    {
        std::vector<Action::Rotate> rotate;
        std::vector<Action::Reconnect> reconnect;
        goal.generateReconnect(reconnect);
        goal.generateRotations(rotate, step);
        std::uniform_int_distribution<unsigned long> flip(0, 5);
        //if ((flip(e) > 1))
        {
            std::uniform_int_distribution<unsigned long> dist(0, reconnect.size() - 1);
            auto value = goal.executeIfValid({{},{reconnect[dist(e)]}});
            unsigned count = 0;
            while (!value.has_value() && count < rotate.size())
            {
                value = goal.executeIfValid({{},{reconnect[dist(e)]}});
                ++count;
            }
            if (count < rotate.size())
                goal = value.value();
        }
        //else
        {
            std::uniform_int_distribution<unsigned long> dist(0, rotate.size() - 1);
            auto value = goal.executeIfValid({{rotate[dist(e)]}, {}});
            unsigned count = 0;
            while (!value.has_value() && count < rotate.size())
            {
                value = goal.executeIfValid({{rotate[dist(e)]}, {}});
                ++count;
            }
            if (count < rotate.size())
                goal = value.value();
        }
    }
    return {init, goal};
}

int main()
{
    unsigned moduleCount[] = {2,3,5,10,15}; //,25,30,40,50};
    unsigned pathSize[] = {5,10,20,30,50};

    for (unsigned i : moduleCount)
    {
        for (unsigned path : pathSize)
        {
            auto test = generateTest(i, path, 90, 1);
            printToFile(test, i, path, 90, 1);
        }
    }
}