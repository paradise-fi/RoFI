#include "check.hpp"
#include <universalModule.hpp>

int check( Dim::Cli & cli ) {
    auto cfgFile = std::ifstream( *inputFile );
    if ( !cfgFile.is_open() )
        throw std::runtime_error( "Cannot open file '" + *inputFile + "'" );

    auto configuration = rofi::configuration::readOldConfigurationFormat( cfgFile );
    rofi::configuration::connect< rofi::configuration::RigidJoint >(
        configuration.getModule( 0 )->body( 0 ),
        Vector( { 0, 0, 0 } ),
        identity );
    auto [ ok, str ] = configuration.isValid( rofi::configuration::SimpleColision() );
    if ( !ok )
        throw std::runtime_error( str );
    return 0;
}

