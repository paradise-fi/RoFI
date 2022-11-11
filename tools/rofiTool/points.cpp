#include "points.hpp"
#include "rendering.hpp"

#include <fstream>
#include <stdexcept>

#include <configuration/rofiworld.hpp>
#include <configuration/universalModule.hpp>

static auto command = Dim::Cli().command( "points" )
    .desc( "Interactively preview a set of points defining the configuration" );
static auto& inputFile = command.opt< std::string >( "<FILE>" )
    .desc( "Specify source file" );
static auto& showModules = command.opt< bool >( "modules" )
    .desc( "Show with modules" );

int points( Dim::Cli & /* cli */ ) {
    auto cfgFile = std::ifstream( *inputFile );
    if ( !cfgFile.is_open() )
        throw std::runtime_error( "Cannot open file '" + *inputFile + "'" );

    auto configuration = rofi::configuration::readOldConfigurationFormat( cfgFile );
    rofi::configuration::connect< rofi::configuration::RigidJoint >(
        configuration.modules().begin()->module->bodies()[ 0 ],
        rofi::configuration::matrices::Vector( { 0, 0, 0 } ),
        rofi::configuration::matrices::identity );
    configuration.prepare();

    renderPoints( configuration, *inputFile, *showModules );

    return 0;
}
