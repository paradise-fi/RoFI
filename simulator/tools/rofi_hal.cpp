#include "rofi_hal.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <utility>

#include <gazebo/gazebo_client.hh>
#include <gazebo/transport/transport.hh>

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>

#ifndef LOCAL_ROFI_ID
#define LOCAL_ROFI_ID 0
#endif


constexpr rofi::hal::RoFI::Id localRoFIId = LOCAL_ROFI_ID;

namespace
{
using namespace rofi::hal;
namespace msgs = rofi::messages;

class GazeboClientHolder
{
    GazeboClientHolder()
    {
        std::cerr << "Local RoFI has id: " << localRoFIId << "\n";
        std::cerr << "Starting gazebo client" << std::endl;
        gazebo::client::setup();
    }

public:
    GazeboClientHolder( const GazeboClientHolder & ) = delete;
    GazeboClientHolder & operator=( const GazeboClientHolder & ) = delete;

    ~GazeboClientHolder()
    {
        gazebo::client::shutdown();
        std::cerr << "Ending gazebo client" << std::endl;
    }

    static std::shared_ptr< GazeboClientHolder > get()
    {
        static std::mutex instanceMutex;
        std::lock_guard< std::mutex > lock( instanceMutex );

        static std::weak_ptr< GazeboClientHolder > weakInstance;
        auto instance = weakInstance.lock();

        if ( !instance )
        {
            instance = std::shared_ptr< GazeboClientHolder >( new GazeboClientHolder() );
            weakInstance = instance;
        }

        assert( weakInstance.lock() == instance );
        assert( instance );
        return instance;
    }
};

class ConnectorSim;
class JointSim;

/**
 * HAL RoFI implementation for Gazebo simulator
 */
class RoFISim
        : public RoFI::Implementation
        , public std::enable_shared_from_this< RoFISim >
{
    explicit RoFISim( RoFI::Id id ) : _id( id ), _clientHolder( GazeboClientHolder::get() )
    {
    }

public:
    using RofiRespPtr = boost::shared_ptr< const msgs::RofiResp >;


    static std::shared_ptr< RoFISim > create()
    {
        return create( localRoFIId );
    }

    static std::shared_ptr< RoFISim > create( RoFI::Id id )
    {
        auto newRoFI = std::shared_ptr< RoFISim >( new RoFISim( id ) );
        newRoFI->init();
        return newRoFI;
    }

    void init()
    {
        _node = boost::make_shared< gazebo::transport::Node >();
        _node->Init();

        if ( _id < 0 )
        {
            throw std::runtime_error( "Unknown id" );
        }

        std::string moduleName = "universalModule";
        if ( _id > 0 )
        {
            moduleName += "_" + std::to_string( static_cast< int >( _id ) - 1 );
        }

        pub = _node->Advertise< msgs::RofiCmd >( "~/" + moduleName + "/control" );
        sub = _node->Subscribe( "~/" + moduleName + "/response", &RoFISim::onResponse, this );
        assert( pub );
        assert( sub );

        std::cerr << "Waiting for description from RoFI " << _id << "...\n";

        pub->WaitForConnection();
        initDescription();
        initJoints();

        std::cerr << "Connected to RoFI " << _id << "\n";
        std::cerr << "Number of joints: " << _joints.size() << "\n";
        std::cerr << "Number of connectors: " << _connectors.size() << "\n";
    }

    void initDescription()
    {
        while ( !hasDescription )
        {
            msgs::RofiCmd rofiCmd;
            rofiCmd.set_rofiid( _id );
            rofiCmd.set_cmdtype( msgs::RofiCmd::DESCRIPTION );
            pub->WaitForConnection();
            pub->Publish( std::move( rofiCmd ), true );
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }
    }

    void initJoints();


    RoFI::Id getId() const override
    {
        return _id;
    }

    Joint getJoint( int index ) override;
    Connector getConnector( int index ) override;

    void onJointResp( const RofiRespPtr & resp );
    void onConnectorResp( const RofiRespPtr & resp );
    void onDescriptionResp( const RofiRespPtr & resp );
    void onWaitResp( const RofiRespPtr & resp );
    void onResponse( const RofiRespPtr & resp );

    void wait( int ms, std::function< void() > callback )
    {
        assert( callback );
        assert( ms > 0 );

        {
            std::lock_guard< std::mutex > lock( waitCallbacksMapMutex );
            waitId++;
            assert( waitCallbacksMap.find( waitId ) == waitCallbacksMap.end() );
            waitCallbacksMap.emplace( waitId, std::move( callback ) );
        }

        msgs::RofiCmd rofiCmd;
        rofiCmd.set_rofiid( getId() );
        rofiCmd.set_cmdtype( msgs::RofiCmd::WAIT_CMD );
        rofiCmd.mutable_waitcmd()->set_waitid( waitId );
        rofiCmd.mutable_waitcmd()->set_waitms( ms );
        pub->Publish( std::move( rofiCmd ), true );
    }

private:
    const RoFI::Id _id;
    std::mutex descriptionMutex;
    std::atomic_bool hasDescription = false;

    const std::shared_ptr< GazeboClientHolder > _clientHolder;
    gazebo::transport::NodePtr _node;

public:
    gazebo::transport::PublisherPtr pub;
    gazebo::transport::SubscriberPtr sub;

private:
    std::vector< std::shared_ptr< JointSim > > _joints;
    std::vector< std::shared_ptr< ConnectorSim > > _connectors;

    static int waitId;
    static std::mutex waitCallbacksMapMutex;
    static std::unordered_map< int, std::function< void() > > waitCallbacksMap;
};

/**
 * HAL Connector implementation for Gazebo simulator
 */
class ConnectorSim : public Connector::Implementation
{
public:
    using Callback = std::function< void( Connector, RoFISim::RofiRespPtr ) >;

