#include "commands.hpp"
#include "rendering.hpp"

#include <fstream>
#include <stdexcept>

#include <configuration/rofiworld.hpp>

static auto command = Dim::Cli().command( "preview" )
    .desc( "Interactively preview a configuration" );
static auto& inputFile = command.opt< std::string >( "<FILE>" )
    .desc("Specify source file");

int preview( Dim::Cli & /* cli */ ) {
    auto configuration = parseConfiguration( *inputFile );

    if ( configuration.modules().size() == 0 )
        throw std::runtime_error( "Configuration in '" + *inputFile + "' does not contain any modules to display" );

    affixConfiguration( configuration );

    configuration.prepare().get_or_throw_as< std::runtime_error >();

    renderConfiguration( configuration, *inputFile );

    return 0;
}
