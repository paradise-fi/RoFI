//
// Created by xmichel on 4/15/19.
//

#ifndef ROFI_DISTRIBUTEDPRINTER_H
#define ROFI_DISTRIBUTEDPRINTER_H

#include "../Configuration.h"
#include "../Printer.h"

class DistributedPrinter {
public:
    static std::string toString(const Configuration &configuration, int step) {
        std::stringstream out;
        Printer printer;
        std::string standardOut = printer.print(configuration);
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
        Printer printer;
        out << step << " " << printer.print(module);

        return out.str();
    }

    static std::string toString(const Edge &edge, int step) {
        std::stringstream out;
        Printer printer;
        out << step << " " << printer.print(edge);

        return out.str();
    }

    static std::string toString(const Action &action, int step) {
        std::stringstream out;
        std::string standardOut = Printer::toString(action);
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
        out << step << " " << Printer::toString(rotation);

        return out.str();
    }

    static std::string toString(const Action::Reconnect &reconnect, int step) {
        std::stringstream out;
        out << step << " " << Printer::toString(reconnect);

        return out.str();
    }
};

#endif //ROFI_DISTRIBUTEDPRINTER_H
