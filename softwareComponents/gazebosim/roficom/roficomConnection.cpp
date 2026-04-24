#include "roficomConnection.hpp"

#include <cassert>

#include "roficomPlugin.hpp"

namespace gazebo
{
void RoficomConnection::load( RoFICoMPlugin & roficomPlugin,
                              gz::sim::Entity model,
                              const gz::sim::EntityComponentManager & ecm,
                              const rofi::gz::NodePtr & node )
{
    _roficomPlugin = &roficomPlugin;
    _model = model;
    _node = node;

    assert( _roficomPlugin );
    assert( _model != gz::sim::kNullEntity );
    assert( _node );

    _scopedName = GetScopedName( _model, ecm );
    _worldScopedName = getElemPath( gz::sim::worldEntity( _model, ecm ), ecm );

    _pubToOther = _node->Advertise< rofi::messages::Packet >( "~/packets" );
    _pubAttachEvent =
            _node->Advertise< rofi::messages::ConnectorAttachInfo >( "/gazebo/" + _worldScopedName
                                                                     + "/attach_event" );
    _subAttachEvent =
            _node->Subscribe( "~/attach_event", &RoficomConnection::onAttachEvent, this );

    assert( _pubToOther );
    assert( _pubAttachEvent );
    assert( _subAttachEvent );
}

bool RoficomConnection::isConnected() const
{
    std::lock_guard lock( _mutex );
    return _orientation.has_value() && !_connectedToName.empty() && static_cast< bool >( _subToOther );
}

std::optional< RoficomConnection::Orientation > RoficomConnection::getOrientation() const
{
    std::lock_guard lock( _mutex );
    return _orientation;
}

void RoficomConnection::connectRequest( gz::sim::Entity other,
                                        Orientation orientation,
                                        const gz::sim::EntityComponentManager & ecm )
{
    if ( isConnected() )
    {
        return;
    }

    rofi::messages::ConnectorAttachInfo msg;
    msg.set_modelname1( _scopedName );
    msg.set_modelname2( GetScopedName( other, ecm ) );
    msg.set_attach( true );
    msg.set_orientation( orientation );
    _pubAttachEvent->Publish( msg, true );
}

void RoficomConnection::disconnectRequest()
{
    std::lock_guard lock( _mutex );
    if ( !_orientation )
    {
        return;
    }

    rofi::messages::ConnectorAttachInfo msg;
    msg.set_modelname1( _scopedName );
    msg.set_modelname2( _connectedToName );
    msg.set_attach( false );
    _pubAttachEvent->Publish( msg, true );
}

void RoficomConnection::sendPacket( const rofi::messages::Packet & packet )
{
    _pubToOther->Publish( packet );
}

void RoficomConnection::processPending( const gz::sim::EntityComponentManager & )
{
    std::vector< rofi::messages::Packet > packets;
    std::vector< rofi::messages::ConnectorAttachInfo > attachInfos;
    {
        std::lock_guard lock( _mutex );
        packets.swap( _pendingPackets );
        attachInfos.swap( _pendingAttachInfos );
    }

    for ( const auto & packet : packets )
    {
        _roficomPlugin->onPacket( packet );
    }

    for ( const auto & attachInfo : attachInfos )
    {
        std::string otherRoficomName;
        if ( attachInfo.modelname1() == _scopedName )
        {
            otherRoficomName = attachInfo.modelname2();
        }
        else if ( attachInfo.modelname2() == _scopedName )
        {
            otherRoficomName = attachInfo.modelname1();
        }
        else
        {
            continue;
        }

        if ( attachInfo.attach() )
        {
            onConnectEvent( otherRoficomName, attachInfo.orientation() );
            _roficomPlugin->onConnectorEvent( rofi::messages::ConnectorCmd::CONNECT );
        }
        else
        {
            onDisconnectEvent( otherRoficomName );
            _roficomPlugin->onConnectorEvent( rofi::messages::ConnectorCmd::DISCONNECT );
        }
    }
}

void RoficomConnection::onPacket( const PacketPtr & packet )
{
    if ( !packet )
    {
        return;
    }

    std::lock_guard lock( _mutex );
    _pendingPackets.push_back( *packet );
}

void RoficomConnection::onAttachEvent( const AttachInfoPtr & attachInfo )
{
    if ( !attachInfo )
    {
        return;
    }

    std::lock_guard lock( _mutex );
    _pendingAttachInfos.push_back( *attachInfo );
}

void RoficomConnection::onConnectEvent( const std::string & otherRoficomName,
                                        RoficomConnection::Orientation orientation )
{
    std::lock_guard lock( _mutex );

    if ( _orientation )
    {
        disconnect();
    }

    _connectedToName = otherRoficomName;
    _orientation = orientation;
    startCommunication( _connectedToName );
}

void RoficomConnection::onDisconnectEvent( const std::string & otherRoficomName )
{
    std::lock_guard lock( _mutex );
    if ( !_orientation )
    {
        return;
    }
    if ( otherRoficomName != _connectedToName )
    {
        gzerr << "Should disconnect with " << otherRoficomName << ", but was connected to "
              << _connectedToName << "\n";
        return;
    }

    disconnect();
}

void RoficomConnection::disconnect()
{
    if ( _subToOther )
    {
        _subToOther->Unsubscribe();
        _subToOther = {};
    }
    _orientation.reset();
    _connectedToName.clear();
}

void RoficomConnection::startCommunication( const std::string & otherRoficomName )
{
    const auto path = "/gazebo/" + scopedNameToPath( otherRoficomName ) + "/packets";
    _subToOther = _node->Subscribe( path, &RoficomConnection::onPacket, this );
}

void RoficomConnection::connectToNearbyRequest( const gz::sim::EntityComponentManager & ecm )
{
    if ( isConnected() )
    {
        return;
    }

    std::optional< Orientation > orientation;
    gz::sim::Entity otherRoficom = gz::sim::kNullEntity;

    {
        std::lock_guard< std::recursive_mutex > lock( RoFICoMPlugin::positionMutex );
        for ( auto [ candidate, position ] : RoFICoMPlugin::positionsMap )
        {
            if ( candidate == _model || position != RoFICoMPosition::Extended )
            {
                continue;
            }

            orientation = canBeConnected( candidate, ecm );
            if ( orientation )
            {
                otherRoficom = candidate;
                break;
            }
        }
    }

    if ( orientation && otherRoficom != gz::sim::kNullEntity )
    {
        connectRequest( otherRoficom, *orientation, ecm );
    }
}

std::optional< RoficomConnection::Orientation > RoficomConnection::canBeConnected(
        gz::sim::Entity roficom,
        const gz::sim::EntityComponentManager & ecm ) const
{
    auto thisInner = getLinkByName( _model, ecm, "inner" );
    auto otherInner = getLinkByName( roficom, ecm, "inner" );
    if ( !thisInner || !otherInner )
    {
        return {};
    }

    gz::sim::Link thisLink( *thisInner );
    gz::sim::Link otherLink( *otherInner );
    auto thisPose = thisLink.WorldPose( ecm );
    auto otherPose = otherLink.WorldPose( ecm );
    if ( !thisPose || !otherPose )
    {
        return {};
    }

    return canRoficomBeConnected( *thisPose, *otherPose );
}

} // namespace gazebo
