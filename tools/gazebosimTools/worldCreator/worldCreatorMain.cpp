#include <algorithm>
#include <string_view>

#include "worldCreator.hpp"


#ifndef WORLD_FILE
static_assert( false, "world file is not defined" );
#endif

#define STRINGIFY( x ) #x
#define TOSTRING( x ) STRINGIFY( x )

constexpr std::string_view worldFileName = std::string_view( TOSTRING( WORLD_FILE ) );

#undef TOSTRING
#undef STRINGIFY


struct CmdArguments
{
    std::optional< std::string > worldFile;
    std::optional< std::string > inputFile;
    std::optional< std::string > outputFile;

    void setArg( std::string_view key, std::string_view value )
    {
        if ( key == "-w" )
        {
            worldFile = std::string( value );
            return;
        }
        if ( key == "-i" )
        {
            inputFile = std::string( value );
            return;
        }
        if ( key == "-o" )
        {
            outputFile = std::string( value );
            return;
        }

        throw std::runtime_error( "Unrecognized command argument: '" + std::string( key ) + "'" );
    }

    static CmdArguments readArguments( const std::vector< std::string_view > & args )
    {
        CmdArguments cmdArgs;
        for ( size_t i = 0; i + 1 < args.size(); i += 2 )
        {
            cmdArgs.setArg( args[ i ], args[ i + 1 ] );
        }

        if ( args.size() % 2 != 0 )
        {
            throw std::runtime_error( "Unexpected last command argument '"
                                      + std::string( args.back() ) + "'" );
        }
        return cmdArgs;
    }
};

void printHelp( std::string_view programName )
{
    std::cerr << "Usage:\n\t" << programName
              << " [-i <input_file>] [-o <output_file>] [-w <empty_world_file>]\n\n"
              << "Runs the World creator tool.\n"
              << "You have to be in RoFI environment for this tool to work properly.\n"
              << "If input/output file is not set, the standard input/output is used.\n"
              << "Make sure that the <empty_world_file> "
              << "has distributor and attacher plugins.\n";
}

int main( int argc, char ** argv )
{
    auto strArgs = std::vector< std::string_view >( argv + 1, argv + argc );
    if ( std::any_of( strArgs.begin(), strArgs.end(), []( auto arg ) {
             return arg == "-h" || arg == "--help";
         } ) )
    {
        printHelp( argv[ 0 ] );
        return 1;
    }

    CmdArguments cmdArgs;
    try
    {
        cmdArgs = CmdArguments::readArguments( strArgs );
    }
    catch ( const std::exception & error )
    {
        std::cerr << error.what() << "\n\n";
        printHelp( argv[ 0 ] );
        return 1;
    }


    std::ifstream ifile;
    if ( cmdArgs.inputFile )
    {
        ifile.open( *cmdArgs.inputFile );
        if ( !ifile.is_open() )
        {
            std::cerr << "Could not open input file '" << *cmdArgs.inputFile << "'\n";
            return 1;
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
            std::cerr << "Loaded config is not valid (make sure each config is connected)\n";
            return 1;
        }
    }


    auto worldFile = cmdArgs.worldFile ? *cmdArgs.worldFile : std::string( worldFileName );
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
