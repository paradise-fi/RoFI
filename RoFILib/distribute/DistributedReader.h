//
// Created by xmichel on 4/15/19.
//

#ifndef ROFI_DISTRIBUTEDREADER_H
#define ROFI_DISTRIBUTEDREADER_H

#include "../IO.h"
#include "DistributedModuleProperties.h"

class DistributedReader {
public:
    static bool fromString(std::istream& input, Configuration& config, int step) {
        std::stringstream stringstream;
        std::string s;

        while (getline(input, s)) {
            if (s.empty()) {
                break;
            }

            int currStep;
            std::stringstream tmp(s);
            tmp >> currStep;
            if (currStep == step) {
                s.erase(0, s.find(' ') + 1);
                stringstream << s << std::endl;
            }
        }

        IO::readConfiguration(stringstream, config);

        return !config.empty();
    }

    static Action fromStringAction(std::istream &input, int step) {
        std::stringstream stringstream;
        std::string s;

        while (getline(input, s)) {
            if (s.empty()) {
                break;
            }

            int currStep;
            std::stringstream tmp(s);
            tmp >> currStep;
            if (currStep == step) {
                s.erase(0, s.find(' ') + 1);
                stringstream << s << std::endl;
            }
        }

        return IO::readAction(stringstream);
    }

    static void readDistributedModule(std::istream &input, DistributedModuleProperties &module) {
        std::string s;

        while (getline(input, s)) {
            if (s.empty()) {
                break;
            }
            char type;
            std::stringstream tmp(s);
            tmp >> type;
            if (type == 'M') {
                double alpha, beta, gamma;
                unsigned int id;
                tmp >> id >> alpha >> beta >> gamma;

                if (id == module.getId()) {
                    module.rotateJoint(Joint::Alpha, alpha);
                    module.rotateJoint(Joint::Beta, beta);
                    module.rotateJoint(Joint::Gamma, gamma);
                }
            } else if (type == 'E') {
                module.addEdge(IO::readEdge(tmp));
            } else {
                throw std::runtime_error("Expected side module (M) or edge (E), got " + std::string(1, type) + ".");
            }
        }
    }
};

#endif //ROFI_DISTRIBUTEDREADER_H
