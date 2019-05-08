//
// Created by xmichel on 4/5/19.
//

#include <iostream>
#include "postprocessing.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cout << "Input files are missing. " << std::endl;
        return 1;
    }

    const std::string fileNameIn = argv[1];
    std::map<int, int> idMap;

    std::ifstream file;
    file.open(argv[2]);
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