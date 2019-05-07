//
// Created by xmichel on 5/7/19.
//

#include <iostream>
#include <map>
#include <fstream>
#include "../../IO.h"

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
        file << IO::toString(module1) << std::endl;
        file.close();
    }

    printEdges(cfg, tmpDirectory, idMap, ".out");
    input.close();

    return cfg.getModules().size();
}

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cout << "Input files or directory is missing. " << std::endl;
        return 1;
    }

    const std::string fileNameIn = argv[1];
    const std::string fileNameOut = argv[2];
    const std::string tmpDirectory = argv[3];
    const std::string tmpDictionary = tmpDirectory + "dictionary";
    const std::string tmpCount = tmpDirectory + "count";

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