    ConnectorSim( std::weak_ptr< RoFISim > rofi, int connectorNumber )
            : _connectorNumber( connectorNumber )
            , _rofi( std::move( rofi ) )
    {
    }


    ConnectorState getState() override
    {
        auto rofi = getRoFI();

        auto result = registerPromise( msgs::ConnectorCmd::GET_STATE );
        rofi->pub->Publish( getCmdMsg( msgs::ConnectorCmd::GET_STATE ), true );

        auto resp = result.get();
        assert( resp );
        return readConnectorState( std::move( resp ) );
    }

    void connect() override
    {
        auto rofi = getRoFI();
        rofi->pub->Publish( getCmdMsg( msgs::ConnectorCmd::CONNECT ), true );
    }

    void disconnect() override
    {
        auto rofi = getRoFI();
        rofi->pub->Publish( getCmdMsg( msgs::ConnectorCmd::DISCONNECT ), true );
    }

    void onConnectorEvent( std::function< void( Connector, ConnectorEvent ) > callback ) override
    {
        if ( !callback )
        {
            throw std::invalid_argument( "empty callback" );
        }
        registerCallback( msgs::ConnectorCmd::CONNECTOR_EVENT,
                          [ callback = std::move( callback ) ]( Connector connector,
                                                                RoFISim::RofiRespPtr resp ) {
                              callback( connector, ConnectorSim::readEvent( std::move( resp ) ) );
                          } );
    }

    void onPacket( std::function< void( Connector, Packet ) > callback ) override
    {
        if ( !callback )
        {
            throw std::invalid_argument( "empty callback" );
        }
        registerCallback( msgs::ConnectorCmd::PACKET,
                          [ callback = std::move( callback ) ]( Connector connector,
                                                                RoFISim::RofiRespPtr resp ) {
                              callback( connector, ConnectorSim::readPacket( std::move( resp ) ) );
                          } );
    }

    void send( Packet packet ) override
    {
        static_assert( sizeof( Packet::value_type ) == sizeof( char ) );

        auto rofi = getRoFI();

        auto msg = getCmdMsg( msgs::ConnectorCmd::PACKET );
        msg.mutable_connectorcmd()->mutable_packet()->set_message( packet.data(), packet.size() );
        rofi->pub->Publish( std::move( msg ), true );
    }

