#include "commands.hpp"
#include "rendering.hpp"

#include <configuration/rofiworld.hpp>

static auto command = Dim::Cli().command( "points" )
    .desc( "Interactively preview a set of points defining the configuration" );
static auto& inputFile = command.opt< std::string >( "<FILE>" )
    .desc( "Specify source file" );
static auto& showModules = command.opt< bool >( "modules" )
    .desc( "Show with modules" );

int points( Dim::Cli & /* cli */ ) {
    auto configuration = parseConfiguration( *inputFile );
    
    if ( configuration.modules().size() == 0 )
        throw std::runtime_error( "Configuration in '" + *inputFile + "' does not contain any modules to decompose into points" );

    affixConfiguration( configuration );

    configuration.prepare().get_or_throw_as< std::runtime_error >();

    renderPoints( configuration, *inputFile, *showModules );

    return 0;
}
