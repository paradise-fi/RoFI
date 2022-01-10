#include <iostream>

#include <dimcli/cli.h>
#include <google/protobuf/wrappers.pb.h>

#include "configuration/serialization.hpp"
#include "configuration/universalModule.hpp"
#include "message_server.hpp"
#include "simplesim/communication.hpp"
#include "simplesim/controllers.hpp"
#include "simplesim/simulation.hpp"


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
        std::shared_ptr< const rofi::configuration::Rofibot > rofibotConfiguration )
{
    using namespace rofi::simplesim;

    auto simulation = std::make_shared< Simulation >( std::move( rofibotConfiguration ) );
    auto communication = std::make_shared< Communication >( simulation->commandHandler() );

    return Controller::runRofiController(
            std::move( simulation ),
            std::move( communication ),
            [ pub = communication->node()->Advertise< google::protobuf::StringValue >(
                      "configuration" ) ](
                    std::shared_ptr< const rofi::configuration::Rofibot > configuration ) {
                assert( configuration );
                auto message = google::protobuf::StringValue();
                message.set_value(
                        rofi::configuration::serialization::toJSON( *configuration ).dump() );
                pub->Publish( message );
            } );
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

    auto gzMaster = rofi::msgs::Server::createAndLoopInThread( "simplesim" );
    std::cout << "Starting simplesim..." << std::endl;
    auto controller = runSimplesim( std::move( configuration ) );
    std::cout << "Simulating..." << std::endl;
    controller.wait();
}
