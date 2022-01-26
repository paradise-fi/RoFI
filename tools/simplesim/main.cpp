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

#include <QtWidgets/QApplication>

std::shared_ptr< const rofi::configuration::Rofibot > readConfigurationFromFile(
        const std::filesystem::path & cfgFileName )
{
    using namespace rofi::configuration;

    auto inputCfgFile = std::ifstream( cfgFileName );
    if ( !inputCfgFile.is_open() ) {
        throw std::runtime_error( "Cannot open file '" + cfgFileName.generic_string() + "'" );
    }

    auto configuration = std::make_shared< Rofibot >( readOldConfigurationFormat( inputCfgFile ) );
    assert( configuration );
    auto modules = configuration->modules();
    if ( modules.size() != 0 ) {
        const auto & firstModule = modules.begin()->module;
        assert( firstModule.get() );
        assert( !firstModule->bodies().empty() );
        connect< RigidJoint >( firstModule->bodies()[ 1 ],
                               Vector( { 0, 0, 0 } ),
                               matrices::identity );
    }
    configuration->prepare();
    if ( auto [ ok, str ] = configuration->validate( SimpleCollision() ); !ok ) {
        throw std::runtime_error( str );
    }
    return configuration;
}

auto readPyFilterFromFile( const std::filesystem::path & packetFilterFileName )
        -> std::unique_ptr< rofi::simplesim::packetf::PyFilter >
{
    using namespace rofi::simplesim::packetf;

    auto inputFile = std::ifstream( packetFilterFileName );
    if ( !inputFile.is_open() ) {
        throw std::runtime_error( "Cannot open file '" + packetFilterFileName.generic_string()
                                  + "'" );
    }

    auto fileContent = std::string( std::istreambuf_iterator( inputFile ), {} );
    inputFile.close();

    return std::make_unique< PyFilter >( fileContent );
}

int main( int argc, char * argv[] )
{
    using rofi::configuration::Rofibot;
    using rofi::simplesim::msgs::SettingsCmd;

    Dim::Cli cli;
    auto & inputCfgFileName = cli.opt< std::filesystem::path >( "<input_cfg_file>" )
                                      .defaultDesc( {} )
                                      .desc( "Input configuration file" );

    auto & pythonPacketFilterFileName = cli.opt< std::filesystem::path >( "p python" )
                                                .valueDesc( "PYTHON_FILE" )
                                                .defaultDesc( {} )
                                                .desc( "Python packet filter file" );

    if ( !cli.parse( argc, argv ) ) {
        return cli.printError( std::cerr );
    }


    QApplication app( argc, argv );
    setlocale( LC_NUMERIC, "C" );

    std::cout << "Reading configuration from file" << std::endl;
    auto configuration = readConfigurationFromFile( *inputCfgFileName );

    auto packetFilter = std::unique_ptr< rofi::simplesim::packetf::PyFilter >{};
    if ( pythonPacketFilterFileName ) {
        std::cout << "Reading python packet filter from file" << std::endl;
        packetFilter = readPyFilterFromFile( *pythonPacketFilterFileName );
    }

    std::cout << "Starting gazebo server" << std::endl;
    auto msgServer = rofi::msgs::Server::createAndLoopInThread( "simplesim" );


    // Setup server
    auto server = rofi::simplesim::Simplesim(
            configuration,
            packetFilter
                    ? [ &packetFilter ](
                              auto packet ) { return packetFilter->filter( std::move( packet ) ); }
                    : rofi::simplesim::PacketFilter::FilterFunction{} );

    // Setup client
    auto client = rofi::simplesim::SimplesimClient();
    client.setOnSettingsCmdCallback( [ &server, &client ]( const SettingsCmd & settingsCmd ) {
        auto settings = server.onSettingsCmd( settingsCmd );
        client.onSettingsResponse( settings.getStateMsg() );
    } );

    std::cout << "Starting simplesim server..." << std::endl;

    // Run server
    auto serverThread = std::jthread( [ &server, &client ]( std::stop_token stopToken ) {
        server.run(
                [ &client ]( std::shared_ptr< const Rofibot > newConfiguration ) {
                    client.onConfigurationUpdate( std::move( newConfiguration ) );
                },
                stopToken );
    } );

    // Run client
    std::cout << "Adding configuration to the client" << std::endl;
    client.onConfigurationUpdate( configuration );
    configuration.reset();

    std::cout << "Starting simplesim client..." << std::endl;
    client.run();
    app.exec();

    std::cout << "Client ended\n";
}
