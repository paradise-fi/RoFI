#include <iostream>
#include <fstream>
#include "../Reader.h"
#include "Visualizer.h"

int main(int argc, char *argv[])
{
    Reader reader;
    Visualizer vis;

    if (argc < 2)
    {
        std::cerr << "Please, include one or more paths to a configuration file." << std::endl;
        return 0;
    }

    for (int i = 1; i < argc; ++i)
    {
        Configuration config;
        std::ifstream input;
        input.open(argv[i]);
        if (input.good())
        {
            reader.read(input, config);
            config.computeRotations();
            vis.drawConfiguration(config);
        }
        else
        {
            std::cerr << "Could not open file: " << argv[i] << ".\n";
        }
    }
}

