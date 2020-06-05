#include "rofi_hal.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

#include <boost/uuid/random_generator.hpp>
#include <gazebo/gazebo_client.hh>
#include <gazebo/transport/transport.hh>

#include "subscriber_wrapper.hpp"

#include <distributorReq.pb.h>
#include <distributorResp.pb.h>
#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>

#ifndef SESSION_ID
#define SESSION_ID boost::uuids::random_generator()()
#endif


namespace
{
using namespace rofi::hal;
namespace msgs = rofi::messages;

class SessionId
{
    template < typename T, std::enable_if_t< std::is_integral_v< T >, int > = 0 >
    constexpr SessionId( T id )
    {
        _bytes.reserve( sizeof( T ) );

        for ( size_t i = 0; i < sizeof( T ); i++ )
        {
            _bytes.push_back( *( reinterpret_cast< char * >( &id ) + i ) );
        }
    }

    template < typename T,
               std::enable_if_t< sizeof( typename T::value_type ) == sizeof( char ), int > = 0 >
    constexpr SessionId( T id )
    {
        _bytes.reserve( id.size() );

        for ( auto & c : id )
        {
            static_assert( sizeof( decltype( c ) ) == sizeof( char ) );
            _bytes.push_back( reinterpret_cast< char & >( c ) );
        }
    }

    SessionId( const SessionId & other ) = delete;
    SessionId & operator=( const SessionId & other ) = delete;

public:
    static const SessionId & get()
    {
        static SessionId instance = SessionId( SESSION_ID );
        return instance;
    }

