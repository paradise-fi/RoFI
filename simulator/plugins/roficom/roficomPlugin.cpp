#include "roficomPlugin.hpp"

#include "../common/utils.hpp"

namespace gazebo
{
void RoFICoMPlugin::Load( physics::ModelPtr model, sdf::ElementPtr /*sdf*/ )
{
    _model = model;
    gzmsg << "The RoFICoM plugin is attached to model [" << _model->GetName() << "]\n";

    initCommunication();
    initSensorCommunication();

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

    // TODO
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

    // TODO
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
        auto otherCollision = getCollisionByScopedName( contact.collision2() );
        auto thisCollision = getCollisionByScopedName( contact.collision1() );
        if ( !thisCollision  || !otherCollision )
        {
            gzwarn << "Got empty collision\n";
            continue;
        }
        if ( thisCollision->GetModel() != _model )
        {
            if ( otherCollision->GetModel() != _model )
            {
                gzwarn << "None of the collisions are from this model\n";
                continue;
            }
            otherCollision = std::move( thisCollision );
        }
        else
        {
            if ( otherCollision->GetModel() == _model )
            {
                gzwarn << "Both collisions are from this model\n";
                continue;
            }
            thisCollision = {};
        }

        if ( !canBeConnected( otherCollision->GetModel() ) )
        {
            continue;
        }

        gzmsg << "Connecting with " << otherCollision->GetModel()->GetScopedName() << "\n";

        position = Position::Connected;

        // TODO connect on the right place
        // TODO start communication

        return;
    }
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
