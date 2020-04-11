#include "rofi_hal_sim.hpp"

#include <cassert>
#include <chrono>
#include <thread>

#include <gazebo/gazebo_client.hh>


namespace rofi
{
namespace hal
{
RoFI::RoFI() : RoFI( 0 )
{
}
RoFI::RoFI( Id remoteId ) : rofiData( std::make_unique< detail::RoFIData >( remoteId ) )
{
}

RoFI::~RoFI() = default;

RoFI & RoFI::getLocalRoFI()
{
    static RoFI localRoFI;
    return localRoFI;
}

RoFI & RoFI::getRemoteRoFI( Id remoteId )
{
    if ( remoteId == 0 )
    {
        return getLocalRoFI();
    }

    static std::map< Id, RoFI > remotes;
    return remotes.emplace( remoteId, RoFI( remoteId ) ).first->second;
}

RoFI::Id RoFI::getId() const
{
    assert( rofiData );
    return rofiData->getId();
}

Joint RoFI::getJoint( int index )
{
    assert( rofiData );
    return rofiData->getJoint( index );
}

Connector RoFI::getConnector( int index )
{
    assert( rofiData );
    return rofiData->getConnector( index );
}

void RoFI::wait( int ms, std::function< void() > callback )
{
    auto & localRoFI = getLocalRoFI();
    assert( localRoFI.rofiData );
    localRoFI.rofiData->wait( ms, std::move( callback ) );
}


Joint::Joint( detail::JointData & jdata ) : jointData( &jdata )
{
}

float Joint::maxPosition() const
{
    auto result = jointData->registerPromise( messages::JointCmd::GET_MAX_POSITION );
    auto msg = jointData->getCmdMsg( messages::JointCmd::GET_MAX_POSITION );
    jointData->rofi.pub->Publish( std::move( msg ), true );

    auto resp = result.get();
    if ( resp->jointresp().values_size() != 1 )
        throw std::runtime_error( "Unexpected size of values from response" );
    return resp->jointresp().values().Get( 0 );
}

float Joint::minPosition() const
{
    auto result = jointData->registerPromise( messages::JointCmd::GET_MIN_POSITION );
    auto msg = jointData->getCmdMsg( messages::JointCmd::GET_MIN_POSITION );
    jointData->rofi.pub->Publish( std::move( msg ), true );

    auto resp = result.get();
    if ( resp->jointresp().values_size() != 1 )
        throw std::runtime_error( "Unexpected size of values from response" );
    return resp->jointresp().values().Get( 0 );
}

float Joint::maxSpeed() const
{
    auto result = jointData->registerPromise( messages::JointCmd::GET_MAX_SPEED );
    auto msg = jointData->getCmdMsg( messages::JointCmd::GET_MAX_SPEED );
    jointData->rofi.pub->Publish( std::move( msg ), true );

    auto resp = result.get();
    if ( resp->jointresp().values_size() != 1 )
        throw std::runtime_error( "Unexpected size of values from response" );
    return resp->jointresp().values().Get( 0 );
}

float Joint::minSpeed() const
{
    auto result = jointData->registerPromise( messages::JointCmd::GET_MIN_SPEED );
    auto msg = jointData->getCmdMsg( messages::JointCmd::GET_MIN_SPEED );
    jointData->rofi.pub->Publish( std::move( msg ), true );

    auto resp = result.get();
    if ( resp->jointresp().values_size() != 1 )
        throw std::runtime_error( "Unexpected size of values from response" );
    return resp->jointresp().values().Get( 0 );
}

float Joint::maxTorque() const
{
    auto result = jointData->registerPromise( messages::JointCmd::GET_MAX_TORQUE );
    auto msg = jointData->getCmdMsg( messages::JointCmd::GET_MAX_TORQUE );
    jointData->rofi.pub->Publish( std::move( msg ), true );

    auto resp = result.get();
    if ( resp->jointresp().values_size() != 1 )
        throw std::runtime_error( "Unexpected size of values from response" );
    return resp->jointresp().values().Get( 0 );
}

float Joint::getVelocity() const
{
    auto result = jointData->registerPromise( messages::JointCmd::GET_VELOCITY );
    auto msg = jointData->getCmdMsg( messages::JointCmd::GET_VELOCITY );
    jointData->rofi.pub->Publish( std::move( msg ), true );

    auto resp = result.get();
    if ( resp->jointresp().values_size() != 1 )
        throw std::runtime_error( "Unexpected size of values from response" );
    return resp->jointresp().values().Get( 0 );
}

void Joint::setVelocity( float velocity )
{
    auto msg = jointData->getCmdMsg( messages::JointCmd::SET_VELOCITY );
    msg.mutable_jointcmd()->mutable_setvelocity()->set_velocity( velocity );
    jointData->rofi.pub->Publish( std::move( msg ), true );
}

float Joint::getPosition() const
{
    auto result = jointData->registerPromise( messages::JointCmd::GET_POSITION );
    auto msg = jointData->getCmdMsg( messages::JointCmd::GET_POSITION );
    jointData->rofi.pub->Publish( std::move( msg ), true );

    auto resp = result.get();
    if ( resp->jointresp().values_size() != 1 )
        throw std::runtime_error( "Unexpected size of values from response" );
    return resp->jointresp().values().Get( 0 );
}

void Joint::setPosition( float pos, float speed, std::function< void( Joint ) > callback )
{
    if ( callback )
    {
        jointData->registerCallback(
                [ pos ]( const messages::JointResp & resp ) {
                    if ( resp.cmdtype() != messages::JointCmd::SET_POS_WITH_SPEED )
                        return false;
                    if ( resp.values_size() != 1 )
                        return false;
                    return detail::equal( resp.values().Get( 0 ), pos );
                },
                std::move( callback ) );
    }

    auto msg = jointData->getCmdMsg( messages::JointCmd::SET_POS_WITH_SPEED );
    msg.mutable_jointcmd()->mutable_setposwithspeed()->set_position( pos );
    msg.mutable_jointcmd()->mutable_setposwithspeed()->set_speed( speed );
    jointData->rofi.pub->Publish( std::move( msg ), true );
}

float Joint::getTorque() const
{
    auto result = jointData->registerPromise( messages::JointCmd::GET_TORQUE );
    auto msg = jointData->getCmdMsg( messages::JointCmd::GET_TORQUE );
    jointData->rofi.pub->Publish( std::move( msg ), true );

    auto resp = result.get();
    if ( resp->jointresp().values_size() != 1 )
        throw std::runtime_error( "Unexpected size of values from response" );
    return resp->jointresp().values().Get( 0 );
}

void Joint::setTorque( float torque )
{
    auto msg = jointData->getCmdMsg( messages::JointCmd::SET_TORQUE );
    msg.mutable_jointcmd()->mutable_settorque()->set_torque( torque );
    jointData->rofi.pub->Publish( std::move( msg ), true );
}


Connector::Connector( detail::ConnectorData & cdata ) : connectorData( &cdata )
{
}

ConnectorState Connector::getState() const
{
    auto result = connectorData->registerPromise( messages::ConnectorCmd::GET_STATE );
    auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::GET_STATE );
    connectorData->rofi.pub->Publish( std::move( msg ), true );

