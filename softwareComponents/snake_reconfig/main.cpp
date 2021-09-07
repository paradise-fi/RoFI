#include <exception>
#include <fstream>
#include <tuple>
#include <iomanip>
#include <sstream>
#include "../configuration/Configuration.h"
#include "../configuration/IO.h"
#include "Snake_algorithms.h"
#include <dimcli/cli.h>

Dim::Cli cli;
auto& inputCfgFile = cli.opt< std::string >( "<INPUT_CFG>" );
auto& logFile = cli.opt< std::string >( "l log" );
auto& outputFile = cli.opt< std::string >( "[OUTPUT_FILE]" );

std::string storePath(const std::vector<Configuration>& configs ) {
    std::ostringstream file;
    for (const auto& conf : configs) {
        file << IO::toString( conf ) << std::endl;
    }
    return file.str();
}

nlohmann::json gCurrentProgress;
std::string gLogPath;

void finishLog() {
    if ( gLogPath.empty() )
        return;
    gCurrentProgress["input"] = *inputCfgFile;
    std::ofstream f( gLogPath );
    f << std::setw( 4 ) << gCurrentProgress << "\n";
}

int main(int argc, char* argv[])
{
    if ( !cli.parse( argc, argv ) )
        return cli.printError( std::cerr );

    gLogPath = *logFile;

    finishLog();

    std::ifstream initInput( *inputCfgFile );
    Configuration init;
    IO::readConfiguration( initInput, init );
    init.computeMatrices();

    auto [reconfigPath, success] = reconfigToSnake(init, [&]( auto... args ) {
        gCurrentProgress = logProgressJson( std::forward< decltype( args ) >( args )... );
        finishLog();
    });

    auto path = storePath( reconfigPath );

    if ( success )
        gCurrentProgress["path"] = path;

    if ( outputFile && success ) {
        std::ofstream f( *outputFile );
        f << path;
    }

    finishLog();

    return 0;
}