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
        if( current != last && step.type != actionType::rotation )
            output << IO::toString( current );
        last = current;
    }
    output << IO::toString( current );

}

struct inspector: public treeConfigInspection {
    void onReconfigurationStart() override {
        globalStart = std::chrono::high_resolution_clock::now();
    };

    void onReconfigurationEnd() override {
        globalEnd = std::chrono::high_resolution_clock::now();
    };

    void onBacktrack() override {
        backtrackCount++;
    }

    void onArmConnectionStart() override {
        armStart = std::chrono::high_resolution_clock::now();
    }

    void onArmConnectionEnd() override {
        auto armEnd = std::chrono::high_resolution_clock::now();
        armConnectionCount++;
        armConnectionTime += std::chrono::duration_cast< std::chrono::microseconds >(
            armEnd - armStart );
    }

    auto reconfigurationTime() const {
        return std::chrono::duration_cast< std::chrono::milliseconds >( globalEnd - globalStart );
    }


    int backtrackCount = 0;
    int armConnectionCount = 0;
    std::chrono::high_resolution_clock::time_point globalStart;
    std::chrono::high_resolution_clock::time_point globalEnd;
    std::chrono::high_resolution_clock::time_point armStart;
    std::chrono::microseconds armConnectionTime;
};

void reconfigure( std::string inputFile, straightening str, collisionStrategy coll,
                  std::string logFile, std::string outputFile ){
    treeConfig t( inputFile );
    t.inspector = std::unique_ptr< treeConfigInspection >( new inspector() );
    t.collisions = coll;
    t.straight = str;

    bool result = t.reconfig();

    auto insp = static_cast< const inspector *>( t.inspector.get() );

    if( logFile.empty() ){
        std::cout << "Collision strategy: " << toString( coll ) << "\n"
                  << "Straightening: " << toString( str ) << "\n"
                  << "Result: " << std::boolalpha << result << "\n"
                  << "Reconfiguration time: " << insp->reconfigurationTime().count() << "ms\n"
                  << "Number of backtracks: " << insp->backtrackCount << "\n"
                  << "Time spent in arm connections: "
                        << std::chrono::duration_cast< std::chrono::milliseconds >(
                            insp->armConnectionTime ).count() << "ms\n"
                  << "Number of connection attempts: " << insp->armConnectionCount << "\n"
                  << "Average connection time: " << std::chrono::duration_cast< std::chrono::milliseconds >(
                            insp->armConnectionTime ).count() / insp->armConnectionCount << "ms\n";
    } else {
        std::ofstream log( logFile );
        if( !log.is_open() ){
            std::cerr << "Invalid logfile\n";
        }
        log << "{ \"collisions\": \"" << toString( coll ) << "\""
            << ", \"straightening\": \"" << toString( str ) << "\""
            << ", \"result\": " << std::boolalpha << result
            << ", \"time\": " << insp->reconfigurationTime().count()
            << ", \"backtracks\": " << insp->backtrackCount
            << ", \"armConnections\": " << insp->armConnectionCount
            << ", \"armConnectionsTime\": " << std::chrono::duration_cast< std::chrono::milliseconds >(
                            insp->armConnectionTime ).count()
            << " }";
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
