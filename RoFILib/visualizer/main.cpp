#include <iostream>
#include <fstream>
#include <string_view>
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

    if (argc == 3)
    {
        if (argv[2] == std::string_view("-m"))
        {
            std::vector<Configuration> configs;
            std::ifstream input;
            input.open(argv[1]);
            if (input.good())
            {
                reader.read(input, configs);
                for (Configuration& config : configs)
                {
                    config.computeRotations();
                    vis.drawConfiguration(config);
                }
            }
            else
            {
                std::cerr << "Could not open file: " << argv[1] << ".\n";
            }
            return 0;
        }
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

