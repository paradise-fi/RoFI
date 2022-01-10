#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <gazebo/gazebo_client.hh>
#include <gazebo/transport/Node.hh>
#include <google/protobuf/wrappers.pb.h>
#include <nlohmann/json.hpp>

#include "configuration/serialization.hpp"
#include "message_server.hpp"
#include "simplesim_client.hpp"


struct OnConfigurationMsg
{
    void onConfigurationMsg( const boost::shared_ptr< const google::protobuf::StringValue > & msg )
    {
        using namespace rofi::configuration;
        assert( msg );

        auto configuration = std::make_shared< Rofibot >(
                serialization::fromJSON< Rofibot >( nlohmann::json::parse( msg->value() ) ) );
        assert( configuration );
        configuration->prepare();

        if ( auto [ ok, err_str ] = configuration->isValid( SimpleColision() ); !ok ) {
            std::cerr << "Configuration not valid: '" << err_str << "'" << std::endl;
            return;
        }

        client.onConfigurationUpdate( std::move( configuration ) );
    }

    rofi::simplesim::SimplesimClient & client;
};

int main( int argc, char * argv[] )
{
    auto msgs_client = rofi::msgs::Client( argc, argv );

    auto client = rofi::simplesim::SimplesimClient();

    auto on_configuration_msg = OnConfigurationMsg{ .client = client };
    auto node = boost::make_shared< gazebo::transport::Node >();
    node->Init();
    auto sub = node->Subscribe( "~/simplesim",
                                &OnConfigurationMsg::onConfigurationMsg,
                                &on_configuration_msg,
                                true );

    std::cout << "Listening on topic '" << sub->GetTopic() << "'\n";

    std::cout << "Starting simplesim client..." << std::endl;
    // Runs until the user closes the window
    client.run();

    std::cout << "Client ended\n";
}
