//
// Created by xmichel on 5/7/19.
//

#include <iostream>
#include <map>
#include <fstream>
#include "../../IO.h"
#include "../../cxxopts.hpp"


std::string dir, init, goal;

void parse(int argc, char* argv[])
{
    cxxopts::Options options("rofi-distribute-preprocessing", "RoFI Distributed Preprocessing: Tool for preparing files for distrbuted reconfiguration. ");
    options.positional_help("[optional args]").show_positional_help();

    options.add_options()
            ("h,help", "Print help")
            ("i,init", "Initial configuration file", cxxopts::value<std::string>())
            ("g,goal", "Goal configuration file", cxxopts::value<std::string>())
            ("d,directory", "Directory for module's configs", cxxopts::value<std::string>())
            ;

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help({"", "Group"}) << std::endl;
            exit(0);
        }

        if (result.count("init") == 1) {
            init = result["init"].as< std::string >();
            std::ifstream initInput;
            initInput.open(init);
            if (!initInput.good()) {
                std::cerr << "Could not open file " << init << std::endl;
                exit(0);
            }

            initInput.close();
        } else {
            std::cerr << "There must be exactly one '-i' or '--init' option." << std::endl;
            exit(0);
        }

        if (result.count("goal") == 1) {
            goal = result["goal"].as< std::string >();
            std::ifstream goalInput;
            goalInput.open(goal);
            if (!goalInput.good()) {
                std::cerr << "Could not open file " << goal << std::endl;
                exit(0);
            }

            goalInput.close();
        } else {
            std::cerr << "There must be exactly one '-g' or '--goal' option." << std::endl;
            exit(0);
        }

        if (result.count("directory") == 1) {
            dir = result["directory"].as< std::string >();
        } else {
            std::cerr << "There must be exactly one '-d' or '--directory' option." << std::endl;
            exit(0);
        }
    } catch (cxxopts::OptionException& e) {
        std::cerr << e.what() << std::endl;
        exit(0);
    }
}

void printEdges(const Configuration &cfg, const std::string &tmpDirectory
        , const std::map<unsigned int, unsigned int> &idMap, const std::string &extension) {
    for (auto &[id, edgeList] : cfg.getEdges()) {
        unsigned int newId = idMap.at(id);
        std::ofstream file;
        file.open(tmpDirectory + std::to_string(newId) + extension, std::ofstream::app);

        for (unsigned long i = 0; i < 6; i++) {
            auto &optEdge = edgeList.at(i);

            if (optEdge.has_value()) {
                Edge edge = optEdge.value();
                Edge edge1(idMap.at(edge.id1()), edge.side1(), edge.dock1(), edge.ori(), edge.dock2(), edge.side2()
                        , idMap.at(edge.id2()));
                file << "E " << IO::toString(edge1) << std::endl;
            }
        }

        file.close();
    }
}

unsigned long preprocessInputFile(const std::string &inFile, const std::string &tmpDirectory
        , std::map<unsigned int, unsigned int> &idMap) {
    int currentId = 0;
    std::ifstream input;
    input.open(inFile);

    Configuration cfg;
    IO::readConfiguration(input, cfg);
    for (auto &[id, module] : cfg.getModules()) {
        idMap.insert({id, currentId});
        Module module1(module.getJoint(Joint::Alpha), module.getJoint(Joint::Beta), module.getJoint(Joint::Gamma),
                       static_cast<ID>(currentId));

        std::ofstream file;
        file.open(tmpDirectory + std::to_string(currentId) + ".in");
        file << "M " << IO::toString(module1) << std::endl;
        file.close();
        currentId++;
    }

    printEdges(cfg, tmpDirectory, idMap, ".in");
    input.close();

    return idMap.size();
}

unsigned long preprocessOutputFile(const std::string &outFile, const std::string &tmpDirectory
        , const std::map<unsigned int, unsigned int> &idMap) {
    std::ifstream input;
    input.open(outFile);

    Configuration cfg;
    IO::readConfiguration(input, cfg);
    for (auto &[id, module] : cfg.getModules()) {
        Module module1(module.getJoint(Joint::Alpha), module.getJoint(Joint::Beta), module.getJoint(Joint::Gamma),
                       static_cast<ID>(idMap.at(id)));

        std::ofstream file;
        file.open(tmpDirectory + std::to_string(idMap.at(id)) + ".out");
        file << "M " << IO::toString(module1) << std::endl;
        file.close();
    }

    printEdges(cfg, tmpDirectory, idMap, ".out");
    input.close();

    return cfg.getModules().size();
}

int main(int argc, char **argv) {
    parse(argc, argv);

    const std::string fileNameIn = init;
    const std::string fileNameOut = goal;
    const std::string tmpDirectory = dir;
    const std::string tmpCount = dir + "/count";
    const std::string tmpDictionary = dir + "/dictionary";

    std::map<unsigned int, unsigned int> idMap;

    unsigned long countIn = preprocessInputFile(fileNameIn, tmpDirectory, idMap);
    unsigned long countOut = preprocessOutputFile(fileNameOut, tmpDirectory, idMap);

    std::ofstream file;
    file.open(tmpCount);
    file << (countIn == countOut ? std::to_string(countIn) : "-1") << std::endl;
    file.close();

    std::ofstream dictionaryFile;
    dictionaryFile.open(tmpDictionary);
    for (auto &[oldId, newId] : idMap) {
        dictionaryFile << std::to_string(oldId) << ' ' << std::to_string(newId) << std::endl;
    }
    dictionaryFile.close();

    return (countIn == countOut) ? 0 : 1;
}
