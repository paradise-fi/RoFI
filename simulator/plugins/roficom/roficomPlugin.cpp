#include "roficomPlugin.hpp"

#include "roficomUtils.hpp"

#include <cassert>
#include <cmath>

namespace gazebo
{
std::mutex RoFICoMPlugin::positionsMapMutex;
std::map< const physics::Model *, RoFICoMPlugin::Position > RoFICoMPlugin::positionsMap;

void RoFICoMPlugin::Load( physics::ModelPtr model, sdf::ElementPtr /*sdf*/ )
{
    _model = std::move( model );
    assert( _model );
    gzmsg << "The RoFICoM plugin is attached to model [" << _model->GetScopedName() << "]\n";

    loadJoint();
    assert( extendJoint && *extendJoint );

    thisConnectionLink = getConnectionLink( _model );
    assert( thisConnectionLink );

    updatePosition( Position::Retracted );

    initCommunication();
    initSensorCommunication();

    onUpdateConnection = event::Events::ConnectWorldUpdateBegin( std::bind( &RoFICoMPlugin::onUpdate, this ) );

    gzmsg << "RoFICoM plugin ready\n";
}

void RoFICoMPlugin::loadJoint()
{
    const auto callback = [ this ]( double desiredPosition )
        {
            assert( this->extendJoint );
            assert( *this->extendJoint );

            std::lock_guard< std::mutex > lock( this->connectionMutex );

            if ( std::abs( desiredPosition - this->extendJoint->minPosition )
                    <= this->extendJoint->positionPrecision )
            {
                this->updatePosition( Position::Retracted );
            }
            else if ( std::abs( desiredPosition - this->extendJoint->maxPosition )
                    <= this->extendJoint->positionPrecision )
            {
                this->updatePosition( Position::Extended );
            }
            else
            {
                gzwarn << "Position of extend joint reached, but is not a limit\n";
            }
        };

    auto pidValuesVector = PIDLoader::loadControllerValues(
            getPluginSdf( _model->GetSDF(), "libroficomPlugin.so" ) );
    assert( pidValuesVector.size() == 1 && "expected 1 controlled joint" );

    auto joint = _model->GetJoint( pidValuesVector.at( 0 ).jointName );
    assert( joint && "no joint with specified name found" );

    extendJoint = std::make_unique< JointData< PIDController > >(
                        std::move( joint ),
                        std::move( pidValuesVector.at( 0 ) ),
                        callback );
    assert( extendJoint );
    assert( *extendJoint );
    assert( extendJoint->joint->GetMsgType() == msgs::Joint::PRISMATIC );
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
    updatePosition( Position::Extending );
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
    updatePosition( Position::Retracting );
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
    updateConnection();
}

void RoFICoMPlugin::extend()
{
    assert( extendJoint );
    extendJoint->pid.setTargetPositionWithSpeed( extendJoint->getMaxPosition(), extendJoint->getMaxVelocity() );
}

void RoFICoMPlugin::retract()
{
    assert( extendJoint );
    extendJoint->pid.setTargetPositionWithSpeed( extendJoint->getMinPosition(), extendJoint->getMaxVelocity() );
}

void RoFICoMPlugin::updateConnection()
{
    if ( !isConnected() )
    {
        return;
    }

    if ( position == Position::Extended || position == Position::Extending )
    {
        assert( connectionJoint );
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
    if ( !isConnected() )
    {
        gzwarn << "Ending not existing connection (" << _model->GetScopedName() << ")\n";
    }

    if ( connectedWith )
    {
        gzmsg << "Ending connection (" << _model->GetScopedName() << ") with " << connectedWith->GetScopedName() << "\n";
    }

    connectedWith.reset();
    if ( connectionJoint )
    {
        connectionJoint->Detach();
        connectionJoint.reset();
    }
    if ( _subOutside )
    {
        _subOutside->Unsubscribe();
        _subOutside.reset();
    }
}

void RoFICoMPlugin::updatePosition( Position newPosition )
{
    assert( _model );
    std::lock_guard< std::mutex > lock( positionsMapMutex );
    position = newPosition;
    positionsMap[ _model.get() ] = position;
}

RoFICoMPlugin::Position RoFICoMPlugin::getOtherPosition( physics::ModelPtr roficom )
{
    assert( roficom );
    assert( isRoFICoM( roficom ) );
    std::lock_guard< std::mutex > lock( positionsMapMutex );
    return positionsMap[ roficom.get() ];
}

void RoFICoMPlugin::createConnection( physics::LinkPtr otherConnectionLink, RoFICoMPlugin::Orientation newOrientation )
{
    assert( thisConnectionLink );
    assert( otherConnectionLink );
    assert( thisConnectionLink != otherConnectionLink );
    assert( isRoFICoM( otherConnectionLink->GetModel() ) );
    if ( isConnected() )
    {
        gzwarn << _model->GetScopedName() << " was already connected\n";
    }

    gzmsg << "Create connection with " << otherConnectionLink->GetScopedName() << " (" << thisConnectionLink->GetScopedName() << ")\n";

    connectedWith = otherConnectionLink;
    orientation = newOrientation;
    startCommunication( otherConnectionLink->GetModel() );

    connectionJoint = getOtherConnectionJoint( otherConnectionLink );

    if ( connectionJoint )
    {
        gzmsg << "Found existing connection(" << _model->GetScopedName() << ")\n";

        assert( connectionJoint->AreConnected( thisConnectionLink, connectedWith ) );
        return;
    }

    gzmsg << "Creating new connection joint (" << _model->GetName() << ")\n";

    auto thisWorldPose = thisConnectionLink->WorldPose();
    auto otherWorldPose = otherConnectionLink->WorldPose();

    auto thisNewWorldPose = otherWorldPose;
    thisNewWorldPose.Pos() = thisNewWorldPose.CoordPositionAdd( { 0, 0, extendJoint->joint->Position() } );

    // TODO rotate according to orientation
    thisNewWorldPose.Rot() *= getThisToOtherRotation( newOrientation );
    thisNewWorldPose.Rot() *= ignition::math::Quaterniond( { 1, 0, 0 }, ignition::math::Angle::Pi.Radian() );
    std::cout << "This model new world pose: " << thisNewWorldPose << "\n";
    _model->SetWorldPose( thisNewWorldPose );
    _model->Update();

    connectionJoint = _model->GetJoint( "outerConnection" );
    if ( connectionJoint )
    {
        assert( connectionJoint->GetMsgType() == msgs::Joint::FIXED );
        connectionJoint->Reset();
        connectionJoint->Attach( thisConnectionLink, otherConnectionLink );
        assert( connectionJoint->AreConnected( thisConnectionLink, otherConnectionLink ) );
    }
    else
    {
        connectionJoint = _model->CreateJoint( "outerConnection", "fixed", thisConnectionLink, otherConnectionLink );
        // connectionJoint = _model->GetWorld()->Physics()->CreateJoint( "fixed", _model );
        // connectionJoint->Attach( thisConnectionLink, otherConnectionLink );
        connectionJoint->Init();
    }

    assert( connectionJoint );
    assert( connectionJoint->AreConnected( thisConnectionLink, connectedWith ) );
}

void RoFICoMPlugin::startCommunication( physics::ModelPtr otherModel )
{
    assert( otherModel );
    assert( !_subOutside && "already has subsriber" );

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
        assert( !isConnected() );
        auto otherModel = getModelOfOther( contact );
        if ( otherModel == _model )
        {
            break;
        }

        if ( !isRoFICoM( otherModel ) )
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
            connectionJoint = getOtherConnectionJoint( otherLink );
            if ( !connectionJoint )
            {
                continue;
            }

            auto newOrientation = getConnectorOrientation( _model->WorldPose(), otherModel->WorldPose() );
            if ( !newOrientation )
            {
                gzerr << "Could not get mutual orientation with " << otherModel->GetScopedName() << " (" << _model->GetScopedName() << ")\n";
                continue;
            }

            createConnection( otherLink, *newOrientation );
            assert( isConnected() );
            return;
        }
        else
        {
            assert( position == Position::Extending );

            if ( auto orientation = canBeConnected( otherLink ) )
            {
                gzmsg << "Connecting with " << otherLink->GetModel()->GetScopedName() << " (" << _model->GetScopedName() << ")\n";

                createConnection( otherLink, *orientation );
                assert( isConnected() );
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

std::optional< RoFICoMPlugin::Orientation > RoFICoMPlugin::canBeConnected( physics::LinkPtr otherConnectionLink ) const
{
    assert( thisConnectionLink );
    assert( otherConnectionLink );
    assert( thisConnectionLink != otherConnectionLink );
    assert( isRoFICoM( otherConnectionLink->GetModel() ) );

    Position otherPosition = getOtherPosition( otherConnectionLink->GetModel() );
    if ( otherPosition == Position::Retracted || otherPosition == Position::Retracting )
    {
        return {};
    }

    return canRoficomBeConnected( thisConnectionLink->WorldPose(), otherConnectionLink->WorldPose() );
}

physics::LinkPtr RoFICoMPlugin::getConnectionLink( physics::ModelPtr roficom )
{
    assert( roficom );
    assert( isRoFICoM( roficom ) );

    auto linkPtr = roficom->GetLink( "inner" );
    if ( !linkPtr )
    {
        linkPtr = roficom->GetLink( "RoFICoM::inner" );
    }
    if ( !linkPtr )
    {
        for ( auto link : roficom->GetLinks() )
        {
            auto name = link->GetName();
            if ( name.size() < 7 )
            {
                continue;
            }
            if ( name.compare( name.size() - 7, 7, "::inner" ) == 0 )
            {
                if ( linkPtr )
                {
                    gzwarn << "Found two inner links: " << linkPtr->GetName() << ", " << link->GetName() << "\n";
                    break;
                }
                linkPtr = std::move( link );
            }
        }
    }
    return linkPtr;
}

physics::JointPtr RoFICoMPlugin::getExtendJoint( physics::ModelPtr roficom )
{
    assert( roficom );
    assert( isRoFICoM( roficom ) );

    auto extendJoint = roficom->GetJoint( "extendJoint" );
    if ( !extendJoint )
    {
        extendJoint = roficom->GetJoint( "RoFICoM::extendJoint" );
    }
    if ( !extendJoint )
    {
        for ( auto joint : roficom->GetJoints() )
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
    return extendJoint;
}

physics::JointPtr RoFICoMPlugin::getOtherConnectionJoint( physics::LinkPtr otherConnectionLink ) const
{
    assert( !connectionJoint );
    assert( thisConnectionLink );
    assert( otherConnectionLink );
    assert( thisConnectionLink != otherConnectionLink );

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
    return {};
}

GZ_REGISTER_MODEL_PLUGIN( RoFICoMPlugin )

} // namespace gazebo