    auto resp = result.get();
    auto respState = resp->connectorresp().state();
    return { static_cast< ConnectorPosition >( respState.position() ),
             respState.internal(),
             respState.external(),
             respState.connected(),
             static_cast< ConnectorOrientation >( respState.orientation() ) };
}

void Connector::connect()
{
    auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::CONNECT );
    connectorData->rofi.pub->Publish( std::move( msg ), true );
}

void Connector::disconnect()
{
    auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::DISCONNECT );
    connectorData->rofi.pub->Publish( std::move( msg ), true );
}

void Connector::onPacket( std::function< void( Connector, Packet ) > callback )
{
    if ( !callback )
    {
        return;
    }
    connectorData->registerCallback(
            []( const messages::ConnectorResp & resp ) {
                return resp.cmdtype() == messages::ConnectorCmd::PACKET;
            },
            [ callback = std::move( callback ) ]( Connector connector,
                                                  detail::RoFIData::RofiRespPtr resp ) {
                callback( connector, detail::ConnectorData::getPacket( std::move( resp ) ) );
            } );
}

void Connector::send( Packet packet )
{
    static_assert( sizeof( Packet::value_type ) == sizeof( char ) );
    auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::PACKET );
    msg.mutable_connectorcmd()->mutable_packet()->set_message( packet.data(), packet.size() );
    connectorData->rofi.pub->Publish( std::move( msg ), true );
}

