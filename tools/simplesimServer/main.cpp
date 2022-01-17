#include <iostream>

#include <dimcli/cli.h>
#include <google/protobuf/wrappers.pb.h>

#include "configuration/serialization.hpp"
#include "configuration/universalModule.hpp"
#include "message_server.hpp"
#include "simplesim/simplesim.hpp"


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
    if ( auto [ ok, str ] = configuration->isValid( SimpleCollision() ); !ok ) {
        throw std::runtime_error( str );
    }
    return configuration;
}

class SettingsCmdSubscriber
{
public:
    using SettingsCmdMsgPtr = boost::shared_ptr< const rofi::simplesim::msgs::SettingsCmd >;

    SettingsCmdSubscriber( rofi::simplesim::Simplesim & simplesim )
            : _simplesim( simplesim )
            , _node( _simplesim.communication()->node() )
            , _pub( _node->Advertise< rofi::simplesim::msgs::SettingsState >( "~/response" ) )
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
        _pub->Publish( settingsState.getStateMsg() );
    }


    rofi::simplesim::Simplesim & _simplesim;

    gazebo::transport::NodePtr _node;
    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;
};


int main( int argc, char * argv[] )
{
    using rofi::configuration::Rofibot;

    Dim::Cli cli;
    auto & inputCfgFileName = cli.opt< std::string >( "<input_cfg_file>" )
                                      .desc( "Input configuration file" );

    if ( !cli.parse( argc, argv ) ) {
        return cli.printError( std::cerr );
    }

    std::cout << "Reading configuration from file" << std::endl;
    auto inputConfiguration = readConfigurationFromFile( *inputCfgFileName );

    auto gzMaster = rofi::msgs::Server::createAndLoopInThread( "simplesim" );
    std::cout << "Starting simplesim..." << std::endl;

    // Server setup
    auto simplesim = rofi::simplesim::Simplesim( std::move( inputConfiguration ) );

    // Listen for settings cmds
    auto settingsCmdSub = SettingsCmdSubscriber( simplesim );

    // Run the server
    auto configurationPub =
            simplesim.communication()->node()->Advertise< google::protobuf::StringValue >(
                    "~/configuration" );
    std::cout << "Sending configurations on topic '" << configurationPub->GetTopic() << "'\n";

    std::cout << "Simulating..." << std::endl;
    simplesim.run( [ configurationPub ]( std::shared_ptr< const Rofibot > configuration ) {
        assert( configurationPub );
        assert( configuration );
        auto message = google::protobuf::StringValue();
        message.set_value( rofi::configuration::serialization::toJSON( *configuration ).dump() );
        configurationPub->Publish( message );
    } );
}
