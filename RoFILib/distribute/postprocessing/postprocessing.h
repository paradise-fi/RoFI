//
// Created by xmichel on 4/3/19.
//

#ifndef ROFI_POSTPROCESSING_H
#define ROFI_POSTPROCESSING_H

#include "../../Configuration.h"
#include "../../Printer.h"
#include "../../Reader.h"

class Postprocessing {
public:
    Postprocessing() = default;

    bool generateConfigurations(const std::string &inputFileName);

private:
    std::vector<Configuration> configurations;

    void addFirstConfiguration(std::ifstream &file);


    bool addNextConfigurations(std::ifstream &file);
};

#endif //ROFI_POSTPROCESSING_H
