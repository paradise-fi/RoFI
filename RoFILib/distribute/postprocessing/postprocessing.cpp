//
// Created by xmichel on 4/3/19.
//

#include "postprocessing.h"

bool Postprocessing::generateConfigurations(const std::string &inputFileName) {
    std::stringstream stringstream;
    std::ifstream file;
    file.open(inputFileName);

    std::string s;
    while (getline(file, s)) {
        stringstream << s << std::endl;
    }
    file.close();

    addFirstConfiguration(stringstream);
    if (configurations.empty()) {
        return false;
    }

    addNextConfigurations(stringstream);
    std::cout << IO::toString(configurations);

    return true;
}

void Postprocessing::addFirstConfiguration(std::stringstream &inputStream) {
    std::vector<std::string> sortedConfig;
    std::string s;

    while (getline(inputStream, s)) {
        if (s.empty()) {
            break;
        }

        int step;
        char type;
        std::stringstream tmp(s);
        tmp >> step >> type;

        if (step != 0) {
            continue;
        }

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

    Configuration cfg;
    DistributedReader::fromString(stream, cfg, 0);
    configurations.push_back(cfg);
}

void Postprocessing::addNextConfigurations(std::stringstream &inputStream) {
    std::string s;
    Configuration currConfiguration = configurations.at(0);
    bool emptyAction = false;
    int step = 1;

    while (!emptyAction) {
        inputStream.clear();
        inputStream.seekg(0, std::ios::beg);
        Action action = DistributedReader::fromStringAction(inputStream, step);
        emptyAction = action.rotations.empty() && action.reconnections.empty();

        if (!emptyAction) {
            currConfiguration.execute(action);
            configurations.push_back(currConfiguration);
            step++;
        }
    }
}
