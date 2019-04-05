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

    while (getline(file, s)) {
        if (s.empty()) {
            configurations.push_back(currConfiguration);
            continue;
        }

        char type;
        std::stringstream tmp(s);
        tmp >> type;

        std::vector<Action::Rotate> rotations;
        std::vector<Action::Reconnect> reconnections;

        if (type == 'M') {
            double alpha, beta, gamma;
            unsigned int id;
            tmp >> id >> alpha >> beta >> gamma;

            rotations.emplace_back(static_cast<ID>(id), Joint::Alpha, alpha);
            rotations.emplace_back(static_cast<ID>(id), Joint::Beta, beta);
            rotations.emplace_back(static_cast<ID>(id), Joint::Gamma, gamma);
        } else if (type == 'E') {
            char addConnection;
            unsigned int id1, s1, d1, ori, d2, s2, id2;
            tmp >> addConnection >> id1 >> s1 >> d1 >> ori >> d2 >> s2 >> id2;

            Edge edge(static_cast<ID>(id1), static_cast<Side>(s1), static_cast<Dock>(d1), ori, static_cast<Dock>(d2),
                      static_cast<Side>(s2), static_cast<ID>(id2));
            reconnections.emplace_back(addConnection == '+', edge);
        }

        currConfiguration.execute({rotations, reconnections});
    }

    return true;
}
