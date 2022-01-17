#include <fstream>
#include <iostream>
#include <stdexcept>

#include <dimcli/cli.h>

#include "configuration/rofibot.hpp"
#include "configuration/universalModule.hpp"
#include "message_server.hpp"
#include "simplesim/simplesim.hpp"
#include "simplesim_client.hpp"


std::shared_ptr< const rofi::configuration::Rofibot > readConfigurationFromFile(
        const std::string & cfgFileName )
{
    using namespace rofi::configuration;

    auto inputCfgFile = std::ifstream( cfgFileName );
    if ( !inputCfgFile.is_open() ) {
        throw std::runtime_error( "Cannot open file '" + cfgFileName + "'" );
    }

    auto configuration = std::make_shared< Rofibot >( readOldConfigurationFormat( inputCfgFile ) );
    assert( configuration );
    auto modules = configuration->modules();
    if ( modules.size() != 0 ) {
        const auto & firstModule = modules.begin()->module;
        assert( firstModule.get() );
        assert( !firstModule->bodies().empty() );
        connect< RigidJoint >( firstModule->bodies().front(),
                               Vector( { 0, 0, 0 } ),
                               matrices::identity );
    }
    configuration->prepare();
    if ( auto [ ok, str ] = configuration->validate( SimpleCollision() ); !ok ) {
        throw std::runtime_error( str );
    }
    return configuration;
}


int main( int argc, char * argv[] )
{
    using rofi::configuration::Rofibot;
    using rofi::simplesim::msgs::SettingsCmd;

    Dim::Cli cli;
    auto & inputCfgFileName = cli.opt< std::string >( "<input_cfg_file>" )
                                      .desc( "Input configuration file" );

    if ( !cli.parse( argc, argv ) ) {
        return cli.printError( std::cerr );
    }

    std::cout << "Reading configuration from file" << std::endl;
    auto configuration = readConfigurationFromFile( *inputCfgFileName );

    std::cout << "Starting gazebo server" << std::endl;
    auto msgServer = rofi::msgs::Server::createAndLoopInThread( "simplesim" );

    // Setup server
    auto server = rofi::simplesim::Simplesim( configuration );

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
    // Runs until the user closes the window
    client.run();

    std::cout << "Client ended\n";
}