void Connector::connectPower( ConnectorLine )
{
    auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::CONNECT_POWER );
    connectorData->rofi.pub->Publish( std::move( msg ), true );
}

void Connector::disconnectPower( ConnectorLine )
{
    auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::DISCONNECT_POWER );
    connectorData->rofi.pub->Publish( std::move( msg ), true );
}

namespace detail
{
int RoFIData::rofiCount = 0;
std::mutex RoFIData::rofiCountMutex = {};

RoFIData::RoFIData( RoFI::Id id ) : id( id )
{
    {
        std::lock_guard< std::mutex > lock( rofiCountMutex );

        assert( rofiCount >= 0 );
        if ( rofiCount == 0 )
        {
            std::cout << "Starting gazebo client" << std::endl;
            gazebo::client::setup();
        }
        rofiCount++;
    }

    node = boost::make_shared< gazebo::transport::Node >();
    node->Init();

    std::string moduleName = "universalModule";
    std::string rofiName = "local RoFI";
    if ( id > 0 )
    {
        moduleName += "_" + std::to_string( static_cast< int >( id ) - 1 );
        rofiName = "RoFI " + std::to_string( static_cast< int >( id ) );
    }

    pub = node->Advertise< rofi::messages::RofiCmd >( "~/" + moduleName + "/control" );
    sub = node->Subscribe( "~/" + moduleName + "/response", &RoFIData::onResponse, this );


    std::cerr << "Waiting for connection with " << rofiName << "...\n";
    pub->WaitForConnection();
    std::cerr << "Connected to " << rofiName << "\n";

    getDescription();

    std::cerr << rofiName << " is ready...\n";
}

RoFIData::~RoFIData()
{
    std::lock_guard< std::mutex > lock( rofiCountMutex );

    assert( rofiCount > 0 );
    rofiCount--;
    if ( rofiCount == 0 )
    {
        gazebo::client::shutdown();
        std::cout << "Ending gazebo client" << std::endl;
    }
}

RoFI::Id RoFIData::getId() const
{
    return id;
}

Joint RoFIData::getJoint( int index )
{
    if ( index < 0 || static_cast< size_t >( index ) >= joints.size() )
    {
        throw std::out_of_range( "Joint index is out of range" );
    }
    return joints[ index ]->getJoint();
}

Connector RoFIData::getConnector( int index )
{
    if ( index < 0 || static_cast< size_t >( index ) >= connectors.size() )
    {
        throw std::out_of_range( "Connector index is out of range" );
    }
    return connectors[ index ]->getConnector();
}

void RoFIData::wait( int ms, std::function< void() > callback ) const
{
    {
        std::lock_guard< std::mutex > lock( waitCallbacksMapMutex );
        waitId++;
        assert( waitCallbacksMap.find( waitId ) == waitCallbacksMap.end() );
        waitCallbacksMap.emplace( waitId, std::move( callback ) );
    }

    messages::RofiCmd rofiCmd;
    rofiCmd.set_rofiid( getId() );
    rofiCmd.set_cmdtype( messages::RofiCmd::WAIT_CMD );
    rofiCmd.mutable_waitcmd()->set_waitid( waitId );
    rofiCmd.mutable_waitcmd()->set_waitms( ms );
    pub->Publish( std::move( rofiCmd ), true );
}

void RoFIData::onResponse( const RoFIData::RofiRespPtr & resp )
{
    using rofi::messages::RofiCmd;

    RofiCmd::Type resptype = resp->resptype();
    switch ( resptype )
    {
        case RofiCmd::JOINT_CMD:
        {
            int index = resp->jointresp().joint();
            if ( index < 0 || static_cast< size_t >( index ) >= joints.size() )
            {
                std::cerr << "Joint index of response is out of range\nIgnoring...\n";
                return;
            }
            joints[ index ]->onResponse( resp );
            break;
        }
        case RofiCmd::CONNECTOR_CMD:
        {
            int index = resp->connectorresp().connector();
            if ( index < 0 || static_cast< size_t >( index ) >= connectors.size() )
            {
                std::cerr << "Connector index of response is out of range\nIgnoring...\n";
                return;
            }
            connectors[ index ]->onResponse( resp );
            break;
        }
        case RofiCmd::DESCRIPTION:
        {
            std::lock_guard< std::mutex > lock( descriptionMutex );
            auto & description = resp->rofidescription();

            if ( description.jointcount() > 0 )
            {
                while ( joints.size() < static_cast< unsigned >( description.jointcount() ) )
                {
                    joints.push_back(
                            std::make_unique< detail::JointData >( *this, joints.size() ) );
                }
            }
            if ( description.connectorcount() > 0 )
            {
                while ( connectors.size()
                        < static_cast< unsigned >( description.connectorcount() ) )
                {
                    connectors.push_back(
                            std::make_unique< detail::ConnectorData >( *this, connectors.size() ) );
                }
            }

            std::cerr << "Number of joints: " << joints.size() << "\n";
            std::cerr << "Number of connectors: " << connectors.size() << "\n";
            hasDescription = true;
            break;
        }
        case RofiCmd::WAIT_CMD:
        {
            std::function< void() > callback;

            {
                std::lock_guard< std::mutex > lock( waitCallbacksMapMutex );
                auto it = waitCallbacksMap.find( resp->waitid() );
                if ( it == waitCallbacksMap.end() )
                {
                    std::cerr << "Got wait response without a callback waiting (ID: "
                              << resp->waitid() << ")" << std::endl;
                    break;
                }
                callback = std::move( it->second );
                waitCallbacksMap.erase( it );
            }

            if ( callback )
            {
                callback();
            }
            break;
        }
        default:
        {
            std::cerr << "Not recognized rofi cmd type\nIgnoring...\n";
            break;
        }
    }
}

void RoFIData::getDescription()
{
    while ( !hasDescription )
    {
        messages::RofiCmd rofiCmd;
        rofiCmd.set_rofiid( getId() );
        rofiCmd.set_cmdtype( messages::RofiCmd::DESCRIPTION );
        pub->WaitForConnection();
        pub->Publish( std::move( rofiCmd ), true );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }
}


Joint JointData::getJoint()
{
    return Joint( *this );
}

messages::RofiCmd JointData::getCmdMsg( messages::JointCmd::Type type ) const
{
    messages::RofiCmd rofiCmd;
    rofiCmd.set_rofiid( rofi.getId() );
    rofiCmd.set_cmdtype( messages::RofiCmd::JOINT_CMD );
    rofiCmd.mutable_jointcmd()->set_joint( jointNumber );
    rofiCmd.mutable_jointcmd()->set_cmdtype( type );
    return rofiCmd;
}

std::future< RoFIData::RofiRespPtr > JointData::registerPromise( messages::JointCmd::Type type )
{
    std::lock_guard< std::mutex > lock( respMapMutex );
    return respMap.emplace( type, std::promise< RoFIData::RofiRespPtr >() )->second.get_future();
}

void JointData::registerCallback( Check && pred, Callback && callback )
{
    Callback oldCallback;
    {
        std::lock_guard< std::mutex > lock( respCallbackMutex );
        oldCallback = std::move( respCallback.second );
        respCallback = { std::move( pred ), std::move( callback ) };
    }
    if ( oldCallback )
    {
        std::cerr << "Aborting old callback\n";
        // TODO abort oldCallback
    }
}

void JointData::onResponse( const RoFIData::RofiRespPtr & resp )
{
    assert( resp->resptype() == messages::RofiCmd::JOINT_CMD );
    assert( resp->jointresp().joint() == jointNumber );

    {
        std::lock_guard< std::mutex > lock( respMapMutex );
        auto range = respMap.equal_range( resp->jointresp().cmdtype() );
        for ( auto it = range.first; it != range.second; it++ )
        {
            it->second.set_value( resp );
        }
        respMap.erase( range.first, range.second );
    }
    {
        std::lock_guard< std::mutex > lock( respCallbackMutex );
        auto & check = respCallback.first;
        if ( check && check( resp->jointresp() ) )
        {
            if ( respCallback.second )
            {
                std::thread( std::move( respCallback.second ), getJoint() ).detach();
            }
            respCallback = {};
        }
    }
}


Connector ConnectorData::getConnector()
{
    return Connector( *this );
}

messages::RofiCmd ConnectorData::getCmdMsg( messages::ConnectorCmd::Type type ) const
{
    messages::RofiCmd rofiCmd;
    rofiCmd.set_rofiid( rofi.getId() );
    rofiCmd.set_cmdtype( messages::RofiCmd::CONNECTOR_CMD );
    rofiCmd.mutable_connectorcmd()->set_connector( connectorNumber );
    rofiCmd.mutable_connectorcmd()->set_cmdtype( type );
    return rofiCmd;
}

Connector::Packet ConnectorData::getPacket( RoFIData::RofiRespPtr resp )
{
    static_assert( sizeof( Connector::Packet::value_type ) == sizeof( char ) );

    const auto & message = resp->connectorresp().packet().message();
    Connector::Packet packet;
    packet.reserve( message.size() );
    for ( auto byte : message )
    {
        packet.push_back( static_cast< Connector::Packet::value_type >( byte ) );
    }
    return packet;
}

std::future< RoFIData::RofiRespPtr > ConnectorData::registerPromise(
        messages::ConnectorCmd::Type type )
{
    std::lock_guard< std::mutex > lock( respMapMutex );
    return respMap.emplace( type, std::promise< RoFIData::RofiRespPtr >() )->second.get_future();
}

void ConnectorData::registerCallback( Check && pred, Callback && callback )
{
    std::lock_guard< std::mutex > lock( respCallbacksMutex );
    respCallbacks.emplace_back( std::move( pred ), std::move( callback ) );
}

void ConnectorData::onResponse( const RoFIData::RofiRespPtr & resp )
{
    assert( resp->resptype() == messages::RofiCmd::CONNECTOR_CMD );
    assert( resp->connectorresp().connector() == connectorNumber );

    {
        std::lock_guard< std::mutex > lock( respMapMutex );
        auto range = respMap.equal_range( resp->connectorresp().cmdtype() );
        for ( auto it = range.first; it != range.second; it++ )
        {
            it->second.set_value( resp );
        }
        respMap.erase( range.first, range.second );
    }
    {
        std::lock_guard< std::mutex > lock( respCallbacksMutex );
        for ( const auto & respCallback : respCallbacks )
        {
            auto & check = respCallback.first;
            if ( check && check( resp->connectorresp() ) )
            {
                if ( respCallback.second )
                {
                    std::thread( respCallback.second, getConnector(), resp ).detach();
                }
            }
        }
    }
}
} // namespace detail
} // namespace hal
} // namespace rofi