    void connectPower( ConnectorLine line ) override
    {
        static_assert( static_cast< msgs::ConnectorCmd::Line >( ConnectorLine::Internal )
                       == msgs::ConnectorCmd::INT_LINE );
        static_assert( static_cast< msgs::ConnectorCmd::Line >( ConnectorLine::External )
                       == msgs::ConnectorCmd::EXT_LINE );

        auto rofi = getRoFI();
        auto msg = getCmdMsg( msgs::ConnectorCmd::CONNECT_POWER );
        msg.mutable_connectorcmd()->set_line( static_cast< msgs::ConnectorCmd::Line >( line ) );
        rofi->pub->Publish( std::move( msg ), true );
    }

    void disconnectPower( ConnectorLine line ) override
    {
        static_assert( static_cast< msgs::ConnectorCmd::Line >( ConnectorLine::Internal )
                       == msgs::ConnectorCmd::INT_LINE );
        static_assert( static_cast< msgs::ConnectorCmd::Line >( ConnectorLine::External )
                       == msgs::ConnectorCmd::EXT_LINE );

        auto rofi = getRoFI();
        auto msg = getCmdMsg( msgs::ConnectorCmd::DISCONNECT_POWER );
        msg.mutable_connectorcmd()->set_line( static_cast< msgs::ConnectorCmd::Line >( line ) );
        rofi->pub->Publish( std::move( msg ), true );
    }


    std::future< RoFISim::RofiRespPtr > registerPromise( msgs::ConnectorCmd::Type type )
    {
        std::lock_guard< std::mutex > lock( respPromisesMutex );
        return respPromises.emplace( type, std::promise< RoFISim::RofiRespPtr >() )
                ->second.get_future();
    }

    void registerCallback( msgs::ConnectorCmd::Type type, Callback && callback )
    {
        assert( callback );

        std::lock_guard< std::mutex > lock( respCallbacksMutex );
        respCallbacks.emplace( type, std::move( callback ) );
    }

    void setPromises( const RoFISim::RofiRespPtr & resp )
    {
        std::lock_guard< std::mutex > lock( respPromisesMutex );
        auto range = respPromises.equal_range( resp->connectorresp().cmdtype() );
        for ( auto it = range.first; it != range.second; it++ )
        {
            it->second.set_value( resp );
        }
        respPromises.erase( range.first, range.second );
    }

    void callCallbacks( const RoFISim::RofiRespPtr & resp )
    {
        std::lock_guard< std::mutex > lock( respCallbacksMutex );
        auto range = respCallbacks.equal_range( resp->connectorresp().cmdtype() );
        for ( auto it = range.first; it != range.second; it++ )
        {
            assert( it->second );
            std::thread( it->second, Connector( shared_from_this() ), resp ).detach();
        }
    }

    void onResponse( const RoFISim::RofiRespPtr & resp )
    {
        assert( resp->resptype() == msgs::RofiCmd::CONNECTOR_CMD );
        assert( resp->connectorresp().connector() == _connectorNumber );

        setPromises( resp );
        callCallbacks( resp );
    }


    msgs::RofiCmd getCmdMsg( msgs::ConnectorCmd::Type type ) const
    {
        msgs::RofiCmd rofiCmd;
        rofiCmd.set_rofiid( getRoFI()->getId() );
        rofiCmd.set_cmdtype( msgs::RofiCmd::CONNECTOR_CMD );
        rofiCmd.mutable_connectorcmd()->set_connector( _connectorNumber );
        rofiCmd.mutable_connectorcmd()->set_cmdtype( type );
        return rofiCmd;
    }

    std::shared_ptr< RoFISim > getRoFI() const
    {
        auto rofi = _rofi.lock();
        if ( !rofi )
        {
            throw std::runtime_error( "RoFI invalid access from connector" );
        }
        return rofi;
    }


