#include "commands.hpp"

#include <fstream>
#include <stdexcept>

#include <configuration/rofiworld.hpp>
#include <configuration/universalModule.hpp>
#include <configuration/serialization.hpp>

rofi::configuration::RofiWorld parseConfiguration( const std::string& inputFile )
{
    auto cfgFile = std::ifstream( inputFile );
    if ( !cfgFile.is_open() )
        throw std::runtime_error( "Cannot open file '" + inputFile + "'" );

    rofi::configuration::RofiWorld configuration;
    if ( inputFile.ends_with( ".json" ) )
        configuration = rofi::configuration::serialization::fromJSON( nlohmann::json::parse( cfgFile ) );
    else 
        configuration = rofi::configuration::readOldConfigurationFormat( cfgFile );

    return configuration;
}

void affixConfiguration( rofi::configuration::RofiWorld& configuration )
{
    auto modules = configuration.modules();
    assert( modules.size() > 0 );
    const auto & firstModule = modules.begin()->module;
    rofi::configuration::connect< rofi::configuration::RigidJoint >( 
        firstModule->bodies().front(),
        rofi::configuration::Vector( { 0, 0, 0, 1 } ),
        rofi::configuration::matrices::identity );
}
