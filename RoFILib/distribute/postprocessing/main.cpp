//
// Created by xmichel on 4/5/19.
//

#include <iostream>
#include "postprocessing.h"
#include <cxxopts.hpp>

std::string dic, init;

void parse(int argc, char* argv[])
{
    cxxopts::Options options("rofi-distribute-postprocessing", "RoFI Distributed Postprocessing: Tool for processing given temporary timestamped file to a sequence of configurations. ");
    options.positional_help("[optional args]").show_positional_help();

    options.add_options()
            ("h,help", "Print help")
            ("t,dictionary", "Dictionary file", cxxopts::value<std::string>())
            ("i,init", "File with timestamped actions", cxxopts::value<std::string>())
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

        if (result.count("dictionary") == 1) {
            dic = result["dictionary"].as< std::string >();
            std::ifstream file;
            file.open(dic);
            if (!file.good()) {
                std::cerr << "Could not open file " << dic << std::endl;
                exit(0);
            }

            file.close();
        } else {
            std::cerr << "There must be exactly one '-t' or '--dictionary' option." << std::endl;
            exit(0);
        }
    } catch (cxxopts::OptionException& e) {
        std::cerr << e.what() << std::endl;
        exit(0);
    }
}

int main(int argc, char **argv) {
    parse(argc, argv);

    const std::string fileNameIn = init;
    std::map<int, int> idMap;

    std::ifstream file;
    file.open(dic);
    std::string s;
    while (getline(file, s)) {
        if (s.empty()) {
            break;
        }

        int origID, newId;
        std::stringstream tmp(s);
        tmp >> origID >> newId;
        idMap.insert({newId, origID});
    }

    file.close();

    std::stringstream output;
    Postprocessing postprocessing;
    postprocessing.generateConfigurations(fileNameIn, output);

    output.seekg(0, std::ios::beg);
    while (getline(output, s)) {
        if (s.empty()) {
            std::cout << std::endl;
            continue;
        }

        char type;
        std::stringstream tmp(s);
        tmp >> type;
        if (type == 'M') {
            double alpha, beta, gamma;
            unsigned int id;
            tmp >> id >> alpha >> beta >> gamma;
            std::cout << "M " << IO::toString(Module(alpha, beta, gamma, static_cast<ID>(idMap.at(id)))) << std::endl;
        } else if (type == 'E') {
            ID id1, id2;
            std::string s1, p1, ori, p2, s2;
            tmp >> id1 >> s1 >> p1 >> ori >> p2 >> s2 >> id2;
            std::cout << "E " << static_cast<ID>(idMap.at(id1)) << ' ' << s1 << ' ' << p1 << ' ' << ori << ' ' << p2
                      << ' ' << s2 << ' ' << static_cast<ID>(idMap.at(id2)) << std::endl;
        } else {
            throw std::runtime_error("Expected side module (M) or edge (E), got " + std::string(1, type) + ".");
        }
    }

    return 0;
}