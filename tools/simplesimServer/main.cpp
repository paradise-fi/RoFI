#include <filesystem>
#include <iostream>
#include <thread>

#include <dimcli/cli.h>
#include <google/protobuf/wrappers.pb.h>

#include "configuration/serialization.hpp"
#include "configuration/universalModule.hpp"
#include "message_server.hpp"
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

    auto & cfgFilePath = simplesim::cfgFilePathCliOpt( cli );
    auto & cfgFormat = simplesim::cfgFormatCliOpt( cli );
    auto & pyPacketFilterFilePath = simplesim::pyPacketFilterFilePathCliOpt( cli );

    auto & verbose = cli.opt< bool >( "v verbose" ).desc( "Run simulator in verbose mode" );

    if ( !cli.parse( argc, argv ) ) {
        return cli.printError( std::cerr );
    }

    std::cout << "Reading configuration from file (" << *cfgFormat << " format)" << std::endl;
    auto inputRofibot = simplesim::readAndPrepareConfigurationFromFile( *cfgFilePath, *cfgFormat );

    auto packetFilter = std::optional< simplesim::packetf::PyFilter >();
    if ( pyPacketFilterFilePath ) {
        std::cout << "Reading python packet filter from file" << std::endl;
        packetFilter = simplesim::readPyFilterFromFile( *pyPacketFilterFilePath );
    }

    std::cout << "Starting simplesim" << std::endl;
    auto gzMaster = rofi::msgs::Server::createAndLoopInThread( "simplesim" );

    // Server setup
    auto server = simplesim::Simplesim(
            inputRofibot,
            packetFilter
                ? [ packetFilter = std::move( *packetFilter ) ]( auto packet ) mutable {
                    return packetFilter.filter( std::move( packet ) );
                }
                : simplesim::PacketFilter::FilterFunction{},
            *verbose );

    // Listen for settings cmds
    auto settingsCmdSub = SettingsCmdSubscriber( server );

    // Run the server
    auto configurationPub =
            server.communication()->node()->Advertise< google::protobuf::StringValue >(
                    "~/configuration" );
    std::cout << "Sending configurations on topic '" << configurationPub->GetTopic() << "'\n";

    std::cout << "Simulating..." << std::endl;
    server.run( [ configurationPub ]( std::shared_ptr< const configuration::Rofibot > rofibot ) {
        assert( configurationPub );
        assert( rofibot );
        auto message = google::protobuf::StringValue();
        message.set_value( configuration::serialization::toJSON( *rofibot ).dump() );
        configurationPub->Publish( message, true );
    } );
}
