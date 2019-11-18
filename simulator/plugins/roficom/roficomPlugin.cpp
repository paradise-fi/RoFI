#include "roficomPlugin.hpp"

#include "../common/utils.hpp"

namespace gazebo
{
void RoFICoMPlugin::Load( physics::ModelPtr model, sdf::ElementPtr /*sdf*/ )
{
    _model = model;
    gzmsg << "The RoFICoM plugin is attached to model [" << _model->GetName() << "]\n";

    extendJoint = _model->GetJoint( "extendJoint" );
    if ( !extendJoint )
    {
        extendJoint = _model->GetJoint( "RoFICoM::extendJoint" );
    }
    if ( !extendJoint )
    {
        if ( _model->GetJointCount() == 0 )
        {
            gzerr << "Could not get any joint from RoFICoM plugin\n";
            return;
        }
        extendJoint = _model->GetJoints()[ 0 ];
        gzwarn << "Could not get extend joint from RoFICoM plugin, using first joint " << extendJoint->GetName() << "\n";
    }

    extendJoint->SetParam( "fmax", 0, maxJointForce );
    stop();

    initCommunication();
    initSensorCommunication();

    onUpdateConnection = event::Events::ConnectWorldUpdateBegin( std::bind( &RoFICoMPlugin::onUpdate, this ) );

    gzmsg << "RoFICoM plugin ready\n";
}

void RoFICoMPlugin::connect()
{
    gzmsg << "Connecting connector " << connectorNumber << "\n";

    std::lock_guard< std::mutex > lock( connectionMutex );
    if ( position != Position::Retracted )
    {
        gzmsg << "Already extended\n";
        return;
    }
    position = Position::Extending;
    gzmsg << "Extending connector " << connectorNumber << "\n";

    extend();
}

void RoFICoMPlugin::disconnect()
{
    gzmsg << "Disconnecting connector " << connectorNumber << "\n";

    std::lock_guard< std::mutex > lock( connectionMutex );
    if ( position == Position::Retracted )
    {
        gzmsg << "Not extended\n";
        return;
    }
    position = Position::Retracted;
    gzmsg << "Retracting connector " << connectorNumber << "\n";

    retract();
}

void RoFICoMPlugin::sendPacket( const rofi::messages::Packet & packet )
{
    gzmsg << "Sending packet from RoFI in connector " << connectorNumber << "\n";

    auto msg = getConnectorResp( rofi::messages::ConnectorCmd::PACKET );
    *msg.mutable_packet() = packet;
    _pubOutside->Publish( std::move( msg ) );
}

void RoFICoMPlugin::onPacket( const RoFICoMPlugin::PacketPtr & packet )
{
    gzmsg << "Received packet in connector " << connectorNumber << "\nSending to RoFI\n";

    auto msg = getConnectorResp( rofi::messages::ConnectorCmd::PACKET );
    *msg.mutable_packet() = *packet;
    _pubRofi->Publish( std::move( msg ) );
}

void RoFICoMPlugin::initCommunication()
{
    if ( !_model )
    {
        gzerr << "Model has to be set before initializing sensor communication\n";
        return;
    }

    if ( _node )
    {
        _node->Fini();
    }

    _node = boost::make_shared< transport::Node >();
    _node->Init( getElemPath( _model ) );

    _pubRofi = _node->Advertise< rofi::messages::ConnectorResp >( "~/response" );
    _subRofi = _node->Subscribe( "~/control", & RoFICoMPlugin::onConnectorCmd, this );
}

void RoFICoMPlugin::initSensorCommunication()
{
    if ( !_node->IsInitialized() )
    {
        gzerr << "Initialize communication before initializing sensor communication\n";
        return;
    }

    auto sensors = _model->SensorScopedName( "roficom-sensor" );

    if ( sensors.size() != 1 )
    {
        gzerr << "RoFICoM plugin expects exactly 1 nested sensor named 'roficom-sensor' (found " << sensors.size() << ")\n";
        return;
    }

    auto topicName = "/gazebo" + replaceDelimeter( sensors.front() ) + "/contacts";
    _subSensor = _node->Subscribe( std::move( topicName ), & RoFICoMPlugin::onSensorMessage, this );
}

void RoFICoMPlugin::onUpdate()
{
    std::lock_guard< std::mutex > lock( connectionMutex );

    if ( position == Position::Retracted )
    {
        if ( equal( extendJoint->Position(), extendJoint->LowerLimit( 0 ), positionPrecision ) )
        {
            onRetracted();
        }
        return;
    }

    if ( equal( extendJoint->Position(), extendJoint->UpperLimit( 0 ), positionPrecision ) )
    {
        onExtended();
    }

    checkConnection();
}

void RoFICoMPlugin::stop()
{
    if ( currentVelocity != 0.0 )
    {
        gzmsg << "stop\n";
        currentVelocity = 0.0;
        extendJoint->SetParam( "vel", 0, currentVelocity );
    }
}

void RoFICoMPlugin::extend()
{
    if ( currentVelocity != maxJointSpeed )
    {
        gzmsg << "extend\n";
        currentVelocity = maxJointSpeed;
        extendJoint->SetParam( "vel", 0, currentVelocity );
    }
}

void RoFICoMPlugin::retract()
{
    if ( currentVelocity != -maxJointSpeed )
    {
        gzmsg << "retract\n";
        currentVelocity = -maxJointSpeed;
        extendJoint->SetParam( "vel", 0, currentVelocity );
    }
}

void RoFICoMPlugin::onExtended()
{
    stop();
    if ( position == Position::Extending )
    {
        position = Position::Extended;
    }
}

void RoFICoMPlugin::onRetracted()
{
    stop();
    if ( position == Position::Connected )
    {
        endConnection();
    }
    position = Position::Retracted;
}

void RoFICoMPlugin::checkConnection()
{
    if ( position != Position::Connected )
    {
        return;
    }

    // TODO check if still connected
    if ( false )
    {
        endConnection();
        position = Position::Extended;
    }
}

void RoFICoMPlugin::endConnection()
{
    // TODO
}

void RoFICoMPlugin::onConnectorCmd( const ConnectorCmdPtr & msg )
{
    using rofi::messages::ConnectorCmd;

    connectorNumber = msg->connector();

    switch ( msg->cmdtype() )
    {
    case ConnectorCmd::NO_CMD:
    {
        _pubRofi->Publish( getConnectorResp( ConnectorCmd::NO_CMD ) );
        gzmsg << "Sending response to NO_CMD message\n";
        break;
    }
    case ConnectorCmd::GET_STATE:
    {
        auto msg = getConnectorResp( ConnectorCmd::GET_STATE );
        auto & state = *msg.mutable_state();
        {
            std::lock_guard< std::mutex > lock( connectionMutex );
            switch ( position )
            {
                case Position::Connected:
                    state.set_connected( true );
                    state.set_orientation( orientation );
                    [[ fallthrough ]];
                case Position::Extended:
                case Position::Extending:
                    state.set_position( true );
                    break;
                case Position::Retracted:
                    break;
            }
        }
        _pubRofi->Publish( std::move( msg ) );
        gzmsg << "Returning state\n";
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
        gzmsg << "Connecting power line is not implemented\n";
        break;
    }
    case ConnectorCmd::DISCONNECT_POWER:
    {
        gzmsg << "Disconnecting power line is not implemented\n";
        break;
    }
    default:
        gzwarn << "Unknown command type: " << msg->cmdtype() << " of connector " << connectorNumber << "\n";
        break;
    }
}

void RoFICoMPlugin::onSensorMessage( const ContactsMsgPtr & contacts )
{
    std::lock_guard< std::mutex > lock( connectionMutex );
    if ( position != Position::Extending )
    {
        return;
    }

    for ( auto & contact : contacts->contact() )
    {
        auto otherModel = getModelOfOther( contact );

        if ( !canBeConnected( otherModel ) )
        {
            continue;
        }

        gzmsg << "Connecting with " << otherModel->GetScopedName() << "\n";

        position = Position::Connected;

        // TODO connect on the right place
        // TODO start communication

        return;
    }
}

physics::ModelPtr RoFICoMPlugin::getModelOfOther( const msgs::Contact & contact ) const
{
    auto collision1 = getCollisionByScopedName( contact.collision1() );
    auto collision2 = getCollisionByScopedName( contact.collision2() );
    if ( !collision1  || !collision2 )
    {
        gzwarn << "Got empty collision\n";
        return {};
    }

    auto model1 = collision1->GetModel();
    auto model2 = collision2->GetModel();
    if ( model1 == _model )
    {
        if ( model2 == _model )
        {
            gzwarn << "Both collisions are from this model\n";
            return {};
        }
        return model2;
    }
    if ( model2 != _model )
    {
        gzwarn << "None of the collisions are from this model\n";
        return {};
    }
    return model1;
}

physics::CollisionPtr RoFICoMPlugin::getCollisionByScopedName( const std::string & collisionName ) const
{
    return boost::dynamic_pointer_cast< physics::Collision >( _model->GetWorld()->EntityByName( collisionName ) );
}

rofi::messages::ConnectorResp RoFICoMPlugin::getConnectorResp( rofi::messages::ConnectorCmd::Type cmdtype ) const
{
    rofi::messages::ConnectorResp resp;
    resp.set_connector( connectorNumber );
    resp.set_cmdtype( cmdtype );
    return resp;
}

bool RoFICoMPlugin::canBeConnected( physics::ModelPtr otherConnector ) const
{
    if ( !isRofiCoM( otherConnector ) )
    {
        return false;
    }

    // TODO check position and rotation

    return true;
}

GZ_REGISTER_MODEL_PLUGIN( RoFICoMPlugin )

} // namespace gazebo