    const std::string & bytes() const
    {
        return _bytes;
    }

private:
    std::string _bytes;
};

class GazeboClientHolder
{
    GazeboClientHolder()
    {
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
    using DistributorRespPtr = boost::shared_ptr< const msgs::DistributorResp >;


    static void distributorRespGetInfoCb( std::once_flag & onceFlag,
                                          DistributorRespPtr resp,
                                          std::promise< DistributorRespPtr > & respPromise )
    {
        if ( resp->resptype() != msgs::DistributorReq::GET_INFO )
        {
            return;
        }
        std::call_once( onceFlag, [ & ] { respPromise.set_value( resp ); } );
    }

    static void distributorRespLockOneCb( std::once_flag & onceFlag,
                                          DistributorRespPtr resp,
                                          std::promise< msgs::RofiInfo > & infoPromise )
    {
        if ( resp->resptype() != msgs::DistributorReq::LOCK_ONE )
        {
            return;
        }
        if ( resp->sessionid() != SessionId::get().bytes() )
        {
            return;
        }
        std::call_once( onceFlag, [ & ] {
            if ( resp->rofiinfos_size() != 1 || !resp->rofiinfos( 0 ).lock() )
            {
                infoPromise.set_exception(
                        std::make_exception_ptr( std::runtime_error( "Could not lock a RoFI" ) ) );
                return;
            }
            infoPromise.set_value( resp->rofiinfos( 0 ) );
        } );
    }

    static void distributorRespTryLockCb( std::once_flag & onceFlag,
                                          DistributorRespPtr resp,
                                          std::promise< msgs::RofiInfo > & infoPromise,
                                          RoFI::Id rofiId )
    {
        if ( resp->resptype() != msgs::DistributorReq::TRY_LOCK )
        {
            return;
        }
        if ( resp->sessionid() != SessionId::get().bytes() )
        {
            return;
        }
        if ( resp->rofiinfos_size() != 1 || resp->rofiinfos( 0 ).rofiid() != rofiId )
        {
            return;
        }
        std::call_once( onceFlag, [ & ] {
            if ( !resp->rofiinfos( 0 ).lock() )
            {
                infoPromise.set_exception( std::make_exception_ptr(
                        std::runtime_error( "Could not lock selected RoFI" ) ) );
                return;
            }
            infoPromise.set_value( resp->rofiinfos( 0 ) );
        } );
    }

    static msgs::RofiInfo getNewLocalInfo()
    {
        auto node = boost::make_shared< gazebo::transport::Node >();
        node->Init();

        auto infoPromise = std::promise< msgs::RofiInfo >();
        auto infoFuture = infoPromise.get_future();
        std::once_flag onceFlag;

        auto sub = SubscriberWrapper< msgs::DistributorResp >(
                node,
                "~/distributor/response",
                [ & ]( auto resp ) { distributorRespLockOneCb( onceFlag, resp, infoPromise ); } );

        msgs::DistributorReq req;
        req.set_sessionid( SessionId::get().bytes() );
        req.set_reqtype( msgs::DistributorReq::LOCK_ONE );
        node->Publish< msgs::DistributorReq >( "~/distributor/request", req );

        return infoFuture.get();
    }

    static msgs::RofiInfo tryLockLocal( RoFI::Id rofiId )
    {
        auto node = boost::make_shared< gazebo::transport::Node >();
        node->Init();

        auto infoPromise = std::promise< msgs::RofiInfo >();
        auto infoFuture = infoPromise.get_future();
        std::once_flag onceFlag;

        auto sub = SubscriberWrapper< msgs::DistributorResp >(
                node,
                "~/distributor/response",
                [ & ]( auto resp ) {
                    distributorRespTryLockCb( onceFlag, resp, infoPromise, rofiId );
                } );

        msgs::DistributorReq req;
        req.set_sessionid( SessionId::get().bytes() );
        req.set_reqtype( msgs::DistributorReq::TRY_LOCK );
        req.set_rofiid( rofiId );
        node->Publish< msgs::DistributorReq >( "~/distributor/request", req );

        return infoFuture.get();
    }

    static std::string getTopic( RoFI::Id rofiId )
    {
        auto node = boost::make_shared< gazebo::transport::Node >();
        node->Init();

        auto respPromise = std::promise< DistributorRespPtr >();
        auto respFuture = respPromise.get_future();
        std::once_flag onceFlag;

        auto sub = SubscriberWrapper< msgs::DistributorResp >(
                node,
                "~/distributor/response",
                [ & ]( auto resp ) { distributorRespGetInfoCb( onceFlag, resp, respPromise ); } );

        msgs::DistributorReq req;
        req.set_sessionid( SessionId::get().bytes() );
        req.set_reqtype( msgs::DistributorReq::GET_INFO );
        node->Publish< msgs::DistributorReq >( "~/distributor/request", req );

        auto resp = respFuture.get();

        for ( auto & info : resp->rofiinfos() )
        {
            if ( info.rofiid() == rofiId )
            {
                return info.topic();
            }
        }

        std::cerr << "No RoFI with ID: " << rofiId << "\n";
        throw std::runtime_error( "No RoFI with given ID" );
    }

    static std::shared_ptr< RoFISim > createLocal()
    {
        auto gazeboHolder = GazeboClientHolder::get();
#ifdef LOCAL_ROFI_ID
        auto info = tryLockLocal( LOCAL_ROFI_ID );
#else
        auto info = getNewLocalInfo();
#endif
        auto newRoFI = std::shared_ptr< RoFISim >( new RoFISim( info.rofiid() ) );
        newRoFI->init( info.topic() );
        return newRoFI;
    }

    static std::shared_ptr< RoFISim > createRemote( RoFI::Id id )
    {
        auto gazeboHolder = GazeboClientHolder::get();
        auto newRoFI = std::shared_ptr< RoFISim >( new RoFISim( id ) );
        newRoFI->init( getTopic( id ) );
        return newRoFI;
    }

    void init( std::string topic )
    {
        _node = boost::make_shared< gazebo::transport::Node >();
        _node->Init( topic );

        _pub = _node->Advertise< msgs::RofiCmd >( "~/control" );
        _sub = _node->Subscribe( "~/response", &RoFISim::onResponse, this );
        assert( _pub );
        assert( _sub );
        _pub->WaitForConnection();

        std::cerr << "Waiting for description from RoFI " << _id << "...\n";

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
            publish( rofiCmd );
            std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
        }
    }

    void publish( const msgs::RofiCmd & msg )
    {
        assert( _pub );
        _pub->Publish( msg, true );
    }

    void initJoints();


    RoFI::Id getId() const override
    {
        return _id;
    }