    static ConnectorState readConnectorState( RoFISim::RofiRespPtr resp )
    {
        static_assert( static_cast< ConnectorPosition >( true ) == ConnectorPosition::Expanded );
        static_assert( static_cast< ConnectorPosition >( false ) == ConnectorPosition::Retracted );

        static_assert( static_cast< ConnectorOrientation >( msgs::ConnectorState::NORTH )
                       == ConnectorOrientation::North );
        static_assert( static_cast< ConnectorOrientation >( msgs::ConnectorState::EAST )
                       == ConnectorOrientation::East );
        static_assert( static_cast< ConnectorOrientation >( msgs::ConnectorState::SOUTH )
                       == ConnectorOrientation::South );
        static_assert( static_cast< ConnectorOrientation >( msgs::ConnectorState::WEST )
                       == ConnectorOrientation::West );

        assert( resp );

        const auto & msg = resp->connectorresp().state();
        ConnectorState result;
        result.position = static_cast< ConnectorPosition >( msg.position() );
        result.internal = msg.internal();
        result.external = msg.external();
        result.connected = msg.connected();
        result.orientation = static_cast< ConnectorOrientation >( msg.orientation() );
        return result;
    }

    static Packet readPacket( RoFISim::RofiRespPtr resp )
    {
        static_assert( sizeof( Packet::value_type ) == sizeof( char ) );

        const auto & message = resp->connectorresp().packet().message();
        Packet packet;
        packet.reserve( message.size() );
        for ( auto byte : message )
        {
            packet.push_back( static_cast< Packet::value_type >( byte ) );
        }
        return packet;
    }

    static ConnectorEvent readEvent( RoFISim::RofiRespPtr resp )
    {
        // TODO: implement
        return {};
    }

private:
    const int _connectorNumber;
    std::weak_ptr< RoFISim > _rofi;

    std::mutex respPromisesMutex;
    std::multimap< msgs::ConnectorCmd::Type, std::promise< RoFISim::RofiRespPtr > > respPromises;

    std::mutex respCallbacksMutex;
    std::multimap< msgs::ConnectorCmd::Type, Callback > respCallbacks;
};

/**
 * HAL Joint implementation for Gazebo simulator
 */
class JointSim : public Joint::Implementation
{
public:
    using Callback = std::function< void( Joint, RoFISim::RofiRespPtr ) >;
    static constexpr float positionPrecision = 0.01;
    static_assert( positionPrecision > 0 );

    JointSim( std::weak_ptr< RoFISim > rofi, int jointNumber )
            : jointNumber( jointNumber )
            , _rofi( std::move( rofi ) )
    {
    }

    void initCapabilities()
    {
        auto rofi = getRoFI();

        auto result = registerPromise( msgs::JointCmd::GET_CAPABILITIES );
        rofi->pub->Publish( getCmdMsg( msgs::JointCmd::GET_CAPABILITIES ), true );

        auto resp = result.get();
        assert( resp );
        _capabilities = readCapabilities( std::move( resp ) );
    }


    const Capabilities & getCapabilities() override
    {
        return _capabilities;
    }

    float getVelocity() override
    {
        auto rofi = getRoFI();

        auto result = registerPromise( msgs::JointCmd::GET_VELOCITY );
        rofi->pub->Publish( getCmdMsg( msgs::JointCmd::GET_VELOCITY ), true );

        auto resp = result.get();
        assert( resp );
        return resp->jointresp().value();
    }

    void setVelocity( float velocity ) override
    {
        auto rofi = getRoFI();

        auto msg = getCmdMsg( msgs::JointCmd::SET_VELOCITY );
        msg.mutable_jointcmd()->mutable_setvelocity()->set_velocity( velocity );
        rofi->pub->Publish( std::move( msg ), true );
    }

    float getPosition() override
    {
        auto rofi = getRoFI();

        auto result = registerPromise( msgs::JointCmd::GET_POSITION );
        rofi->pub->Publish( getCmdMsg( msgs::JointCmd::GET_POSITION ), true );

        auto resp = result.get();
        assert( resp );
        return resp->jointresp().value();
    }

