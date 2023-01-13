#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <dimcli/cli.h>

#include "configuration/rofiworld.hpp"
#include "configuration/universalModule.hpp"
#include "message_server.hpp"
#include "simplesim/packet_filters/py_filter.hpp"
#include "simplesim/simplesim.hpp"
#include "simplesim_client.hpp"
#include "simplesim_server.hpp"

#include <QtWidgets/QApplication>


namespace configuration = rofi::configuration;
namespace simplesim = rofi::simplesim;

int main( int argc, char * argv[] )
{
    Dim::Cli cli;
    auto opts = simplesim::SimplesimServerOpts( cli );

    auto & qtArgs = cli.optVec< std::string >( "[QT_ARGS]" )
                            .desc( "Optional arguments to pass to the Qt application" );

    if ( !cli.parse( argc, argv ) ) {
        return cli.printError( std::cerr );
    }

    auto qtCArgs = rofi::msgs::getCStyleArgs( argv[ 0 ], *qtArgs );
    auto qtCArgc = static_cast< int >( qtCArgs.size() );
    auto app = QApplication( qtCArgc, qtCArgs.data() );
    setlocale( LC_NUMERIC, "C" );

    auto inputWorld = opts.readInputWorldFile().and_then( rofi::parsing::toSharedAndValidate );
    if ( !inputWorld ) {
        cli.fail( EXIT_FAILURE, "Error while reading input", inputWorld.assume_error() );
        return cli.printError( std::cerr );
    }
    auto packetFilter = opts.getPyPacketFilter();

    std::cout << "Starting gazebo server" << std::endl;
    auto msgServer = rofi::msgs::Server::createAndLoopInThread( "simplesim" );


    // Setup server
    auto server = simplesim::Simplesim(
            *inputWorld,
            packetFilter
                ? [ packetFilter = std::move( *packetFilter ) ]( auto packet ) mutable {
                    return packetFilter.filter( std::move( packet ) );
                }
                : simplesim::PacketFilter::FilterFunction{},
            opts.verbose );

    // Setup client
    auto client = simplesim::SimplesimClient();
    client.setOnSettingsCmdCallback(
            [ &server, &client ]( const simplesim::msgs::SettingsCmd & settingsCmd ) {
                auto settings = server.onSettingsCmd( settingsCmd );
                client.onSettingsResponse( settings.getStateMsg() );
            } );

    std::cout << "Starting simplesim server..." << std::endl;

    // Run server
    auto serverThread = std::jthread( [ &server, &client ]( std::stop_token stopToken ) {
        server.run(
                [ &client ]( std::shared_ptr< const configuration::RofiWorld > newRofiWorld ) {
                    client.onConfigurationUpdate( std::move( newRofiWorld ) );
                },
                stopToken );
    } );

    // Run client
    std::cout << "Adding configuration to the client" << std::endl;
    client.onConfigurationUpdate( std::move( *inputWorld ) );

    std::cout << "Starting simplesim client..." << std::endl;
    client.run();
    app.exec();

    std::cout << "Client ended\n";
}
