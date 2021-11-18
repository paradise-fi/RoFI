#include "preview.hpp"
#include "rendering.hpp"

#include <fstream>
#include <stdexcept>

#include <configuration/rofibot.hpp>
#include <configuration/universalModule.hpp>

static auto command = Dim::Cli().command( "preview" )
    .desc( "Interactively preview a configuration" );
static auto& inputFile = command.opt< std::string >( "<FILE>" )
    .desc("Specify source file");
static auto& changePrepared = command.opt< bool >( "<changePrepared>" )
    .desc("True to call prepare before changing");

int preview( Dim::Cli & cli ) {
    auto cfgFile = std::ifstream( *inputFile );
    if ( !cfgFile.is_open() )
        throw std::runtime_error( "Cannot open file '" + *inputFile + "'" );

    auto configuration = rofi::configuration::readOldConfigurationFormat( cfgFile );
    rofi::configuration::connect< rofi::configuration::RigidJoint >(
        configuration.modules().begin()->module->bodies()[ 0 ],
        Vector( { 0, 0, 0 } ),
        identity );
    if ( *changePrepared ) {
        configuration.prepare();
    }
    configuration.getModule( 0 )->joints()[ 2 ].joint->setPositions( std::array{ 1.f } );
    configuration.prepare();

    renderConfiguration( configuration, *inputFile );

    return 0;
}