    Joint getJoint( int index ) override;
    Connector getConnector( int index ) override;
    RoFI::Descriptor getDescriptor() const override
    {
        RoFI::Descriptor descriptor;
        descriptor.jointCount = _joints.size();
        descriptor.connectorCount = _connectors.size();
        return descriptor;
    }

    void onJointResp( const RofiRespPtr & resp );
    void onConnectorResp( const RofiRespPtr & resp );
    void onDescriptionResp( const RofiRespPtr & resp );
    void onWaitResp( const RofiRespPtr & resp );
    void onResponse( const RofiRespPtr & resp );

    void wait( int ms, std::function< void() > callback )
    {
        assert( callback );
        assert( ms > 0 );

        int tmpWaitId = 0;

        {
            std::lock_guard< std::mutex > lock( waitCallbacksMapMutex );
            tmpWaitId = ++waitId;
            assert( waitCallbacksMap.find( tmpWaitId ) == waitCallbacksMap.end() );
            waitCallbacksMap.emplace( tmpWaitId, std::move( callback ) );
        }

        msgs::RofiCmd rofiCmd;
        rofiCmd.set_rofiid( getId() );
        rofiCmd.set_cmdtype( msgs::RofiCmd::WAIT_CMD );
        rofiCmd.mutable_waitcmd()->set_waitid( tmpWaitId );
        rofiCmd.mutable_waitcmd()->set_waitms( ms );
        publish( rofiCmd );
    }

private:
    const RoFI::Id _id;
    std::mutex descriptionMutex;
    std::atomic_bool hasDescription = false;

    const std::shared_ptr< GazeboClientHolder > _clientHolder;
    gazebo::transport::NodePtr _node;
    gazebo::transport::SubscriberPtr _sub;
    gazebo::transport::PublisherPtr _pub;

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
    using EventCallback = std::function< void( Connector, ConnectorEvent ) >;
    using PacketCallback = std::function< void( Connector, Packet ) >;

    ConnectorSim( std::weak_ptr< RoFISim > rofi, int connectorNumber )
            : _connectorNumber( connectorNumber )
            , _rofi( std::move( rofi ) )
    {
    }


    ConnectorState getState() override
    {
        auto rofi = getRoFI();

        auto result = registerPromise( msgs::ConnectorCmd::GET_STATE );
        rofi->publish( getCmdMsg( msgs::ConnectorCmd::GET_STATE ) );

        auto resp = result.get();
        assert( resp );
        return readConnectorState( std::move( resp ) );
    }

    void connect() override
    {
        auto rofi = getRoFI();
        rofi->publish( getCmdMsg( msgs::ConnectorCmd::CONNECT ) );
    }

    void disconnect() override
    {
        auto rofi = getRoFI();
        rofi->publish( getCmdMsg( msgs::ConnectorCmd::DISCONNECT ) );
    }

    void onConnectorEvent( EventCallback callback ) override
    {
        if ( !callback )
        {
            throw std::invalid_argument( "empty callback" );
        }
        registerEventCallback( std::move( callback ) );
    }

    void onPacket( PacketCallback callback ) override
    {
        if ( !callback )
        {
            throw std::invalid_argument( "empty callback" );
        }
        registerPacketCallback( std::move( callback ) );
    }

    void send( Packet packet ) override
    {
        static_assert( sizeof( Packet::value_type ) == sizeof( char ) );

        auto rofi = getRoFI();

        auto msg = getCmdMsg( msgs::ConnectorCmd::PACKET );
        msg.mutable_connectorcmd()->mutable_packet()->set_message( packet.data(), packet.size() );
        rofi->publish( msg );
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
        rofi->publish( msg );
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
        rofi->publish( msg );
    }


    std::future< RoFISim::RofiRespPtr > registerPromise( msgs::ConnectorCmd::Type type )
    {
        std::lock_guard< std::mutex > lock( respPromisesMutex );
        return respPromises.emplace( type, std::promise< RoFISim::RofiRespPtr >() )
                ->second.get_future();
    }

