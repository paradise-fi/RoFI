#include <dimcli/cli.h>
#include <universalModule.hpp>

static auto& command = Dim::Cli().command( "check" )
    .desc( "Check a given configuration" );
static auto& inputFile = command.opt< std::string >( "<FILE>" )
    .desc("Specify configuration file");


int check( Dim::Cli & cli ) {
    auto cfgFile = std::ifstream( *inputFile );
    if ( !cfgFile.is_open() )
        throw std::runtime_error( "Cannot open file '" + *inputFile + "'" );

    auto configuration = rofi::readOldConfigurationFormat( cfgFile );
    rofi::connect< rofi::RigidJoint >(
        configuration.getModule( 0 )->body( 0 ),
        Vector( { 0, 0, 0 } ),
        identity );
    auto [ ok, str ] = configuration.isValid( rofi::SimpleColision() );
    if ( !ok )
        throw std::runtime_error( str );
    return 0;
}

