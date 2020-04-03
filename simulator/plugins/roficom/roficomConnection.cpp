#include "roficomConnection.hpp"

#include <cassert>

#include "roficomConnect.hpp"
#include "roficomPlugin.hpp"


namespace gazebo
{
void RoficomConnection::load( RoFICoMPlugin & roficomPlugin,
                              physics::ModelPtr model,
                              transport::NodePtr node )
{
    _roficomPlugin = &roficomPlugin;
    assert( _roficomPlugin );

    _model = std::move( model );
    assert( _model );


    _node = std::move( node );
    assert( _node );
    assert( _node->IsInitialized() );

    _pubToOther = _node->Advertise< rofi::messages::Packet >( "~/packets" );
    assert( _pubToOther );

    auto attachEventPath = "/gazebo/" + _model->GetWorld()->Name() + "/attach_event";
    _pubAttachEvent = _node->Advertise< rofi::messages::ConnectorAttachInfo >( attachEventPath );
    assert( _pubAttachEvent );
    gzmsg << "Advertising on topic: " << _pubAttachEvent->GetTopic() << "\n";

    _subAttachEvent = _node->Subscribe( "~/attach_event", &RoficomConnection::onAttachEvent, this );
    assert( _subAttachEvent );
}

bool RoficomConnection::isConnected() const
{
    bool connected = getOrientation().has_value();
    assert( connected == !_connectedToName.empty() );
    assert( connected == bool( _connectedTo ) );
    assert( connected == bool( _subToOther ) );

    return connected;
}

void RoficomConnection::connectRequest( physics::ModelPtr other,
                                        RoficomConnection::Orientation orientation )
{
    assert( _model );
    assert( _pubAttachEvent );
    assert( other );

    if ( isConnected() )
    {
        return;
    }

    rofi::messages::ConnectorAttachInfo msg;
    msg.set_modelname1( _model->GetName() );
    msg.set_modelname2( other->GetName() );
    msg.set_attach( true );
    msg.set_orientation( orientation );

    gzmsg << "Publishing to " << _pubAttachEvent->GetTopic() << "\n";
    gzmsg << "Message\n" << msg.DebugString() << "\n";
    _pubAttachEvent->Publish( msg, true );
}

void RoficomConnection::disconnectRequest()
{
    assert( _model );
    assert( _pubAttachEvent );

    if ( !isConnected() )
    {
        return;
    }

    assert( _connectedTo );

    rofi::messages::ConnectorAttachInfo msg;
    msg.set_modelname1( _model->GetName() );
    msg.set_modelname1( _connectedTo->GetName() );
    msg.set_attach( false );

    _pubAttachEvent->Publish( msg, true );
}

void RoficomConnection::sendPacket( const rofi::messages::Packet & packet )
{
    assert( _pubToOther );

    _pubToOther->Publish( packet );
}

void RoficomConnection::onPacket( const RoficomConnection::PacketPtr & packet )
{
    assert( _roficomPlugin );

    assert( packet );
    if ( !packet )
    {
        return;
    }

    _roficomPlugin->onPacket( *packet );
}

void RoficomConnection::onAttachEvent( const RoficomConnection::AttachInfoPtr & attachInfo )
{
    std::string otherRoficomName;
    if ( _model->GetName() == attachInfo->modelname1() )
    {
        otherRoficomName = attachInfo->modelname2();
    }
    else if ( _model->GetName() == attachInfo->modelname2() )
    {
        otherRoficomName = attachInfo->modelname1();
    }
    else
    {
        gzwarn << "Roficom not one of attached/dettached roficoms\n";
        return;
    }

    if ( attachInfo->attach() )
    {
        onConnectEvent( otherRoficomName, attachInfo->orientation() );
    }
    else
    {
        onDisconnectEvent( otherRoficomName );
    }
}

void RoficomConnection::onConnectEvent( const std::string & otherRoficomName,
                                        RoficomConnection::Orientation orientation )
{
    assert( _model );

    auto otherRoficom = _model->GetWorld()->ModelByName( otherRoficomName );
    if ( !otherRoficom )
    {
        gzerr << "Could not find model " << otherRoficomName << "\n";
        return;
    }
    if ( !isRoFICoM( otherRoficom ) )
    {
        gzerr << "Model " << otherRoficom->GetScopedName() << " (got name: " << otherRoficomName
              << ") is not RoFICoM\n";
        return;
    }

    if ( isConnected() )
    {
        disconnect();
    }

    _connectedToName = otherRoficomName;
    _connectedTo = std::move( otherRoficom );
    _orientation = orientation;

    startCommunication( _connectedTo );

    gzmsg << "Connected RoFICoM " << _model->GetScopedName() << " to "
          << _connectedTo->GetScopedName() << "\n";

    assert( isConnected() );
}

void RoficomConnection::onDisconnectEvent( const std::string & otherRoficomName )
{
    if ( !isConnected() )
    {
        gzerr << "Disconnecting, but wasn't connected\n";
        return;
    }

    if ( otherRoficomName != _connectedToName )
    {
        gzerr << "Should disconnect with " << otherRoficomName << ", but was connected to "
              << _connectedToName << "\n";
        return;
    }

    disconnect();
    assert( !isConnected() );
}

void RoficomConnection::disconnect()
{
    assert( _model );

    if ( _subToOther )
    {
        _subToOther->Unsubscribe();
        _subToOther = {};
    }

    _orientation = {};
    _connectedToName = {};
    _connectedTo = {};

    assert( !isConnected() );
}

void RoficomConnection::startCommunication( physics::ModelPtr otherModel )
{
    assert( _roficomPlugin );
    assert( otherModel );
    assert( !_subToOther );

    auto path = "/gazebo/" + getElemPath( otherModel ) + "/packets";
    _subToOther = _node->Subscribe( path, &RoficomConnection::onPacket, this );
}

void RoficomConnection::connectToNearbyRequest()
{
    assert( _model );

    if ( isConnected() )
    {
        return;
    }

    gzmsg << "Searching for nearby extended roficoms (" << _model->GetScopedName() << ")\n";

    // TODO search only nearby roficoms and remove const_cast
    physics::ModelPtr otherRoficom;
    std::optional< Orientation > orientation;

    {
        std::lock_guard< std::recursive_mutex > lock( RoFICoMPlugin::positionMutex );

        assert( RoFICoMPlugin::getOtherPosition( _model ) == RoFICoMPosition::Extended );

        for ( auto pair : RoFICoMPlugin::positionsMap )
        {
            if ( pair.second != RoFICoMPosition::Extended )
            {
                continue;
            }

            otherRoficom = const_cast< physics::Model * >( pair.first )->shared_from_this();

            if ( otherRoficom == _model )
            {
                continue;
            }

            assert( otherRoficom );
            assert( isRoFICoM( otherRoficom ) );

            if ( orientation = canBeConnected( otherRoficom ) )
            {
                break;
            }
        }
    }

    if ( orientation )
    {
        assert( otherRoficom );
        connectRequest( otherRoficom, *orientation );
    }
}

std::optional< RoficomConnection::Orientation > RoficomConnection::canBeConnected(
        physics::ModelPtr roficom ) const
{
    assert( _model );
    assert( roficom );
    assert( isRoFICoM( roficom ) );
    assert( roficom != _model );

    auto thisInnerLink = getLinkByName( _model, "inner" );
    auto otherInnerLink = getLinkByName( roficom, "inner" );

    assert( thisInnerLink );
    assert( otherInnerLink );
    assert( thisInnerLink != otherInnerLink );

    assert( RoFICoMPlugin::getOtherPosition( roficom ) == RoFICoMPosition::Extended );

    return canRoficomBeConnected( thisInnerLink->WorldPose(), otherInnerLink->WorldPose() );
}

} // namespace gazebo
