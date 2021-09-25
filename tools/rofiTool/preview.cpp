#include "preview.hpp"
#include "rendering.hpp"

#include <fstream>
#include <stdexcept>

#include <rofibot.hpp>
#include <universalModule.hpp>

static auto command = Dim::Cli().command( "preview" )
    .desc( "Interactively preview a configuration" );
static auto& inputFile = command.opt< std::string >( "<FILE>" )
    .desc("Specify source file");

int preview( Dim::Cli & cli ) {
    auto cfgFile = std::ifstream( *inputFile );
    if ( !cfgFile.is_open() )
        throw std::runtime_error( "Cannot open file '" + *inputFile + "'" );

    auto configuration = rofi::readOldConfigurationFormat( cfgFile );
    rofi::connect< rofi::RigidJoint >(
        configuration.getModule( 0 )->body( 0 ),
        Vector( { 0, 0, 0 } ),
        identity );
    configuration.prepare();

    renderConfiguration( configuration, *inputFile );

    return 0;
}
