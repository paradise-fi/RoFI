#pragma once

#include <array>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <gz/sim/EntityComponentManager.hh>

#include <rofi/gz_transport.hpp>

#include "roficomConnect.hpp"
#include "roficomUtils.hpp"

#include <connectorAttachInfo.pb.h>
#include <connectorCmd.pb.h>
#include <connectorResp.pb.h>

namespace gazebo
{
class RoFICoMPlugin;

class RoficomConnection
{
public:
    using Orientation = rofi::messages::ConnectorState::Orientation;
    using PacketPtr = boost::shared_ptr< const rofi::messages::Packet >;
    using AttachInfoPtr = boost::shared_ptr< const rofi::messages::ConnectorAttachInfo >;

    void load( RoFICoMPlugin & roficomPlugin,
               gz::sim::Entity model,
               const gz::sim::EntityComponentManager & ecm,
               const rofi::gz::NodePtr & node );

    bool isConnected() const;
    std::optional< Orientation > getOrientation() const;

    void connectToNearbyRequest( const gz::sim::EntityComponentManager & ecm );
    void connectRequest( gz::sim::Entity otherRoficom,
                         Orientation orientation,
                         const gz::sim::EntityComponentManager & ecm );
    void disconnectRequest();

    void sendPacket( const rofi::messages::Packet & packet );
    void processPending( const gz::sim::EntityComponentManager & ecm );

private:
    std::optional< Orientation > canBeConnected( gz::sim::Entity roficom,
                                                 const gz::sim::EntityComponentManager & ecm ) const;
    void startCommunication( const std::string & otherRoficomName );
    void disconnect();

    void onPacket( const PacketPtr & packet );
    void onAttachEvent( const AttachInfoPtr & attachInfo );
    void onConnectEvent( const std::string & otherRoficomName, Orientation orientation );
    void onDisconnectEvent( const std::string & otherRoficomName );

    std::optional< Orientation > _orientation;

    RoFICoMPlugin * _roficomPlugin = nullptr;
    gz::sim::Entity _model = gz::sim::kNullEntity;
    std::string _scopedName;
    std::string _worldScopedName;

    rofi::gz::NodePtr _node;
    rofi::gz::PublisherPtr _pubToOther;
    rofi::gz::SubscriberPtr _subToOther;

    rofi::gz::PublisherPtr _pubAttachEvent;
    rofi::gz::SubscriberPtr _subAttachEvent;

    std::string _connectedToName;

    mutable std::mutex _mutex;
    std::vector< rofi::messages::Packet > _pendingPackets;
    std::vector< rofi::messages::ConnectorAttachInfo > _pendingAttachInfos;
};

} // namespace gazebo
