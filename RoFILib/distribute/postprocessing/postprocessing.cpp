//
// Created by xmichel on 4/3/19.
//

#include "postprocessing.h"

bool Postprocessing::generateConfigurations(const std::string &inputFileName) {
    std::ifstream file;
    file.open(inputFileName);

    addFirstConfiguration(file);

    if (configurations.empty()) {
        return false;
    }

    bool isOk = addNextConfigurations(file);

    if (!isOk) {
        return false;
    }

    Printer printer;
    std::cout << printer.print(configurations);

    file.close();
    return true;
}

void Postprocessing::addFirstConfiguration(std::ifstream &file) {
    std::vector<std::string> sortedConfig;
    std::string s;

    while (file.is_open()) {
        getline(file, s);

        if (s.empty()) {
            break;
        }

        char type;
        std::stringstream tmp(s);
        tmp >> type;

        if (type == 'M') {
            sortedConfig.insert(sortedConfig.begin(), s);
        } else if (type == 'E') {
            sortedConfig.push_back(s);
        }
    }

    std::stringstream stream;
    for (const std::string &oneLine : sortedConfig) {
        stream << oneLine << std::endl;
    }

    Reader reader;
    reader.read(stream, configurations);
}

bool Postprocessing::addNextConfigurations(std::ifstream &file) {
    std::string s;
    Configuration currConfiguration = configurations.at(0);
    bool emptyAction = false;

    while (!emptyAction) {
        Action action = Reader::fromStringAction(file);
        emptyAction = action.rotations.empty() && action.reconnections.empty();

        if (!emptyAction) {
            currConfiguration.execute(action);
            configurations.push_back(currConfiguration);
        }
    }

    return true;
}
