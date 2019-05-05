//
// Created by xmichel on 4/15/19.
//

#ifndef ROFI_DISTRIBUTEDPRINTER_H
#define ROFI_DISTRIBUTEDPRINTER_H

#include "../Configuration.h"
#include "../IO.h"

class DistributedPrinter {
public:
    static std::string toString(const Configuration &configuration, int step) {
        std::stringstream out;
        std::string standardOut = IO::toString(configuration);
        if (standardOut.empty()) {
            return "";
        }

        out << step << " ";
        for (char letter : standardOut) {
            out << letter;
            if (letter == '\n') {
                out << step << " ";
            }
        }

        return out.str();
    }

    static std::string toString(const Module &module, int step) {
        std::stringstream out;
        out << step << " " << IO::toString(module);

        return out.str();
    }

    static std::string toString(const Edge &edge, int step) {
        std::stringstream out;
        out << step << " " << IO::toString(edge);

        return out.str();
    }

    static std::string toString(const Action &action, int step) {
        std::stringstream out;
        std::string standardOut = IO::toString(action);
        if (standardOut.empty()) {
            return "";
        }

        out << step << " ";
        for (char letter : standardOut) {
            out << letter;
            if (letter == '\n') {
                out << step << " ";
            }
        }

        return out.str();
    }
    static std::string toString(const Action::Rotate &rotation, int step) {
        std::stringstream out;
        out << step << " " << IO::toString(rotation);

        return out.str();
    }

    static std::string toString(const Action::Reconnect &reconnect, int step) {
        std::stringstream out;
        out << step << " " << IO::toString(reconnect);

        return out.str();
    }
};

#endif //ROFI_DISTRIBUTEDPRINTER_H
