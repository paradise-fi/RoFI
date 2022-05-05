#include <filesystem>
#include <iostream>

#include <dimcli/cli.h>
#include <google/protobuf/wrappers.pb.h>

#include "configuration/serialization.hpp"
#include "configuration/universalModule.hpp"
#include "message_server.hpp"
#include "simplesim/packet_filters/py_filter.hpp"
#include "simplesim/simplesim.hpp"


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

class SettingsCmdSubscriber {
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
    auto & inputCfgFileName = cli.opt< std::filesystem::path >( "<input_cfg_file>" )
                                      .defaultDesc( {} )
                                      .desc( "Input configuration file" );

    auto & pythonPacketFilterFileName = cli.opt< std::filesystem::path >( "p python" )
                                                .valueDesc( "PYTHON_FILE" )
                                                .defaultDesc( {} )
                                                .desc( "Python packet filter file" );

    auto & verbose = cli.opt< bool >( "v verbose" ).desc( "Run simulator in verbose mode" );

    if ( !cli.parse( argc, argv ) ) {
        return cli.printError( std::cerr );
    }

    std::cout << "Reading configuration from file" << std::endl;
    auto inputConfiguration = readConfigurationFromFile( *inputCfgFileName );

    auto packetFilter = std::unique_ptr< rofi::simplesim::packetf::PyFilter >{};
    if ( pythonPacketFilterFileName ) {
        std::cout << "Reading python packet filter from file" << std::endl;
        packetFilter = readPyFilterFromFile( *pythonPacketFilterFileName );
    }

    std::cout << "Starting simplesim" << std::endl;
    auto gzMaster = rofi::msgs::Server::createAndLoopInThread( "simplesim" );

    // Server setup
    auto server = rofi::simplesim::Simplesim(
            inputConfiguration,
            packetFilter
                    ? [ &packetFilter ](
                              auto packet ) { return packetFilter->filter( std::move( packet ) ); }
                    : rofi::simplesim::PacketFilter::FilterFunction{},
            *verbose );

    // Listen for settings cmds
    auto settingsCmdSub = SettingsCmdSubscriber( server );

    // Run the server
    auto configurationPub =
            server.communication()->node()->Advertise< google::protobuf::StringValue >(
                    "~/configuration" );
    std::cout << "Sending configurations on topic '" << configurationPub->GetTopic() << "'\n";

    std::cout << "Simulating..." << std::endl;
    server.run( [ configurationPub ]( std::shared_ptr< const Rofibot > configuration ) {
        assert( configurationPub );
        assert( configuration );
        auto message = google::protobuf::StringValue();
        message.set_value( rofi::configuration::serialization::toJSON( *configuration ).dump() );
        configurationPub->Publish( message );
    } );
}