    void registerEventCallback( EventCallback && callback )
    {
        assert( callback );

        std::lock_guard< std::mutex > lock( eventCallbacksMutex );
        eventCallbacks.push_back( std::move( callback ) );
    }

    void registerPacketCallback( PacketCallback && callback )
    {
        assert( callback );

        std::lock_guard< std::mutex > lock( packetCallbacksMutex );
        packetCallbacks.push_back( std::move( callback ) );
    }

    void setPromises( const RoFISim::RofiRespPtr & resp )
    {
        std::lock_guard< std::mutex > lock( respPromisesMutex );
        auto range = respPromises.equal_range( resp->connectorresp().resptype() );
        for ( auto it = range.first; it != range.second; it++ )
        {
            it->second.set_value( resp );
        }
        respPromises.erase( range.first, range.second );
    }

    void callEventCallbacks( ConnectorEvent event )
    {
        std::lock_guard< std::mutex > lock( eventCallbacksMutex );
        for ( const auto & callback : eventCallbacks )
        {
            assert( callback );
            std::thread( callback, Connector( shared_from_this() ), event ).detach();
        }
    }

    void callPacketCallbacks( const Packet & packet )
    {
        std::lock_guard< std::mutex > lock( packetCallbacksMutex );
        for ( const auto & callback : packetCallbacks )
        {
            assert( callback );
            std::thread( callback, Connector( shared_from_this() ), packet ).detach();
        }
    }

    void onResponse( const RoFISim::RofiRespPtr & resp )
    {
        assert( resp->resptype() == msgs::RofiCmd::CONNECTOR_CMD );
        assert( resp->connectorresp().connector() == _connectorNumber );

        setPromises( resp );

        auto event = ConnectorSim::readEvent( resp );
        if ( event )
        {
            callEventCallbacks( *event );
        }

        auto packet = ConnectorSim::readPacket( resp );
        if ( packet )
        {
            callPacketCallbacks( *packet );
        }
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
        static_assert( static_cast< ConnectorPosition >( true ) == ConnectorPosition::Extended );
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

    static std::optional< Packet > readPacket( RoFISim::RofiRespPtr resp )
    {
        static_assert( sizeof( Packet::value_type ) == sizeof( char ) );

        if ( resp->connectorresp().resptype() != msgs::ConnectorCmd::PACKET )
        {
            return {};
        }

        const auto & message = resp->connectorresp().packet().message();
        Packet packet;
        packet.reserve( message.size() );
        for ( auto byte : message )
        {
            packet.push_back( static_cast< Packet::value_type >( byte ) );
        }
        return packet;
    }

    static std::optional< ConnectorEvent > readEvent( RoFISim::RofiRespPtr resp )
    {
        switch ( resp->connectorresp().resptype() )
        {
            case msgs::ConnectorCmd::CONNECT:
                return ConnectorEvent::Connected;
            case msgs::ConnectorCmd::DISCONNECT:
                return ConnectorEvent::Disconnected;
            default:
                return {};
        }
    }

private:
    const int _connectorNumber;
    std::weak_ptr< RoFISim > _rofi;

    std::mutex respPromisesMutex;
    std::multimap< msgs::ConnectorCmd::Type, std::promise< RoFISim::RofiRespPtr > > respPromises;

    std::mutex eventCallbacksMutex;
    std::vector< EventCallback > eventCallbacks;
    std::mutex packetCallbacksMutex;
    std::vector< PacketCallback > packetCallbacks;
};

/**
 * HAL Joint implementation for Gazebo simulator
 */
class JointSim : public Joint::Implementation
{
public:
    using Callback = std::function< void( Joint, RoFISim::RofiRespPtr ) >;
    static constexpr float positionPrecision = 1e-3;
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
        rofi->publish( getCmdMsg( msgs::JointCmd::GET_CAPABILITIES ) );

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
        rofi->publish( getCmdMsg( msgs::JointCmd::GET_VELOCITY ) );

        auto resp = result.get();
        assert( resp );
        return resp->jointresp().value();
    }

    void setVelocity( float velocity ) override
    {
        auto rofi = getRoFI();

        auto msg = getCmdMsg( msgs::JointCmd::SET_VELOCITY );
        msg.mutable_jointcmd()->mutable_setvelocity()->set_velocity( velocity );
        rofi->publish( msg );
    }

