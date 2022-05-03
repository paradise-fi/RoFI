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


namespace configuration = rofi::configuration;
namespace simplesim = rofi::simplesim;

std::shared_ptr< const configuration::Rofibot > readConfigurationFromFile(
        const std::filesystem::path & cfgFileName )
{
    auto inputCfgFile = std::ifstream( cfgFileName );
    if ( !inputCfgFile.is_open() ) {
        throw std::runtime_error( "Cannot open file '" + cfgFileName.generic_string() + "'" );
    }

    auto rofibot = std::make_shared< configuration::Rofibot >(
            configuration::readOldConfigurationFormat( inputCfgFile ) );
    assert( rofibot );
    auto modules = rofibot->modules();
    if ( modules.size() != 0 ) {
        const auto & firstModule = modules.begin()->module;
        assert( firstModule.get() );
        assert( !firstModule->bodies().empty() );
        connect< configuration::RigidJoint >( firstModule->bodies().front(),
                                              configuration::Vector( { 0, 0, 0 } ),
                                              configuration::matrices::identity );
    }
    rofibot->prepare();
    if ( auto [ ok, str ] = rofibot->isValid( configuration::SimpleCollision() ); !ok ) {
        throw std::runtime_error( str );
    }
    return rofibot;
}

auto readPyFilterFromFile( const std::filesystem::path & packetFilterFileName )
        -> std::unique_ptr< simplesim::packetf::PyFilter >
{
    auto inputFile = std::ifstream( packetFilterFileName );
    if ( !inputFile.is_open() ) {
        throw std::runtime_error( "Cannot open file '" + packetFilterFileName.generic_string()
                                  + "'" );
    }

    auto fileContent = std::string( std::istreambuf_iterator( inputFile ), {} );
    inputFile.close();

    return std::make_unique< simplesim::packetf::PyFilter >( fileContent );
}

int main( int argc, char * argv[] )
{
    Dim::Cli cli;
    auto & inputCfgFileName = cli.opt< std::filesystem::path >( "<input_cfg_file>" )
                                      .defaultDesc( {} )
                                      .desc( "Input configuration file" );

    auto & pythonPacketFilterFileName = cli.opt< std::filesystem::path >( "p python" )
                                                .valueDesc( "PYTHON_FILE" )
                                                .defaultDesc( {} )
                                                .desc( "Python packet filter file" );

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

    std::cout << "Reading configuration from file" << std::endl;
    auto inputConfiguration = readConfigurationFromFile( *inputCfgFileName );

    auto packetFilter = std::unique_ptr< simplesim::packetf::PyFilter >{};
    if ( pythonPacketFilterFileName ) {
        std::cout << "Reading python packet filter from file" << std::endl;
        packetFilter = readPyFilterFromFile( *pythonPacketFilterFileName );
    }

    std::cout << "Starting gazebo server" << std::endl;
    auto msgServer = rofi::msgs::Server::createAndLoopInThread( "simplesim" );


    // Setup server
    auto server = simplesim::Simplesim(
            inputConfiguration,
            packetFilter
                    ? [ &packetFilter ](
                              auto packet ) { return packetFilter->filter( std::move( packet ) ); }
                    : simplesim::PacketFilter::FilterFunction{},
            *verbose );

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
    client.onConfigurationUpdate( inputConfiguration );
    inputConfiguration.reset();

    std::cout << "Starting simplesim client..." << std::endl;
    client.run();
    app.exec();

    std::cout << "Client ended\n";
}
