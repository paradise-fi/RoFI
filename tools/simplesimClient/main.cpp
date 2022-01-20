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


class SimplesimMsgSubscriber
{
public:
    using SettingsStateMsgPtr = boost::shared_ptr< const rofi::simplesim::msgs::SettingsState >;
    using ConfigurationMsgPtr = boost::shared_ptr< const google::protobuf::StringValue >;

    explicit SimplesimMsgSubscriber( rofi::simplesim::SimplesimClient & client )
            : _client( client ), _node( boost::make_shared< gazebo::transport::Node >() )
    {
        assert( _node );
        _node->Init();

        _configurationSub = _node->Subscribe( "~/configuration",
                                              &SimplesimMsgSubscriber::onConfigurationMsg,
                                              this,
                                              true );

        _settingsSub = _node->Subscribe( "~/response",
                                         &SimplesimMsgSubscriber::onSettingsResp,
                                         this,
                                         true );

        assert( _configurationSub );
        assert( _settingsSub );

        std::cout << "Listening for configurations on topic '" << _configurationSub->GetTopic()
                  << "'\nListening for settings responses on topic '" << _settingsSub->GetTopic()
                  << "'\n";
    }

private:
    void onConfigurationMsg( const ConfigurationMsgPtr & msg )
    {
        using namespace rofi::configuration;
        assert( msg );

        auto configuration = std::make_shared< Rofibot >(
                serialization::fromJSON< Rofibot >( nlohmann::json::parse( msg->value() ) ) );
        assert( configuration );

        if ( auto [ ok, err_str ] = configuration->validate( SimpleColision() ); !ok ) {
            std::cerr << "Configuration not valid: '" << err_str << "'" << std::endl;
            return;
        }

        _client.onConfigurationUpdate( std::move( configuration ) );
    }

    void onSettingsResp( const SettingsStateMsgPtr & msgPtr )
    {
        assert( msgPtr );
        auto msgCopy = msgPtr;
        _client.onSettingsResponse( *msgCopy );
    }


    rofi::simplesim::SimplesimClient & _client;

    gazebo::transport::NodePtr _node;
    gazebo::transport::SubscriberPtr _configurationSub;
    gazebo::transport::SubscriberPtr _settingsSub;
};

int main( int argc, char * argv[] )
{
    using rofi::simplesim::SimplesimClient;
    using rofi::simplesim::msgs::SettingsCmd;

    auto msgs_client = rofi::msgs::Client( argc, argv );

    auto node = boost::make_shared< gazebo::transport::Node >();
    node->Init();

    auto settingsCmdPub = node->Advertise< SettingsCmd >( "~/control" );
    std::cout << "Sending settings commands on topic '" << settingsCmdPub->GetTopic() << "'\n";
    auto client = SimplesimClient( [ settingsCmdPub ]( const SettingsCmd & settingsCmd ) {
        assert( settingsCmdPub );
        settingsCmdPub->Publish( settingsCmd );
    } );

    auto simplesimMsgSub = SimplesimMsgSubscriber( client );

    std::cout << "Starting simplesim client..." << std::endl;
    // Runs until the user closes the window
    client.run();

    std::cout << "Client ended\n";
}
