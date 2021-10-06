#include <dimcli/cli.h>

static auto command = Dim::Cli().command( "check" )
    .desc( "Check a given configuration" );
static auto& inputFile = command.opt< std::string >( "<FILE>" )
    .desc("Specify configuration file");


int check( Dim::Cli & cli );
