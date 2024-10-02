#include <filesystem>
#include <iostream>
#include <thread>

#include <dimcli/cli.h>
#include <google/protobuf/wrappers.pb.h>

#include "configuration/serialization.hpp"
#include "configuration/universalModule.hpp"
#include "message_server.hpp"
#include "simplesim/collision.hpp"
#include "simplesim/packet_filters/py_filter.hpp"
#include "simplesim/simplesim.hpp"
#include "simplesim_server.hpp"


namespace configuration = rofi::configuration;
namespace simplesim = rofi::simplesim;

class SettingsCmdSubscriber {
public:
    using SettingsCmdMsgPtr = boost::shared_ptr< const simplesim::msgs::SettingsCmd >;

    SettingsCmdSubscriber( simplesim::Simplesim & simplesim )
            : _simplesim( simplesim )
            , _node( _simplesim.communication()->node() )
            , _pub( _node->Advertise< simplesim::msgs::SettingsState >( "~/response" ) )
    {
        assert( _simplesim.communication() );
        assert( _node );

        _sub = _node->Subscribe( "~/control", &SettingsCmdSubscriber::onSettingsCmd, this );

        assert( _pub );
        assert( _sub );

        std::cout << "Sending settings responses on topic '" << _pub->GetTopic() << "'\n";
        std::cout << "Listening for settings commands on topic '" << _sub->GetTopic() << "'\n";
    }

    SettingsCmdSubscriber( const SettingsCmdSubscriber & ) = delete;
    SettingsCmdSubscriber & operator=( const SettingsCmdSubscriber & ) = delete;

    ~SettingsCmdSubscriber()
    {
        assert( _sub );
        _sub->Unsubscribe();
    }

private:
    void onSettingsCmd( const SettingsCmdMsgPtr & cmdPtr )
    {
        assert( cmdPtr );
        auto cmdCopy = cmdPtr;

        auto settingsState = _simplesim.onSettingsCmd( *cmdCopy );

        // Workaround for gazebo losing messages
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

        _pub->Publish( settingsState.getStateMsg(), true );
    }


    simplesim::Simplesim & _simplesim;

    gazebo::transport::NodePtr _node;
    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;
};


int main( int argc, char * argv[] )
{
    Dim::Cli cli;
    auto opts = simplesim::SimplesimServerOpts( cli );

    if ( !cli.parse( argc, argv ) ) {
        return cli.printError( std::cerr );
    }

    auto inputWorld = opts.readInputWorldFile().and_then( rofi::parsing::toSharedAndValidate );
    if ( !inputWorld ) {
        cli.fail( EXIT_FAILURE, "Error while reading input", inputWorld.assume_error() );
        return cli.printError( std::cerr );
    }
    auto packetFilter = opts.getPyPacketFilter();

    // Collision model selection
    auto result = getCollisionPtr( opts.collision );
    if ( !result.has_value() ) {
        cli.fail( EXIT_FAILURE, result.assume_error() );
        return cli.printError( std::cerr );
    }
    std::shared_ptr< rofi::configuration::Collision > collModel = result.assume_value();

    std::cout << "Starting simplesim" << std::endl;
    auto gzMaster = rofi::msgs::Server::createAndLoopInThread( "simplesim" );

    // Server setup
    auto server = simplesim::Simplesim(
            *inputWorld,
            packetFilter
                ? [ packetFilter = std::move( *packetFilter ) ]( auto packet ) mutable {
                    return packetFilter.filter( std::move( packet ) );
                }
                : simplesim::PacketFilter::FilterFunction{},
            collModel,
            opts.verbose );

    // Listen for settings cmds
    auto settingsCmdSub = SettingsCmdSubscriber( server );

    // Run the server
    auto configurationPub =
            server.communication()->node()->Advertise< google::protobuf::StringValue >(
                    "~/configuration" );
    std::cout << "Sending configurations on topic '" << configurationPub->GetTopic() << "'\n";

    std::cout << "Simulating..." << std::endl;
    server.run(
            [ configurationPub ]( std::shared_ptr< const configuration::RofiWorld > newRofiWorld ) {
                assert( configurationPub );
                assert( newRofiWorld );
                auto message = google::protobuf::StringValue();
                message.set_value( configuration::serialization::toJSON( *newRofiWorld ).dump() );
                configurationPub->Publish( message, true );
            } );
}
