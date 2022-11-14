#include "preview.hpp"
#include "rendering.hpp"

#include <fstream>
#include <stdexcept>

#include <configuration/rofiworld.hpp>
#include <configuration/universalModule.hpp>
#include <configuration/serialization.hpp>

static auto command = Dim::Cli().command( "preview" )
    .desc( "Interactively preview a configuration" );
static auto& inputFile = command.opt< std::string >( "<FILE>" )
    .desc("Specify source file");

int preview( Dim::Cli & /* cli */ ) {
    auto cfgFile = std::ifstream( *inputFile );
    if ( !cfgFile.is_open() )
        throw std::runtime_error( "Cannot open file '" + *inputFile + "'" );

    rofi::configuration::RofiWorld configuration;
    if ( (*inputFile).ends_with( ".json" ) )
        configuration = rofi::configuration::serialization::fromJSON( nlohmann::json::parse( cfgFile ) );
    else 
        configuration = rofi::configuration::readOldConfigurationFormat( cfgFile );

    auto modules = configuration.modules();
    if ( modules.size() == 0 ) 
        throw std::runtime_error( "Configuration in file '" + *inputFile + "' does not contain any modules to display" );

    const auto & firstModule = modules.begin()->module;
    rofi::configuration::connect< rofi::configuration::RigidJoint >( 
        firstModule->bodies().front(),
        rofi::configuration::Vector( { 0, 0, 0 } ),
        rofi::configuration::matrices::identity );

    configuration.prepare().get_or_throw_as< std::runtime_error >();

    renderConfiguration( configuration, *inputFile );

    return 0;
}
