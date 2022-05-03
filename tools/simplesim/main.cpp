#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <dimcli/cli.h>

#include "configuration/rofibot.hpp"
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
    auto & cfgFilePath = simplesim::cfgFilePathCliOpt( cli );
    auto & cfgFormat = simplesim::cfgFormatCliOpt( cli );
    auto & pyPacketFilterFilePath = simplesim::pyPacketFilterFilePathCliOpt( cli );

    auto & verbose = cli.opt< bool >( "v verbose" ).desc( "Run simulator in verbose mode" );

    auto & qtArgs = cli.optVec< std::string >( "[QT_ARGS]" )
                            .desc( "Optional arguments to pass to the Qt application" );

    if ( !cli.parse( argc, argv ) ) {
        return cli.printError( std::cerr );
    }

    auto qtCArgs = rofi::msgs::getCStyleArgs( argv[ 0 ], *qtArgs );
    auto qtCArgc = static_cast< int >( qtCArgs.size() );
    auto app = QApplication( qtCArgc, qtCArgs.data() );
    setlocale( LC_NUMERIC, "C" );

    std::cout << "Reading configuration from file (" << *cfgFormat << " format)" << std::endl;
    auto inputRofibot = simplesim::readAndPrepareConfigurationFromFile( *cfgFilePath, *cfgFormat );

    auto packetFilter = std::optional< simplesim::packetf::PyFilter >();
    if ( pyPacketFilterFilePath ) {
        std::cout << "Reading python packet filter from file" << std::endl;
        packetFilter = simplesim::readPyFilterFromFile( *pyPacketFilterFilePath );
    }

    std::cout << "Starting gazebo server" << std::endl;
    auto msgServer = rofi::msgs::Server::createAndLoopInThread( "simplesim" );


    // Setup server
    auto server = simplesim::Simplesim(
            inputRofibot,
            packetFilter
                ? [ packetFilter = std::move( *packetFilter ) ]( auto packet ) mutable {
                    return packetFilter.filter( std::move( packet ) );
                }
                : simplesim::PacketFilter::FilterFunction{},
            *verbose);

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
                [ &client ]( std::shared_ptr< const configuration::Rofibot > newConfiguration ) {
                    client.onConfigurationUpdate( std::move( newConfiguration ) );
                },
                stopToken );
    } );

    // Run client
    std::cout << "Adding configuration to the client" << std::endl;
    client.onConfigurationUpdate( inputRofibot );
    inputRofibot.reset();

    std::cout << "Starting simplesim client..." << std::endl;
    client.run();
    app.exec();

    std::cout << "Client ended\n";
}
