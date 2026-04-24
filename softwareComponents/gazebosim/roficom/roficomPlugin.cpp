#include "roficomPlugin.hpp"

#include <gz/plugin/Register.hh>

namespace gazebo
{
std::recursive_mutex RoFICoMPlugin::positionMutex;
std::map< gz::sim::Entity, RoFICoMPlugin::Position > RoFICoMPlugin::positionsMap;

void RoFICoMPlugin::Configure( const gz::sim::Entity & entity,
                               const std::shared_ptr< const sdf::Element > & sdf,
                               gz::sim::EntityComponentManager & ecm,
                               gz::sim::EventManager & )
{
    _entity = entity;
    _model = gz::sim::Model( entity );
    _sdf = sdf;

    loadJoint( ecm );
    initCommunication( ecm );
    roficomConnection.load( *this, _entity, ecm, _node );
    startListening();
}

void RoFICoMPlugin::PreUpdate( const gz::sim::UpdateInfo & info,
                               gz::sim::EntityComponentManager & ecm )
{
    roficomConnection.processPending( ecm );

    std::vector< rofi::messages::ConnectorCmd > pendingCommands;
    {
        std::lock_guard lock( _queueMutex );
        pendingCommands.swap( _pendingCommands );
    }

    for ( const auto & cmd : pendingCommands )
    {
        handleConnectorCmd( cmd, ecm );
    }

    if ( extendJoint )
    {
        extendJoint->controller.update( info, ecm );
    }

    if ( _connectToNearbyRequested )
    {
        _connectToNearbyRequested = false;
        roficomConnection.connectToNearbyRequest( ecm );
    }
}

void RoFICoMPlugin::loadJoint( gz::sim::EntityComponentManager & ecm )
{
    auto controllerValues = RoficomController::loadControllerValues( _sdf );
    auto jointEntity = _model.JointByName( ecm, controllerValues.jointName );
    if ( jointEntity == gz::sim::kNullEntity )
    {
        throw std::runtime_error( "No controlled joint found in RoFICoM model" );
    }

    updatePosition( controllerValues.position() );
    extendJoint = std::make_unique< JointData< RoficomController > >(
            jointEntity,
            controllerValues.limitSdf,
            ecm,
            controllerValues.position(),
            [ this ]( Position pos ) { jointPositionReachedCallback( pos ); } );
}

void RoFICoMPlugin::initCommunication( const gz::sim::EntityComponentManager & ecm )
{
    _node = std::make_shared< rofi::gz::Node >();
    _node->Init( getElemPath( _entity, ecm ) );
    _pubRofi = _node->Advertise< rofi::messages::ConnectorResp >( "~/response" );
}

void RoFICoMPlugin::startListening()
{
    _subRofi = _node->Subscribe( "~/control", &RoFICoMPlugin::onConnectorCmd, this );
}

void RoFICoMPlugin::jointPositionReachedCallback( Position newPosition )
{
    updatePosition( newPosition );
    if ( newPosition == Position::Extended )
    {
        _connectToNearbyRequested = true;
    }
}

void RoFICoMPlugin::connect( gz::sim::EntityComponentManager & ecm )
{
    std::lock_guard< std::recursive_mutex > lock( positionMutex );
    switch ( getPosition() )
    {
        case Position::Extended:
        case Position::Extending:
            return;
        default:
            break;
    }

    updatePosition( Position::Extending );
    extendJoint->controller.extend( ecm );
}

void RoFICoMPlugin::disconnect( gz::sim::EntityComponentManager & ecm )
{
    std::lock_guard< std::recursive_mutex > lock( positionMutex );
    switch ( getPosition() )
    {
        case Position::Retracted:
        case Position::Retracting:
            return;
        default:
            break;
    }

    updatePosition( Position::Retracting );
    roficomConnection.disconnectRequest();
    extendJoint->controller.retract( ecm );
}

void RoFICoMPlugin::sendPacket( const rofi::messages::Packet & packet )
{
    roficomConnection.sendPacket( packet );
}

void RoFICoMPlugin::onPacket( const rofi::messages::Packet & packet )
{
    auto msg = getConnectorResp( rofi::messages::ConnectorCmd::PACKET );
    *msg.mutable_packet() = packet;
    _pubRofi->Publish( msg, true );
}

void RoFICoMPlugin::onConnectorEvent( rofi::messages::ConnectorCmd::Type eventType )
{
    _pubRofi->Publish( getConnectorResp( eventType ), true );
}

bool RoFICoMPlugin::isConnected() const
{
    return roficomConnection.isConnected();
}

void RoFICoMPlugin::onConnectorCmd( const ConnectorCmdPtr & msg )
{
    if ( !msg )
    {
        return;
    }
    std::lock_guard lock( _queueMutex );
    _pendingCommands.push_back( *msg );
}

void RoFICoMPlugin::handleConnectorCmd( const rofi::messages::ConnectorCmd & msg,
                                        gz::sim::EntityComponentManager & ecm )
{
    using rofi::messages::ConnectorCmd;

    connectorNumber = msg.connector();
    switch ( msg.cmdtype() )
    {
        case ConnectorCmd::NO_CMD:
            break;
        case ConnectorCmd::GET_STATE:
        {
            auto response = getConnectorResp( ConnectorCmd::GET_STATE );
            auto & state = *response.mutable_state();

            switch ( getPosition() )
            {
                case Position::Extended:
                {
                    state.set_position( true );
                    if ( auto orientation = getOrientation() )
                    {
                        state.set_connected( true );
                        state.set_orientation( *orientation );
                    }
                    else
                    {
                        state.set_connected( false );
                    }
                    break;
                }
                case Position::Extending:
                    state.set_position( true );
                    state.set_connected( false );
                    break;
                case Position::Retracted:
                case Position::Retracting:
                    state.set_position( false );
                    state.set_connected( false );
                    break;
            }
            _pubRofi->Publish( response, true );
            break;
        }
        case ConnectorCmd::CONNECT:
            connect( ecm );
            break;
        case ConnectorCmd::DISCONNECT:
            disconnect( ecm );
            break;
        case ConnectorCmd::PACKET:
            sendPacket( msg.packet() );
            break;
        case ConnectorCmd::CONNECT_POWER:
        case ConnectorCmd::DISCONNECT_POWER:
            gzwarn << "Power lines are not implemented\n";
            break;
        default:
            gzwarn << "Unknown connector command type: " << msg.cmdtype() << "\n";
            break;
    }
}

rofi::messages::ConnectorResp RoFICoMPlugin::getConnectorResp(
        rofi::messages::ConnectorCmd::Type resptype ) const
{
    rofi::messages::ConnectorResp resp;
    resp.set_connector( connectorNumber );
    resp.set_resptype( resptype );
    return resp;
}

std::optional< RoFICoMPlugin::Orientation > RoFICoMPlugin::getOrientation() const
{
    return roficomConnection.getOrientation();
}

void RoFICoMPlugin::updatePosition( Position newPosition )
{
    std::lock_guard< std::recursive_mutex > lock( positionMutex );
    positionsMap[ _entity ] = newPosition;
}

RoFICoMPlugin::Position RoFICoMPlugin::getPosition() const
{
    return getOtherPosition( _entity );
}

RoFICoMPlugin::Position RoFICoMPlugin::getOtherPosition( gz::sim::Entity roficom )
{
    std::lock_guard< std::recursive_mutex > lock( positionMutex );
    return positionsMap.at( roficom );
}

void RoFICoMPlugin::removePosition()
{
    std::lock_guard< std::recursive_mutex > lock( positionMutex );
    positionsMap.erase( _entity );
}

} // namespace gazebo

GZ_ADD_PLUGIN( gazebo::RoFICoMPlugin,
               gz::sim::System,
               gz::sim::ISystemConfigure,
               gz::sim::ISystemPreUpdate )
GZ_ADD_PLUGIN_ALIAS( gazebo::RoFICoMPlugin, "rofi::sim::systems::RoFICoMPlugin" )
