//
// Created by xmichel on 4/15/19.
//

#ifndef ROFI_DISTRIBUTEDREADER_H
#define ROFI_DISTRIBUTEDREADER_H

#include "../IO.h"

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
};

#endif //ROFI_DISTRIBUTEDREADER_H
