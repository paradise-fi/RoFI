#include "roficomConnection.hpp"

#include <cassert>

#include "roficomPlugin.hpp"
#include "roficomConnect.hpp"



namespace gazebo
{

void RoficomConnection::load( RoFICoMPlugin & roficomPlugin, physics::ModelPtr model, transport::NodePtr node )
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
}

bool RoficomConnection::isConnected() const
{
    bool connected = getOrientation().has_value();
    assert( connected == bool( _connectedTo ) );
    assert( connected == bool( _subToOther ) );

    // TODO checks

    return connected;
}

void RoficomConnection::disconnect()
{
    assert( _model );

    if ( _subToOther )
    {
        _subToOther->Unsubscribe();
        _subToOther.reset();
    }

    // TODO disconnect

    _orientation = {};
    _connectedTo = {};

    assert( !isConnected() );
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

void RoficomConnection::connect( physics::ModelPtr other, RoficomConnection::Orientation orientation )
{
    assert( _model );
    assert( other );

    gzmsg << "Connecting RoFICoM " << _model->GetScopedName() << " to " << other->GetScopedName() << "\n";

    _orientation = orientation;

    // TODO connect to other

    startCommunication( other );

    assert( isConnected() );
}

void RoficomConnection::startCommunication( physics::ModelPtr otherModel )
{
    assert( _roficomPlugin );
    assert( otherModel );
    assert( !_subToOther && "already has subsriber" );

    auto path = "/gazebo/" + getElemPath( otherModel ) + "/packets";
    _subToOther = _node->Subscribe( path, &RoficomConnection::onPacket, this );
}

void RoficomConnection::checkConnection( RoFICoMPosition position )
{
    if ( !isConnected() )
    {
        // TODO detection if someone connected to me
        return;
    }

    if ( position == RoFICoMPosition::Extended || position == RoFICoMPosition::Extending )
    {
        if ( false /* TODO Connection does not last */ )
        {
            disconnect();
            assert( !isConnected() );
            return;
        }
    }

    if ( position == RoFICoMPosition::Retracted || position == RoFICoMPosition::Retracting )
    {
        disconnect();
        assert( !isConnected() );
        return;
    }
}

void RoficomConnection::connectToNearby()
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

        assert( RoFICoMPlugin::positionsMap[ _model.get() ] == RoFICoMPosition::Extended );

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
        connect( otherRoficom, *orientation );
    }
}

std::optional< RoficomConnection::Orientation > RoficomConnection::canBeConnected( physics::ModelPtr roficom ) const
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

/* TODO check other's position
    assert( position == Position::Extended );

    if ( getOtherPosition( otherInnerLink->GetModel() ) != Position::Extended )
    {
        return {};
    }
*/

    return canRoficomBeConnected( thisInnerLink->WorldPose(), otherInnerLink->WorldPose() );
}

} // namespace gazebo
