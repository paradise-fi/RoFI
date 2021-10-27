#include "check.hpp"
#include <configuration/universalModule.hpp>

static auto command = Dim::Cli().command( "check" )
    .desc( "Check a given configuration" );
static auto& inputFile = command.opt< std::string >( "<FILE>" )
    .desc("Specify configuration file");

int check( Dim::Cli & cli ) {
    auto cfgFile = std::ifstream( *inputFile );
    if ( !cfgFile.is_open() )
        throw std::runtime_error( "Cannot open file '" + *inputFile + "'" );

    auto configuration = rofi::configuration::readOldConfigurationFormat( cfgFile );
    rofi::configuration::connect< rofi::configuration::RigidJoint >(
        configuration.getModule( 0 )->bodies()[ 0 ],
        Vector( { 0, 0, 0 } ),
        identity );
    auto [ ok, str ] = configuration.isValid( rofi::configuration::SimpleColision() );
    if ( !ok )
        throw std::runtime_error( str );
    return 0;
}

