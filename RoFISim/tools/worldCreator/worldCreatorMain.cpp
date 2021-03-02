#include "worldCreator.hpp"


#define STRINGIFY( x ) #x
#define TOSTRING( x ) STRINGIFY( x )

#ifndef WORLD_FILE
static_assert( false, "world file is not defined" );
#endif

struct CmdArguments
{
    std::optional< std::string > worldFile;
    std::optional< std::string > inputFile;
    std::optional< std::string > outputFile;

    void setArg( const std::string & key, const std::string & value )
    {
        if ( key == "-w" )
        {
            worldFile = value;
            return;
        }
        if ( key == "-i" )
        {
            inputFile = value;
            return;
        }
        if ( key == "-o" )
        {
            outputFile = value;
            return;
        }

        throw std::runtime_error( "Unrecognized command argument: '" + key + "'" );
    }

    static CmdArguments readArguments( const std::vector< std::string > & args )
    {
        if ( args.size() % 2 != 0 )
        {
            throw std::runtime_error( "Wrong number of command arguments" );
        }

        CmdArguments cmdArgs;
        for ( size_t i = 0; i + 1 < args.size(); i += 2 )
        {
            cmdArgs.setArg( args[ i ], args[ i + 1 ] );
        }
        return cmdArgs;
    }
};

int main( int argc, char ** argv )
{
    std::vector< std::string > strArgs;
    for ( int i = 1; i < argc; i++ )
    {
        strArgs.emplace_back( argv[ i ] );
    }
    auto cmdArgs = CmdArguments::readArguments( strArgs );


    std::ifstream ifile;
    if ( cmdArgs.inputFile )
    {
        ifile.open( *cmdArgs.inputFile );
        if ( !ifile.is_open() )
        {
            throw std::runtime_error( "Could not open input file '" + *cmdArgs.inputFile + "'" );
        }
    }

    std::istream & istr = cmdArgs.inputFile ? ifile : std::cin;
    auto configs = ConfigWithPose::readConfigurations( istr );
    for ( auto & config : configs )
    {
        if ( config.config.empty() )
        {
            std::cerr << "Loaded empty config\n";
            continue;
        }
        if ( !config.config.isValid() )
        {
            throw std::runtime_error(
                    "Loaded config is not valid (make sure each config is connected)" );
        }
    }


    auto worldFile = cmdArgs.worldFile ? *cmdArgs.worldFile : std::string( TOSTRING( WORLD_FILE ) );
    auto worldSdf = createWorld( worldFile, configs );


    std::ofstream ofile;
    if ( cmdArgs.outputFile )
    {
        ofile.open( *cmdArgs.outputFile );
        if ( !ofile.is_open() )
        {
            throw std::runtime_error( "Could not open output file '" + *cmdArgs.outputFile + "'" );
        }
    }
    std::ostream & ostr = cmdArgs.outputFile ? ofile : std::cout;
    ostr << worldSdf->ToString();
}
