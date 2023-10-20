#include <dimcli/cli.h>
#include <atoms/cmdline_utils.hpp>
#include <configuration/rofiworld.hpp>
#include <parsing/parsing.hpp>
#include <isoreconfig/algorithms.hpp>

using namespace rofi::configuration;
using namespace rofi::isoreconfig;
using namespace rofi::parsing;

int main(int argc, char** argv) 
{
    Dim::Cli cli;
    auto & algo = cli.opt< Algorithm >( "a algo" )
        .valueDesc( "algorithm" )
        .desc( "Algorithm to use" )
        .choice( Algorithm::BFStrict, "bfstrict", "bfs for configuration state space" )
        .choice( Algorithm::BFShape, "bfshape", "bfs for shape state space" )
        .choice( Algorithm::BFSEigen, "bfseigen", "bfs for eigen state space" )
        // .choice( Algorithm::BFSCovMat, "bfscov" )
        .choice( Algorithm::ShapeStar, "shapestar", "a* with the convtable heuristic" );
        
    auto & maxDepth = cli.opt<size_t>("m max", 0).desc("Maximum depth for the BFS algorithm to reach; 0 for no limit");

    auto & startInputFile = cli.opt< std::filesystem::path >( "<start_world_file>" )
        .defaultDesc( {} )
        .desc( "Start world (configuration) file ('-' for standard input)" );
    auto & startInputFormat = cli.opt< RofiWorldFormat >( "sf startFormat" )
        .valueDesc( "start_world_format" )
        .desc( "Format of the start RofiWorld file" )
        .choice( RofiWorldFormat::Json, "json" )
        .choice( RofiWorldFormat::Voxel, "voxel" )
        .choice( RofiWorldFormat::Old, "old" );

    auto & targetInputFile = cli.opt< std::filesystem::path >( "<target_world_file>" )
        .defaultDesc( {} )
        .desc( "Target world (configuration) file ('-' for standard input)" );
    auto & targetInputFormat = cli.opt< RofiWorldFormat >( "tf targetFormat" )
        .valueDesc( "target_world_format" )
        .desc( "Format of the target RofiWorld file" )
        .choice( RofiWorldFormat::Json, "json" )
        .choice( RofiWorldFormat::Voxel, "voxel" )
        .choice( RofiWorldFormat::Old, "old" );

    auto & outputPath = cli.opt< std::filesystem::path >( "<found_path_file>" )
        .defaultDesc( {} )
        .desc("File to serialize the found reconfiguration path into as a sequence of RofiWorlds");
    auto & outputFormat = cli.opt< RofiWorldFormat >( "of outputFormat" )
        .valueDesc( "target_world_format" )
        .desc( "Format of the serialized found reconfiguration path" )
        .choice( RofiWorldFormat::Json, "json" )
        .choice( RofiWorldFormat::Voxel, "voxel" )
        .choice( RofiWorldFormat::Old, "old" );

    auto & step = cli.opt<int>("step", 90).desc("Degree of rotation for 1 step");

    if (!cli.parse(argc, argv))
        return cli.printError(std::cerr); // prints error and returns cli.exitCode()

    auto start = atoms::readInput( *startInputFile, [ & ]( std::istream & istr ) {
        return parseRofiWorld( istr, *startInputFormat );
    } );
    if ( !start ) {
        cli.fail( EXIT_FAILURE, "Error while parsing start world", start.assume_error() );
        return cli.printError(std::cerr);
    }

    auto target = atoms::readInput( *targetInputFile, [ & ]( std::istream & istr ) {
        return parseRofiWorld( istr, *targetInputFormat );
    } );
    if ( !target ) {
        cli.fail( EXIT_FAILURE, "Error while parsing target world", target.assume_error() );
        return cli.printError(std::cerr);
    }

    if ( start->referencePoints().empty() )
        fixateRofiWorld( *start );
    if ( target->referencePoints().empty() )
        fixateRofiWorld( *target );

    if ( auto valid = start->validate(); !valid ) {
        cli.fail( EXIT_FAILURE, "Start RofiWorld is invalid", valid.assume_error() );
        return cli.printError(std::cerr);
    }
    if ( auto valid = target->validate(); !valid ) {
        cli.fail( EXIT_FAILURE, "Target RofiWorld is invalid", valid.assume_error() );
        return cli.printError(std::cerr);
    }

    Reporter rep;
    std::vector< RofiWorld > result;

    switch ( *algo )
    {
    case Algorithm::BFStrict:
        result = bfs< NodeType::World >( *start, *target, 
            Angle::deg( static_cast<float>( *step ) ).rad(), rep, *maxDepth );
        break;
    case Algorithm::BFShape:
        result = bfs< NodeType::EigenCloud >( *start, *target, 
            Angle::deg( static_cast<float>( *step ) ).rad(), rep, *maxDepth );
        break;
    case Algorithm::BFSEigen:
        result = bfs< NodeType::Eigen >( *start, *target, 
            Angle::deg( static_cast<float>( *step ) ).rad(), rep, *maxDepth );
        break;
    case Algorithm::ShapeStar:
        result = shapeStar< NodeType::EigenCloud >( *start, *target, 
            Angle::deg( static_cast<float>( *step ) ).rad(), rep );
        break;
    }

    std::cout << rep.toJSON().dump() << "\n";
    std::ofstream out( *outputPath );
    if ( auto written = writeRofiWorldSeq( out, result, *outputFormat ); !written )
    {
        cli.fail( EXIT_FAILURE, "Cannot write to given outputPath", written.assume_error() );
        return cli.printError(std::cerr);
    }
}

