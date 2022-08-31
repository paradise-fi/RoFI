#include <fReconfig.hpp>
#include "dimcli/cli.h"

void visualize( const treeConfig& t, std::string inputFile, std::string outputFile ){
    std::ofstream output;
    output.open( outputFile, std::ofstream::out | std::ofstream::trunc );

    if( !output.is_open() ){
        std::cerr << "Invalid output file\n";
        return;
    }

    Configuration current;
    Configuration last;

    std::ifstream input( inputFile );
    IO::readConfiguration( input, current );

    output << IO::toString( current ); // Initial configuration

    treeConfig treefied( current );
    current = treefied.config;
    output << IO::toString( current ); // Treefied

    last = current;
    for( const auto& step : t.reconfigurationSteps ){
        if( step.type == actionType::rotation ){
            current.getModule( step.id ).setJoint( step.j, step.angle );
        }
        if( step.type == actionType::connection ){
            current.execute( Action( Action::Reconnect( true, step.edge ) ) );
        }
        if( step.type == actionType::disconnect ){
            current.execute( Action( Action::Reconnect( false, step.edge ) ) );
        }
        if( current != last )
            output << IO::toString( current );
        last = current;
    }

}

void reconfigure( std::string inputFile, straightening str, collisionStrategy coll,
                  std::string logFile, std::string outputFile ){
    treeConfig t( inputFile );
    t.collisions = coll;
    t.straight = str;

    auto start = std::chrono::high_resolution_clock::now();
    bool result = t.tryConnections();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast< std::chrono::milliseconds >( end - start );

    if( logFile.empty() ){
        std::cout << "Collision strategy: " << toString( coll ) << "\n"
                  << "Straightening: " << toString( str ) << "\n"
                  << "Result: " << std::boolalpha << result << "\n"
                  << "Reconfiguration time: " << duration.count() << "ms\n";
    } else {
        std::ofstream log( logFile );
        if( !log.is_open() ){
            std::cerr << "Invalid logfile\n";
        }
        log << "{ \"collisions\": \"" << toString( coll ) << "\""
            << ", \"straightening\": \"" << toString( str ) << "\""
            << ", \"result\" : " << std::boolalpha << result
            << ", \"time\" : " << duration.count() << " }";
    }

    if( !outputFile.empty() ){
        visualize( t, inputFile, outputFile );
    }
}

int main( int argc, char** argv ){
    Dim::Cli cli;
    auto& in = cli.opt< std::string >( "<PATH>" ).desc( "Path to input configuration" );
    auto& log = cli.opt< std::string >( "l log" ).desc( "Path to log file (json output)" );
    auto& collisions = cli.opt< collisionStrategy >( "c collisions" ).desc( "Collision strategy" )
        .choice( collisionStrategy::all, "all", "Test all the options" )
        .choice( collisionStrategy::none, "none", "Ignore collisions" )
        .choice( collisionStrategy::naive, "naive", "After computing a connection, reject if it caused a collision" )
        .choice( collisionStrategy::online, "online", "Compute collisions on-line during the algorithm" );

    auto& straight = cli.opt< straightening >( "s straighten" ).desc( "Set when arms straighten" )
        .choice( straightening::all, "all", "Test out all the strategies" )
        .choice( straightening::none, "no", "Never straighten" )
        .choice( straightening::onCollision, "coll", "Straighten an arm when it would cause a collision" )
        .choice( straightening::always, "yes", "Straighten arm after making a connection" );

    auto& out = cli.opt< std::string >( "o output" ).desc( "Path to output file for visualization" );

    if( !cli.parse( argc, argv ) )
        return cli.printError( std::cerr );

    std::ifstream input( *in );
    if( !input.is_open() ){
        std::cerr << "Invalid input file\n";
        return 1;
    }
    input.close();

    for( auto str : { straightening::none, straightening::onCollision, straightening::always } ){
        if( *straight == str || *straight == straightening::all ){
            for( auto coll : { collisionStrategy::none, collisionStrategy::naive, collisionStrategy::online} ){
                if( *collisions == coll || *collisions == collisionStrategy::all )
                    reconfigure( *in, str, coll, *log, *out );
            }
        }
    }


    return 0;
}