    void setPosition( float pos, float velocity, std::function< void( Joint ) > callback ) override
    {
        auto rofi = getRoFI();

        registerPositionCallback( pos, std::move( callback ) );

        auto msg = getCmdMsg( msgs::JointCmd::SET_POS_WITH_SPEED );
        msg.mutable_jointcmd()->mutable_setposwithspeed()->set_position( pos );
        msg.mutable_jointcmd()->mutable_setposwithspeed()->set_speed( velocity );
        rofi->pub->Publish( std::move( msg ), true );
    }

    float getTorque() override
    {
        auto rofi = getRoFI();

        auto result = registerPromise( msgs::JointCmd::GET_TORQUE );
        rofi->pub->Publish( getCmdMsg( msgs::JointCmd::GET_TORQUE ), true );

        auto resp = result.get();
        assert( resp );
        return resp->jointresp().value();
    }

    void setTorque( float torque ) override
    {
        auto rofi = getRoFI();

        auto msg = getCmdMsg( msgs::JointCmd::SET_TORQUE );
        msg.mutable_jointcmd()->mutable_settorque()->set_torque( torque );
        rofi->pub->Publish( std::move( msg ), true );
    }

    void onError( std::function< void( Joint, Joint::Error, const std::string & msg ) > callback )
            override
    {
        if ( !callback )
        {
            throw std::invalid_argument( "empty callback" );
        }
        registerCallback(
                msgs::JointCmd::ERROR,
                [ callback = std::move( callback ) ]( Joint joint, RoFISim::RofiRespPtr resp ) {
                    auto [ error, msg ] = readError( std::move( resp ) );
                    callback( joint, error, std::move( msg ) );
                } );
    }


    std::future< RoFISim::RofiRespPtr > registerPromise( msgs::JointCmd::Type type )
    {
        std::lock_guard< std::mutex > lock( respPromisesMutex );
        return respPromises.emplace( type, std::promise< RoFISim::RofiRespPtr >() )
                ->second.get_future();
    }

    void registerCallback( msgs::JointCmd::Type type, Callback && callback )
    {
        assert( callback );

        std::lock_guard< std::mutex > lock( respCallbacksMutex );
        respCallbacks.emplace( type, std::move( callback ) );
    }

    void registerPositionCallback( float position, std::function< void( Joint ) > && callback )
    {
        std::function< void( Joint ) > oldCallback;
        {
            std::lock_guard< std::mutex > lock( positionReachedCallbackMutex );
            oldCallback = std::move( positionReachedCallback.second );
            positionReachedCallback = { position, std::move( callback ) };
        }
        if ( oldCallback )
        {
            std::cerr << "Aborting old callback\n";
            // TODO abort oldCallback
        }
    }

    void setPromises( const RoFISim::RofiRespPtr & resp )
    {
        std::lock_guard< std::mutex > lock( respPromisesMutex );
        auto range = respPromises.equal_range( resp->jointresp().cmdtype() );
        for ( auto it = range.first; it != range.second; it++ )
        {
            it->second.set_value( resp );
        }
        respPromises.erase( range.first, range.second );
    }

    void callCallbacks( const RoFISim::RofiRespPtr & resp )
    {
        std::lock_guard< std::mutex > lock( respCallbacksMutex );
        auto range = respCallbacks.equal_range( resp->jointresp().cmdtype() );
        for ( auto it = range.first; it != range.second; it++ )
        {
            assert( it->second );
            std::thread( it->second, Joint( shared_from_this() ), resp ).detach();
        }
    }

    void checkPositionReachedCallback( float positionReached )
    {
        std::lock_guard< std::mutex > lock( positionReachedCallbackMutex );
        auto & [ position, callback ] = positionReachedCallback;
        if ( callback && std::abs( positionReached - position ) <= positionPrecision )
        {
            std::thread( std::move( callback ), Joint( shared_from_this() ) ).detach();
            positionReachedCallback = {};
        }
    }

