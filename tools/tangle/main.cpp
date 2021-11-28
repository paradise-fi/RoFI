#include <legacy/configuration/Configuration.h>
#include <legacy/configuration/Generators.h>
#include <legacy/configuration/IO.h>
#include <dimcli/cli.h>
#include <iostream>
#include <random>
#include <cassert>


int main( int argc, char* argv[] ) {
    Dim::Cli cli;
    auto& inputCfgFile = cli.opt< std::string >( "<INPUT_CFG>" );
    auto& stepCount = cli.opt< int >( "s steps" );
    auto& outputFile = cli.opt< std::string >( "[OUTPUT_FILE]" );
    if ( !cli.parse( argc, argv ) )
        return cli.printError( std::cerr );

    std::ofstream outputFileStream;
    if ( outputFile )
        outputFileStream.open( *outputFile );
    std::ostream &outputStream = outputFile ? outputFileStream : std::cout;

    Configuration configuration;
    std::ifstream inputStream( *inputCfgFile );
    IO::readConfiguration( inputStream, configuration );

    std::random_device rd;
    std::mt19937 gen( rd() );

    for ( int i = 0; i < *stepCount; i++ ) {
        configuration.computeMatrices();
        std::vector< Action > availableActions;
        generateSimpleActions( configuration, availableActions, 90 );
        assert( !availableActions.empty() );

        std::uniform_int_distribution< int > selector( 0, availableActions.size() - 1 );

        auto cfg = executeIfValid( configuration, availableActions[ selector( gen ) ] );
        if ( !cfg.has_value() ) {
            i--;
            continue;
        }
        configuration = *cfg;
    }

    outputStream << IO::toString( configuration ) << "\n";

    return 0;
}