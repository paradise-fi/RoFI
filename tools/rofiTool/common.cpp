#include "common.hpp"

#include <fstream>
#include <stdexcept>

#include <atoms/result.hpp>
#include <configuration/rofiworld.hpp>
#include <configuration/serialization.hpp>
#include <configuration/universalModule.hpp>


atoms::Result< rofi::configuration::RofiWorld > parseRofiWorld( const std::string & inputFile )
{
    using namespace rofi::configuration;

    auto cfgFile = std::ifstream( inputFile );
    if ( !cfgFile.is_open() ) {
        return atoms::result_error( "Cannot open file '" + inputFile + "'" );
    }

    try {
        if ( inputFile.ends_with( ".json" ) ) {
            auto json = nlohmann::json::parse( cfgFile );
            return atoms::result_value( serialization::fromJSON( json ) );
        }

        return atoms::result_value( readOldConfigurationFormat( cfgFile ) );
    } catch ( const std::exception & e ) {
        return atoms::result_error< std::string >( e.what() );
    }
}

void affixRofiWorld( rofi::configuration::RofiWorld & world )
{
    using namespace rofi::configuration;
    auto modules = world.modules();
    assert( !modules.empty() );
    const auto & firstModule = modules.begin()->module;
    assert( firstModule );
    assert( !firstModule->components().empty() );
    auto component = !firstModule->bodies().empty() ? firstModule->bodies().front()
                                                    : firstModule->components().front();
    connect< RigidJoint >( component, {}, matrices::identity );
}
