#include "commands.hpp"

#include <configuration/rofiworld.hpp>

static auto command = Dim::Cli().command( "check" )
    .desc( "Check a given configuration" );
static auto& inputFile = command.opt< std::string >( "<FILE>" )
    .desc("Specify configuration file");

int check( Dim::Cli & /* cli */ ) {
    auto configuration = parseConfiguration( *inputFile );

    // Empty configuration is valid
    if ( configuration.modules().size() == 0 )
        return 0;

    affixConfiguration( configuration );

    configuration.validate( rofi::configuration::SimpleCollision() ).get_or_throw_as< std::runtime_error >();

    return 0;
}

