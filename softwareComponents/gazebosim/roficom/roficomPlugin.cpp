#include "roficomPlugin.hpp"

#include <cassert>
#include <cmath>

namespace gazebo
{
std::recursive_mutex RoFICoMPlugin::positionMutex;
std::map< const physics::Model *, RoFICoMPlugin::Position > RoFICoMPlugin::positionsMap;

void RoFICoMPlugin::Load( physics::ModelPtr model, sdf::ElementPtr sdf )
{
    _model = std::move( model );
    _sdf = std::move( sdf );
    assert( _model );
    assert( _sdf );

    gzmsg << "The RoFICoM plugin is attached to model [" << _model->GetScopedName() << "]\n";

    if ( !hasAttacherPlugin( _model->GetWorld() ) )
    {
        gzerr << "Could not find the attacher world plugin. "
              << "Connecting roficoms will not work.\n";
    }

    loadJoint();
    assert( extendJoint && *extendJoint );

    initCommunication();
    assert( _node );
    roficomConnection.load( *this, _model, _node );

    startListening();

    gzmsg << "RoFICoM plugin ready (" << _model->GetScopedName() << ")\n";
}

void RoFICoMPlugin::jointPositionReachedCallback( Position newPosition )
{
    assert( newPosition == Position::Retracted || newPosition == Position::Extended );

    std::lock_guard< std::recursive_mutex > lock( positionMutex );
    updatePosition( newPosition );

    if ( newPosition == Position::Extended )
    {
        roficomConnection.connectToNearbyRequest();
    }
}

void RoFICoMPlugin::loadJoint()
{
    assert( _sdf );

    auto controllerValues = RoficomController::loadControllerValues( _sdf );

    auto joint = _model->GetJoint( controllerValues.jointName );
    if ( !joint )
    {
        gzerr << "No joint with name '" + controllerValues.jointName + "' found in roficom\n";
        throw std::runtime_error( "No joint with name '" + controllerValues.jointName
                                  + "' found in roficom" );
    }
    if ( joint->GetMsgType() != msgs::Joint::PRISMATIC )
    {
        gzerr << "Controlled joint in roficom has to be prismatic\n";
        throw std::runtime_error( "Controlled joint in roficom has to be prismatic" );
    }

    updatePosition( controllerValues.position() );
    extendJoint = std::make_unique< JointData< RoficomController > >(
            std::move( joint ),
            controllerValues.limitSdf,
            controllerValues.position(),
            [ this ]( Position pos ) { this->jointPositionReachedCallback( pos ); } );

    assert( extendJoint );
    assert( extendJoint->joint );
    assert( extendJoint->joint->GetMsgType() == msgs::Joint::PRISMATIC );
}

void RoFICoMPlugin::connect()
{
    gzmsg << "Extending connector " << connectorNumber << " (" << _model->GetScopedName() << ")\n";

    std::lock_guard< std::recursive_mutex > lock( positionMutex );

    switch ( getPosition() )
    {
        case Position::Extended:
        case Position::Extending:
        {
            gzwarn << "Already extending (" << _model->GetScopedName() << ")\n";
            return;
        }
        default:
            break;
    }

    updatePosition( Position::Extending );
    extendJoint->controller.extend();
}

void RoFICoMPlugin::disconnect()
{
    assert( extendJoint );
    gzmsg << "Retracting connector " << connectorNumber << " (" << _model->GetScopedName() << ")\n";

    std::lock_guard< std::recursive_mutex > lock( positionMutex );

    switch ( getPosition() )
    {
        case Position::Retracted:
        case Position::Retracting:
        {
            gzwarn << "Already retracting (" << _model->GetScopedName() << ")\n";
            return;
        }
        default:
            break;
    }

    updatePosition( Position::Retracting );
    roficomConnection.disconnectRequest();
    extendJoint->controller.retract();
}

void RoFICoMPlugin::sendPacket( const rofi::messages::Packet & packet )
{
    gzmsg << "Sending packet (" << _model->GetScopedName() << ")\n";

    roficomConnection.sendPacket( packet );
}

void RoFICoMPlugin::onPacket( const rofi::messages::Packet & packet )
{
    gzmsg << "Received packet (" << _model->GetScopedName() << ")\n";

    auto msg = getConnectorResp( rofi::messages::ConnectorCmd::PACKET );
    *msg.mutable_packet() = packet;
    _pubRofi->Publish( std::move( msg ), true );
}

void RoFICoMPlugin::onConnectorEvent( rofi::messages::ConnectorCmd::Type eventType )
{
    using rofi::messages::ConnectorCmd;
    using rofi::messages::ConnectorState;

    assert( eventType != ConnectorCmd::GET_STATE );
    assert( eventType != ConnectorCmd::PACKET );

    _pubRofi->Publish( getConnectorResp( eventType ), true );
}

bool RoFICoMPlugin::isConnected() const
{
    bool connected = roficomConnection.isConnected();
    assert( !connected || getPosition() == Position::Extended );
    return connected;
}

void RoFICoMPlugin::initCommunication()
{
    assert( _model );

    if ( _node )
    {
        _node->Fini();
    }

    _node = boost::make_shared< transport::Node >();
    assert( _node );
    _node->Init( getElemPath( _model ) );

    _pubRofi = _node->Advertise< rofi::messages::ConnectorResp >( "~/response" );
    assert( _pubRofi );
}

void RoFICoMPlugin::startListening()
{
    assert( _node );
    assert( _node->IsInitialized() );

    _subRofi = _node->Subscribe( "~/control", &RoFICoMPlugin::onConnectorCmd, this );
    assert( _subRofi );
}

std::optional< RoFICoMPlugin::Orientation > RoFICoMPlugin::getOrientation() const
{
    auto orientation = roficomConnection.getOrientation();
    assert( orientation.has_value() == isConnected() );
    return orientation;
}

void RoFICoMPlugin::updatePosition( Position newPosition )
{
    assert( _model );
    assert( _sdf );

    bool extend = newPosition == Position::Extending || newPosition == Position::Extended;

    std::lock_guard< std::recursive_mutex > lock( positionMutex );

    getOnlyChild< true >( _sdf, "extend" )->Set( extend );
    positionsMap[ _model.get() ] = newPosition;
}

RoFICoMPlugin::Position RoFICoMPlugin::getPosition() const
{
    assert( _model );

    return getOtherPosition( _model );
}

RoFICoMPlugin::Position RoFICoMPlugin::getOtherPosition( physics::ModelPtr roficom )
{
    assert( roficom );
    assert( isRoFICoM( roficom ) );

    std::lock_guard< std::recursive_mutex > lock( positionMutex );

    return positionsMap[ roficom.get() ];
}

void RoFICoMPlugin::removePosition()
{
    assert( _model );

    std::lock_guard< std::recursive_mutex > lock( positionMutex );

    positionsMap.erase( _model.get() );
}

void RoFICoMPlugin::onConnectorCmd( const ConnectorCmdPtr & msg )
{
    using rofi::messages::ConnectorCmd;
    using rofi::messages::ConnectorState;

    connectorNumber = msg->connector();

    switch ( msg->cmdtype() )
    {
        case ConnectorCmd::NO_CMD:
            break;
        case ConnectorCmd::GET_STATE:
        {
            auto msg = getConnectorResp( ConnectorCmd::GET_STATE );
            auto & state = *msg.mutable_state();

            {
                switch ( getPosition() )
                {
                    case Position::Extended:
                    {
                        state.set_position( true );

                        auto orientation = getOrientation();
                        assert( orientation.has_value() == isConnected() );

                        if ( orientation )
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
                    {
                        state.set_position( true );
                        state.set_connected( false );
                        break;
                    }
                    case Position::Retracting:
                    case Position::Retracted:
                    {
                        state.set_position( false );
                        state.set_connected( false );
                        break;
                    }
                }
            }
            _pubRofi->Publish( std::move( msg ), true );
            gzmsg << "Returning state (" << _model->GetScopedName() << ")\n";
            break;
        }
        case ConnectorCmd::CONNECT:
        {
            connect();
            break;
        }
        case ConnectorCmd::DISCONNECT:
        {
            disconnect();
            break;
        }
        case ConnectorCmd::PACKET:
        {
            sendPacket( msg->packet() );
            break;
        }
        case ConnectorCmd::CONNECT_POWER:
        {
            gzwarn << "Connecting power line is not implemented\n";
            break;
        }
        case ConnectorCmd::DISCONNECT_POWER:
        {
            gzwarn << "Disconnecting power line is not implemented\n";
            break;
        }
        default:
            gzwarn << "Unknown command type: " << msg->cmdtype() << " (" << _model->GetScopedName()
                   << ")\n";
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

GZ_REGISTER_MODEL_PLUGIN( RoFICoMPlugin )

} // namespace gazebo