    void onResponse( const RoFISim::RofiRespPtr & resp )
    {
        assert( resp->resptype() == msgs::RofiCmd::JOINT_CMD );
        assert( resp->jointresp().joint() == jointNumber );

        setPromises( resp );
        callCallbacks( resp );

        if ( resp->jointresp().cmdtype() == msgs::JointCmd::SET_POS_WITH_SPEED )
        {
            checkPositionReachedCallback( resp->jointresp().value() );
        }
    }


    msgs::RofiCmd getCmdMsg( msgs::JointCmd::Type type ) const
    {
        msgs::RofiCmd rofiCmd;
        rofiCmd.set_rofiid( getRoFI()->getId() );
        rofiCmd.set_cmdtype( msgs::RofiCmd::JOINT_CMD );
        rofiCmd.mutable_jointcmd()->set_joint( jointNumber );
        rofiCmd.mutable_jointcmd()->set_cmdtype( type );
        return rofiCmd;
    }

    std::shared_ptr< RoFISim > getRoFI() const
    {
        auto rofi = _rofi.lock();
        if ( !rofi )
        {
            // TODO call error callback
            throw std::runtime_error( "RoFI invalid access from joint" );
        }
        return rofi;
    }


    static Capabilities readCapabilities( RoFISim::RofiRespPtr resp )
    {
        assert( resp );

        const auto & msg = resp->jointresp().capabilities();
        Capabilities result;
        result.maxPosition = msg.maxposition();
        result.minPosition = msg.minposition();
        result.maxSpeed = msg.maxspeed();
        result.minSpeed = msg.minspeed();
        result.maxTorque = msg.maxtorque();
        return result;
    }

    static std::pair< Joint::Error, std::string > readError( RoFISim::RofiRespPtr /* resp */ )
    {
        // TODO: implement
        return {};
    }

private:
    const int jointNumber;
    std::weak_ptr< RoFISim > _rofi;
    Capabilities _capabilities;

    std::mutex respPromisesMutex;
    std::multimap< msgs::JointCmd::Type, std::promise< RoFISim::RofiRespPtr > > respPromises;

    std::mutex respCallbacksMutex;
    std::multimap< msgs::JointCmd::Type, Callback > respCallbacks;

