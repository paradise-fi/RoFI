//
// Created by xmichel on 4/3/19.
//

#ifndef ROFI_POSTPROCESSING_H
#define ROFI_POSTPROCESSING_H

#include "../../Configuration.h"
#include "../../IO.h"
#include "../DistributedReader.h"

class Postprocessing {
public:
    Postprocessing() = default;

    bool generateConfigurations(const std::string &inputFileName);

private:
    std::vector<Configuration> configurations;

    void addFirstConfiguration(std::stringstream &inputStream);

    void addNextConfigurations(std::stringstream &inputStream);
};

#endif //ROFI_POSTPROCESSING_H
