#include "worldCreator.hpp"


int main()
{
    auto configs = ConfigWithPose::readConfigurations( std::cin );

    for ( auto & config : configs )
    {
        if ( config.config.empty() )
        {
            std::cerr << "Loaded empty config\n";
            continue;
        }
        if ( !config.config.isValid() )
        {
            throw std::runtime_error(
                    "Loaded config is not valid (each config has to be connected)" );
        }
    }

    createWorld( configs )->PrintValues();
}