    std::mutex positionReachedCallbackMutex;
    std::pair< float, std::function< void( Joint ) > > positionReachedCallback;
};


int RoFISim::waitId = {};
std::mutex RoFISim::waitCallbacksMapMutex = {};
std::unordered_map< int, std::function< void() > > RoFISim::waitCallbacksMap = {};

void RoFISim::initJoints()
{
    assert( hasDescription );

    for ( auto & joint : _joints )
    {
        joint->initCapabilities();
    }
}


Joint RoFISim::getJoint( int index )
{
    assert( hasDescription );

    auto jointSim = std::dynamic_pointer_cast< Joint::Implementation >( _joints.at( index ) );
    assert( jointSim );
    return Joint( jointSim );
}

Connector RoFISim::getConnector( int index )
{
    assert( hasDescription );

    auto connectorSim =
            std::dynamic_pointer_cast< Connector::Implementation >( _connectors.at( index ) );
    assert( connectorSim );
    return Connector( connectorSim );
}

void RoFISim::onJointResp( const RoFISim::RofiRespPtr & resp )
{
    assert( resp->resptype() == msgs::RofiCmd::JOINT_CMD );

    int index = resp->jointresp().joint();
    if ( index < 0 || static_cast< size_t >( index ) >= _joints.size() )
    {
        std::cerr << "Joint index of response is out of range. Ignoring...\n";
        return;
    }
    _joints.at( index )->onResponse( resp );
}

void RoFISim::onConnectorResp( const RoFISim::RofiRespPtr & resp )
{
    assert( resp->resptype() == msgs::RofiCmd::CONNECTOR_CMD );

    int index = resp->connectorresp().connector();
    if ( index < 0 || static_cast< size_t >( index ) >= _connectors.size() )
    {
        std::cerr << "Connector index of response is out of range. Ignoring...\n";
        return;
    }
    _connectors.at( index )->onResponse( resp );
}

void RoFISim::onDescriptionResp( const RoFISim::RofiRespPtr & resp )
{
    assert( resp->resptype() == msgs::RofiCmd::DESCRIPTION );

    std::lock_guard< std::mutex > lock( descriptionMutex );
    if ( hasDescription )
    {
        std::cerr << "Already have RoFI description. Ignoring ...\n";
        return;
    }

    auto & description = resp->rofidescription();

    if ( description.jointcount() < 0 )
    {
        std::cerr << "RoFI description has negative joint count. Ignoring ...\n";
        return;
    }
    if ( description.connectorcount() < 0 )
    {
        std::cerr << "RoFI description has negative connector count. Ignoring ...\n";
        return;
    }

    size_t jointCount = static_cast< size_t >( description.jointcount() );
    while ( static_cast< long >( _joints.size() ) < jointCount )
    {
        _joints.push_back( std::make_shared< JointSim >( shared_from_this(), _joints.size() ) );
    }

    size_t connectorCount = static_cast< size_t >( description.connectorcount() );
    while ( static_cast< long >( _connectors.size() ) < connectorCount )
    {
        _connectors.push_back(
                std::make_shared< ConnectorSim >( shared_from_this(), _connectors.size() ) );
    }

    hasDescription = true;
}

void RoFISim::onWaitResp( const RoFISim::RofiRespPtr & resp )
{
    assert( resp->resptype() == msgs::RofiCmd::WAIT_CMD );

    std::function< void() > callback;

    {
        std::lock_guard< std::mutex > lock( waitCallbacksMapMutex );
        auto it = waitCallbacksMap.find( resp->waitid() );
        if ( it == waitCallbacksMap.end() )
        {
            std::cerr << "Got wait response without a callback waiting (ID: " << resp->waitid()
                      << "). Ignoring...\n";
            return;
        }
        callback = std::move( it->second );
        waitCallbacksMap.erase( it );
    }

    assert( callback );
    callback();
}

void RoFISim::onResponse( const RoFISim::RofiRespPtr & resp )
{
    switch ( resp->resptype() )
    {
        case msgs::RofiCmd::JOINT_CMD:
        {
            onJointResp( resp );
            break;
        }
        case msgs::RofiCmd::CONNECTOR_CMD:
        {
            onConnectorResp( resp );
            break;
        }
        case msgs::RofiCmd::DESCRIPTION:
        {
            onDescriptionResp( resp );
            break;
        }
        case msgs::RofiCmd::WAIT_CMD:
        {
            onWaitResp( resp );
            break;
        }
        default:
        {
            std::cerr << "Not recognized rofi cmd type. Ignoring...\n";
            break;
        }
    }
}

} // namespace

namespace rofi::hal
{
RoFI RoFI::getLocalRoFI()
{
    static auto localRoFI = RoFISim::create();
    return RoFI( localRoFI );
}

RoFI RoFI::getRemoteRoFI( RoFI::Id id )
{
    static std::mutex remotesMutex;
    std::lock_guard< std::mutex > lock( remotesMutex );
    static std::map< Id, std::weak_ptr< RoFISim > > remotes;

    std::shared_ptr< RoFISim > remoteRoFI;

    auto it = remotes.find( id );
    if ( it != remotes.end() )
    {
        remoteRoFI = it->second.lock();
    }

    if ( !remoteRoFI )
    {
        remoteRoFI = RoFISim::create( id );
        remotes.insert_or_assign( id, remoteRoFI );
    }

    assert( remoteRoFI );
    assert( remotes.find( id ) != remotes.end() );
    assert( !remotes.find( id )->second.expired() );
    return RoFI( remoteRoFI );
}

void RoFI::wait( int ms, std::function< void() > callback )
{
    if ( !callback )
    {
        throw std::invalid_argument( "empty callback" );
    }

    if ( ms <= 0 )
    {
        throw std::invalid_argument( "non-positive wait time" );
    }

    auto localRoFI = std::dynamic_pointer_cast< RoFISim >( RoFI::getLocalRoFI()._impl );
    assert( localRoFI );
    localRoFI->wait( ms, std::move( callback ) );
}

} // namespace rofi::hal
