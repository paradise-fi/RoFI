#include <filesystem>
#include <iostream>

#include <dimcli/cli.h>
#include <google/protobuf/wrappers.pb.h>

#include "configuration/serialization.hpp"
#include "configuration/universalModule.hpp"
#include "message_server.hpp"
#include "simplesim/packet_filters/py_filter.hpp"
#include "simplesim/simplesim.hpp"


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
        _pub->Publish( settingsState.getStateMsg() );
    }


    simplesim::Simplesim & _simplesim;

    gazebo::transport::NodePtr _node;
    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;
};


int main( int argc, char * argv[] )
{
    Dim::Cli cli;
    auto & inputCfgFileName = cli.opt< std::filesystem::path >( "<input_cfg_file>" )
                                      .defaultDesc( {} )
                                      .desc( "Configuration file" );

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

    auto packetFilter = std::unique_ptr< simplesim::packetf::PyFilter >{};
    if ( pythonPacketFilterFileName ) {
        std::cout << "Reading python packet filter from file" << std::endl;
        packetFilter = readPyFilterFromFile( *pythonPacketFilterFileName );
    }

    std::cout << "Starting simplesim" << std::endl;
    auto gzMaster = rofi::msgs::Server::createAndLoopInThread( "simplesim" );

    // Server setup
    auto server = simplesim::Simplesim(
            inputConfiguration,
            packetFilter
                    ? [ &packetFilter ](
                              auto packet ) { return packetFilter->filter( std::move( packet ) ); }
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
        configurationPub->Publish( message );
    } );
}
