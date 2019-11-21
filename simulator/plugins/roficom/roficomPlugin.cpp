#include "roficomPlugin.hpp"

#include <cassert>
#include <cmath>

#include "../common/utils.hpp"

namespace gazebo
{
void RoFICoMPlugin::Load( physics::ModelPtr model, sdf::ElementPtr /*sdf*/ )
{
    _model = std::move( model );
    assert( _model );
    gzmsg << "The RoFICoM plugin is attached to model [" << _model->GetScopedName() << "]\n";

    extendJoint = _model->GetJoint( "extendJoint" );
    if ( !extendJoint )
    {
        extendJoint = _model->GetJoint( "RoFICoM::extendJoint" );
    }
    if ( !extendJoint )
    {
        for ( auto joint : _model->GetJoints() )
        {
            auto name = joint->GetName();
            if ( name.size() < 13 )
            {
                continue;
            }
            if ( name.compare( name.size() - 13, 13, "::extendJoint" ) == 0 )
            {
                if ( extendJoint )
                {
                    gzwarn << "Found two extendJoints: " << extendJoint->GetName() << ", " << joint->GetName() << "\n";
                    break;
                }
                extendJoint = std::move( joint );
            }
        }
    }
    if ( !extendJoint )
    {
        gzerr << "Could not get extend joint in RoFICoM plugin\n";

        throw std::runtime_error( "Could not get extend joint in RoFICoM plugin" );
    }

    thisConnectionLink = getConnectionLink( _model );
    if ( !thisConnectionLink )
    {
        gzerr << "Could not get connection link in RoFICoM plugin\n";

        throw std::runtime_error( "Could not get connection link in RoFICoM plugin" );
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
    gzmsg << "Connecting connector " << connectorNumber << " (" << _model->GetScopedName() << ")\n";

    std::lock_guard< std::mutex > lock( connectionMutex );

    if ( position != Position::Retracted && position != Position::Retracting )
    {
        gzmsg << "Already extended (" << _model->GetScopedName() << ")\n";
        return;
    }
    position = Position::Extending;
    gzmsg << "Extending connector  (" << _model->GetScopedName() << ")\n";

    extend();
}

void RoFICoMPlugin::disconnect()
{
    gzmsg << "Disconnecting connector " << connectorNumber << " (" << _model->GetScopedName() << ")\n";

    std::lock_guard< std::mutex > lock( connectionMutex );
    if ( position == Position::Retracted || position == Position::Retracting )
    {
        gzmsg << "Not extended (" << _model->GetScopedName() << ")\n";
        return;
    }

    gzmsg << "Retracting connector " << connectorNumber << " (" << _model->GetScopedName() << ")\n";
    position = Position::Retracting;
    endConnection();

    retract();
}

void RoFICoMPlugin::sendPacket( const rofi::messages::Packet & packet )
{
    gzmsg << "Sending packet (" << _model->GetScopedName() << ")\n";

    _pubOutside->Publish( packet );
}

void RoFICoMPlugin::onPacket( const RoFICoMPlugin::PacketPtr & packet )
{
    gzmsg << "Received packet (" << _model->GetScopedName() << ")\n";

    auto msg = getConnectorResp( rofi::messages::ConnectorCmd::PACKET );
    *msg.mutable_packet() = *packet;
    _pubRofi->Publish( std::move( msg ) );
}

bool RoFICoMPlugin::isConnected() const
{
    bool connected = bool( connectedWith );
    assert( connected == bool( connectionJoint ) );
    assert( connected == bool( _subOutside ) );
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
    _subRofi = _node->Subscribe( "~/control", & RoFICoMPlugin::onConnectorCmd, this );
    _pubOutside = _node->Advertise< rofi::messages::Packet >( "~/packets" );
    assert( _pubRofi );
    assert( _subRofi );
    assert( _pubOutside );
}

void RoFICoMPlugin::initSensorCommunication()
{
    assert( _model );
    assert( _node );
    assert( _node->IsInitialized() );

    auto sensors = _model->SensorScopedName( "roficom-sensor" );

    if ( sensors.size() != 1 )
    {
        gzerr << "RoFICoM plugin expects exactly 1 nested sensor named 'roficom-sensor' (found " << sensors.size() << ")\n";
        return;
    }

    auto topicName = "/gazebo" + replaceDelimeter( sensors.front() ) + "/contacts";
    _subSensor = _node->Subscribe( std::move( topicName ), & RoFICoMPlugin::onSensorMessage, this );
    assert( _subSensor );
}

void RoFICoMPlugin::onUpdate()
{
    std::lock_guard< std::mutex > lock( connectionMutex );

    if ( position == Position::Retracting )
    {
        if ( equal( extendJoint->Position(), extendJoint->LowerLimit( 0 ), positionPrecision ) )
        {
            stop();
            position = Position::Retracted;
        }
    }
    if ( position == Position::Extending )
    {
        if ( equal( extendJoint->Position(), extendJoint->UpperLimit( 0 ), positionPrecision ) )
        {
            stop();
            position = Position::Extended;
        }
    }

    updateConnection();
}

void RoFICoMPlugin::stop()
{
    if ( currentVelocity != 0.0 )
    {
        gzmsg << "stop (" << _model->GetScopedName() << ")\n";
        currentVelocity = 0.0;
        extendJoint->SetParam( "vel", 0, currentVelocity );
    }
}

void RoFICoMPlugin::extend()
{
    if ( currentVelocity != maxJointSpeed )
    {
        gzmsg << "extend (" << _model->GetScopedName() << ")\n";
        currentVelocity = maxJointSpeed;
        extendJoint->SetParam( "vel", 0, currentVelocity );
    }
}

void RoFICoMPlugin::retract()
{
    if ( currentVelocity != -maxJointSpeed )
    {
        gzmsg << "retract (" << _model->GetScopedName() << ")\n";
        currentVelocity = -maxJointSpeed;
        extendJoint->SetParam( "vel", 0, currentVelocity );
    }
}

void RoFICoMPlugin::updateConnection()
{
    if ( !isConnected() )
    {
        return;
    }

    if ( position == Position::Extended || position == Position::Extending )
    {
        if ( !connectionJoint->AreConnected( thisConnectionLink, connectedWith ) )
        {
            endConnection();
            return;
        }
    }

    if ( position == Position::Retracted || position == Position::Retracting )
    {
        endConnection();
        return;
    }
}

void RoFICoMPlugin::endConnection()
{
    connectedWith = {};
    if ( connectionJoint )
    {
        connectionJoint->Detach();
        connectionJoint = {};
    }
    if ( _subOutside )
    {
        _subOutside->Unsubscribe();
        _subOutside = {};
    }
}

void RoFICoMPlugin::createConnection( physics::LinkPtr otherConnectionLink )
{
    assert( thisConnectionLink );
    assert( otherConnectionLink );
    assert( thisConnectionLink != otherConnectionLink );
    assert( isRofiCoM( otherConnectionLink->GetModel() ) );
    assert( !isConnected() );

    connectionJoint = getConnectionJoint( otherConnectionLink );

    if ( !connectionJoint )
    {
        // TODO create joint
        connectionJoint = _model->CreateJoint( "outerConnection", "prismatic", thisConnectionLink, otherConnectionLink );
        connectionJoint->SetParam( "fmax", 0, 15.0 );
        connectionJoint->SetParam( "vel", 0, 0.0 );
        connectionJoint->Init();
    }

    connectedWith = otherConnectionLink;
    startCommunication( otherConnectionLink->GetModel() );
}

void RoFICoMPlugin::startCommunication( physics::ModelPtr otherModel )
{
    assert( otherModel );
    assert( !_subOutside );

    auto path = "/gazebo/" + getElemPath( otherModel ) + "/packets";
    _subOutside = _node->Subscribe( path, &RoFICoMPlugin::onPacket, this );
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
        gzmsg << "Sending response to NO_CMD message (" << _model->GetScopedName() << ")\n";
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
                case Position::Extended:
                case Position::Extending:
                    state.set_position( true );
                    if ( isConnected() )
                    {
                        state.set_connected( true );
                        state.set_orientation( orientation );
                    }
                    break;
                case Position::Retracting:
                case Position::Retracted:
                    break;
            }
        }
        _pubRofi->Publish( std::move( msg ) );
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
        gzmsg << "Connecting power line is not implemented\n";
        break;
    }
    case ConnectorCmd::DISCONNECT_POWER:
    {
        gzmsg << "Disconnecting power line is not implemented\n";
        break;
    }
    default:
        gzwarn << "Unknown command type: " << msg->cmdtype() << " (" << _model->GetScopedName() << ")\n";
        break;
    }
}

