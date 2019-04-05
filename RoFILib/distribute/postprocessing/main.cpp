//
// Created by xmichel on 4/5/19.
//

#include <iostream>
#include "postprocessing.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Input file is missing. " << std::endl;
        return 1;
    }

    const std::string fileNameIn = argv[1];
    Postprocessing postprocessing;
    postprocessing.generateConfigurations(fileNameIn);

    return 0;
}