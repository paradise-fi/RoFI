#include <fstream>
#include <iostream>
#include <stdexcept>

#include <dimcli/cli.h>

#include "configuration/rofibot.hpp"
#include "configuration/universalModule.hpp"
#include "message_server.hpp"
#include "simplesim/controllers.hpp"
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
    if ( auto [ ok, str ] = configuration->isValid( SimpleColision() ); !ok ) {
        throw std::runtime_error( str );
    }
    return configuration;
}


[[nodiscard]] rofi::simplesim::Controller runSimplesim(
        std::shared_ptr< const rofi::configuration::Rofibot > rofibotConfiguration,
        rofi::simplesim::Controller::OnConfigurationUpdate onConfigurationUpdate )
{
    using namespace rofi::simplesim;

    auto simulation = std::make_shared< Simulation >( std::move( rofibotConfiguration ) );
    auto communication = std::make_shared< Communication >( simulation->commandHandler() );

    return Controller::runRofiController( std::move( simulation ),
                                          std::move( communication ),
                                          std::move( onConfigurationUpdate ) );
}


int main( int argc, char * argv[] )
{
    Dim::Cli cli;
    auto & inputCfgFileName = cli.opt< std::string >( "<input_cfg_file>" )
                                      .desc( "Input configuration file" );

    if ( !cli.parse( argc, argv ) ) {
        return cli.printError( std::cerr );
    }

    std::cout << "Reading configuration from file" << std::endl;
    auto configuration = readConfigurationFromFile( *inputCfgFileName );

    std::cout << "Starting gazebo server" << std::endl;
    auto msgServer = rofi::msgs::MessageServer::createAndLoopInThread( "simplesim" );

    auto client = rofi::simplesim::SimplesimClient();

    std::cout << "Starting simplesim server..." << std::endl;
    auto server = runSimplesim( configuration, [ &client ]( auto rofiConfig ) {
        client.onConfigurationUpdate( std::move( rofiConfig ) );
    } );

    std::cout << "Adding configuration to the client" << std::endl;
    client.onConfigurationUpdate( configuration );
    configuration.reset();

    std::cout << "Starting simplesim client..." << std::endl;
    // Runs until the user closes the window
    client.run();

    std::cout << "Client ended\n";
}