void RoFICoMPlugin::onSensorMessage( const ContactsMsgPtr & contacts )
{
    std::lock_guard< std::mutex > lock( connectionMutex );
    if ( isConnected() )
    {
        return;
    }

    if ( position != Position::Extended && position != Position::Extending )
    {
        return;
    }

    for ( auto & contact : contacts->contact() )
    {
        auto otherModel = getModelOfOther( contact );
        if ( !isRofiCoM( otherModel ) )
        {
            break;
        }

        auto otherLink = getConnectionLink( otherModel );
        if ( !otherLink )
        {
            gzwarn << "No connection link in other RoFICoM\n";
            break;
        }

        if ( position == Position::Extended )
        {
            connectionJoint = getConnectionJoint( otherLink );

            if ( connectionJoint )
            {
                gzmsg << "Connected by " << otherModel->GetScopedName() << " (" << _model->GetScopedName() << ")\n";
                connectedWith = otherLink;
                startCommunication( otherLink->GetModel() );
                return;
            }
        }
        else
        {
            assert( position == Position::Extending );

            if ( canBeConnected( otherLink ) )
            {
                gzmsg << "Connecting with " << otherLink->GetModel()->GetScopedName() << " (" << _model->GetScopedName() << ")\n";

                createConnection( otherLink );
                return;
            }
        }
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

bool RoFICoMPlugin::canBeConnected( physics::LinkPtr otherConnectionLink ) const
{
    assert( thisConnectionLink );
    assert( otherConnectionLink );
    assert( thisConnectionLink != otherConnectionLink );
    assert( isRofiCoM( otherConnectionLink->GetModel() ) );

    auto thisPos = thisConnectionLink->WorldPose().Pos();
    auto otherPos = otherConnectionLink->WorldPose().Pos();

    auto squaredDistance = ( thisPos - otherPos ).SquaredLength();
    if ( squaredDistance > maxConnectionCenterDistance * maxConnectionCenterDistance )
    {
        return false;
    }

    auto otherModelPose = otherConnectionLink->GetModel()->WorldPose();
    auto squaredExtendDistance = ( otherModelPose.Pos() - otherPos ).SquaredLength();

    if ( squaredExtendDistance < extendDistance * extendDistance )
    {
        return false;
    }

    auto thisVector = _model->WorldPose().Rot().RotateVector( { 0, 0, 1 } );
    auto otherVector = otherModelPose.Rot().RotateVector( { 0, 0, -1 } );

    auto cosAngle = thisVector.Dot( otherVector ) / ( thisVector.Length() * otherVector.Length() );
    if ( cosAngle <= minConnectionCosAngle )
    {
        return false;
    }

    // TODO check connect orientation

    return true;
}

physics::LinkPtr RoFICoMPlugin::getConnectionLink( physics::ModelPtr roficom )
{
    assert( roficom );
    assert( isRofiCoM( roficom ) );

    auto linkPtr = roficom->GetLink( "connectionLink" );
    if ( !linkPtr )
    {
        linkPtr = roficom->GetLink( "RoFICoM::connectionLink" );
    }
    if ( !linkPtr )
    {
        for ( auto link : roficom->GetLinks() )
        {
            auto name = link->GetName();
            if ( name.size() < 16 )
            {
                continue;
            }
            if ( name.compare( name.size() - 16, 16, "::connectionLink" ) == 0 )
            {
                if ( linkPtr )
                {
                    gzwarn << "Found two extendJoints: " << linkPtr->GetName() << ", " << link->GetName() << "\n";
                    break;
                }
                linkPtr = std::move( link );
            }
        }
    }
    return linkPtr;
}

physics::JointPtr RoFICoMPlugin::getConnectionJoint( physics::LinkPtr otherConnectionLink ) const
{
    assert( !connectionJoint );
    assert( thisConnectionLink );
    assert( otherConnectionLink );
    assert( thisConnectionLink != otherConnectionLink );

    if ( !extendJoint->AreConnected( thisConnectionLink, otherConnectionLink ) )
    {
        return {};
    }

    auto outerConnection = otherConnectionLink->GetModel()->GetJoint( "outerConnection" );

    if ( outerConnection )
    {
        auto parentLink = outerConnection->GetParent();
        auto childLink = outerConnection->GetChild();

        if ( parentLink == thisConnectionLink && childLink == otherConnectionLink )
        {
            return outerConnection;
        }
        if ( parentLink == otherConnectionLink && childLink == thisConnectionLink )
        {
            return outerConnection;
        }
    }
    return outerConnection;
}

GZ_REGISTER_MODEL_PLUGIN( RoFICoMPlugin )

} // namespace gazebo