    float getPosition() override
    {
        auto rofi = getRoFI();

        auto result = registerPromise( msgs::JointCmd::GET_POSITION );
        rofi->publish( getCmdMsg( msgs::JointCmd::GET_POSITION ) );

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
        rofi->publish( msg );
    }

    float getTorque() override
    {
        auto rofi = getRoFI();

        auto result = registerPromise( msgs::JointCmd::GET_TORQUE );
        rofi->publish( getCmdMsg( msgs::JointCmd::GET_TORQUE ) );

        auto resp = result.get();
        assert( resp );
        return resp->jointresp().value();
    }

    void setTorque( float torque ) override
    {
        auto rofi = getRoFI();

        auto msg = getCmdMsg( msgs::JointCmd::SET_TORQUE );
        msg.mutable_jointcmd()->mutable_settorque()->set_torque( torque );
        rofi->publish( msg );
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
        auto range = respPromises.equal_range( resp->jointresp().resptype() );
        for ( auto it = range.first; it != range.second; it++ )
        {
            it->second.set_value( resp );
        }
        respPromises.erase( range.first, range.second );
    }

    void callCallbacks( const RoFISim::RofiRespPtr & resp )
    {
        std::lock_guard< std::mutex > lock( respCallbacksMutex );
        auto range = respCallbacks.equal_range( resp->jointresp().resptype() );
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

        if ( resp->jointresp().resptype() == msgs::JointCmd::SET_POS_WITH_SPEED )
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
    while ( _joints.size() < jointCount )
    {
        _joints.push_back( std::make_shared< JointSim >( shared_from_this(), _joints.size() ) );
    }

    size_t connectorCount = static_cast< size_t >( description.connectorcount() );
    while ( _connectors.size() < connectorCount )
    {
        _connectors.push_back(
                std::make_shared< ConnectorSim >( shared_from_this(), _connectors.size() ) );
    }

    hasDescription = true;
}

void RoFISim::onWaitResp( const RoFISim::RofiRespPtr & resp )
{
    assert( resp->resptype() == msgs::RofiCmd::WAIT_CMD );

    std::lock_guard< std::mutex > lock( waitCallbacksMapMutex );

    auto it = waitCallbacksMap.find( resp->waitid() );
    if ( it == waitCallbacksMap.end() )
    {
        std::cerr << "Got wait response without a callback waiting (ID: " << resp->waitid()
                  << "). Ignoring...\n";
        return;
    }

    assert( it->second );
    std::thread( it->second ).detach();

    waitCallbacksMap.erase( it );
}

void RoFISim::onResponse( const RoFISim::RofiRespPtr & resp )
{
    switch ( resp->resptype() )
    {
        case msgs::RofiCmd::NO_CMD:
            break;
        case msgs::RofiCmd::JOINT_CMD:
            onJointResp( resp );
            break;
        case msgs::RofiCmd::CONNECTOR_CMD:
            onConnectorResp( resp );
            break;
        case msgs::RofiCmd::DESCRIPTION:
            onDescriptionResp( resp );
            break;
        case msgs::RofiCmd::WAIT_CMD:
            onWaitResp( resp );
            break;
        default:
            std::cerr << "Not recognized rofi cmd type. Ignoring...\n";
            break;
    }
}

} // namespace

namespace rofi::hal
{
RoFI RoFI::getLocalRoFI()
{
    static auto localRoFI = RoFISim::createLocal();
    return RoFI( localRoFI );
}

RoFI RoFI::getRemoteRoFI( RoFI::Id id )
{
    static std::mutex remotesMutex;
    std::lock_guard< std::mutex > lock( remotesMutex );
    static std::map< Id, std::weak_ptr< RoFISim > > remotes;

    auto remoteRoFI = remotes[ id ].lock();

    if ( !remoteRoFI )
    {
        remoteRoFI = RoFISim::createRemote( id );
        remotes[ id ] = remoteRoFI;
    }

    assert( remoteRoFI );
    assert( remotes[ id ].lock() != nullptr );
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
