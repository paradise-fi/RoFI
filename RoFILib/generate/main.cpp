//
// Created by xvozarov on 5/2/19.
//

#include <cassert>
#include <Configuration.h>
#include <IO.h>
#include <chrono>

using namespace IO;

using ConfigPair = std::pair<Configuration, Configuration>;
std::string folder = "../data/trees/";

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

Configuration sampleTree(std::vector<ID>& ids)
{
    std::default_random_engine e;
    e.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> ab(-1,1);
    std::uniform_int_distribution<int> c(-1, 2);
    Configuration cfg;

    cfg.addModule(90 * ab(e), 90 * ab(e), 90 * c(e), ids[0]);
    std::vector<std::tuple<ID, Side, Dock>> unoccupied;
    for (Side s : {A, B})
    {
        for (Dock d : {Xn, Xp, Zn})
        {
            unoccupied.emplace_back(ids[0], s, d);
        }
    }
    for (unsigned i = 1; i < ids.size(); ++i)
    {
        std::uniform_int_distribution<unsigned long> dock(0, unoccupied.size()-1);
        auto id1 = ids[i];
        cfg.addModule(90 * ab(e), 90 * ab(e), 90 * c(e), id1);
        while (true)
        {
            ID id2;
            Side s2;
            Dock d2;
            std::tie(id2, s2, d2) = unoccupied[dock(e)];
            Side s1 = Side((c(e) + 1) % 2);
            Dock d1 = Dock(ab(e) + 1);
            auto ori = static_cast<unsigned int>(c(e) + 1);
            Configuration cfg2 = cfg;
            cfg2.addEdge({id1, s1, d1, ori, d2, s2, id2});
            if (cfg2.isValid())
            {
                cfg.addEdge({id1, s1, d1, ori, d2, s2, id2});
                for (Side s : {A, B})
                {
                    for (Dock d : {Xn, Xp, Zn})
                    {
                        unoccupied.emplace_back(id1, s, d);
                    }
                }
                unoccupied.erase(std::remove(unoccupied.begin(), unoccupied.end(),
                        std::make_tuple(id2, s2, d2)), unoccupied.end());
                unoccupied.erase(std::remove(unoccupied.begin(), unoccupied.end(),
                        std::make_tuple(id1, s1, d1)), unoccupied.end());
                break;
            }
        }
    }
    return cfg;
}

ConfigPair generateTest(unsigned modules, unsigned path, unsigned step, unsigned bound)
{
    std::default_random_engine e;
    e.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::vector<ID> ids(modules * 5);
    std::iota(ids.begin(), ids.end(), 0);
    std::shuffle(ids.begin(), ids.end(), e);
    ids.resize(modules);

    Configuration init = sampleTree(ids);
    Configuration goal = init;
    for (unsigned i = 0; i < path; )
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
            {
                goal = value.value();
                ++i;
            }

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
            {
                goal = value.value();
                ++i;
            }

        }
    }
    return {init, goal};
}

int main()
{
    unsigned moduleCount[] = {50}; //,25,30,40,50};
    unsigned pathSize[] = {5};

    for (unsigned i : moduleCount)
    {
        for (unsigned path : pathSize)
        {
            auto test = generateTest(i, path, 90, 1);
            printToFile(test, i, path, 90, 1);
        }
    }
}