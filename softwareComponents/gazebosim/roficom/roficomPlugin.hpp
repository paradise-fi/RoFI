#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <gz/sim/Model.hh>
#include <gz/sim/System.hh>

#include <rofi/gz_transport.hpp>

#include "roficomConnection.hpp"
#include "roficomUtils.hpp"

#include <connectorCmd.pb.h>
#include <connectorResp.pb.h>

namespace gazebo
{
class RoFICoMPlugin : public gz::sim::System,
                      public gz::sim::ISystemConfigure,
                      public gz::sim::ISystemPreUpdate
{
public:
    using Orientation = rofi::messages::ConnectorState::Orientation;
    using Position = RoFICoMPosition;
    using ConnectorCmdPtr = boost::shared_ptr< const rofi::messages::ConnectorCmd >;

    void Configure( const gz::sim::Entity & entity,
                    const std::shared_ptr< const sdf::Element > & sdf,
                    gz::sim::EntityComponentManager & ecm,
                    gz::sim::EventManager & eventMgr ) override;

    void PreUpdate( const gz::sim::UpdateInfo & info,
                    gz::sim::EntityComponentManager & ecm ) override;

    void connect( gz::sim::EntityComponentManager & ecm );
    void disconnect( gz::sim::EntityComponentManager & ecm );
    void sendPacket( const rofi::messages::Packet & packet );
    void onPacket( const rofi::messages::Packet & packet );
    void onConnectorEvent( rofi::messages::ConnectorCmd::Type eventType );

    bool isConnected() const;
    std::optional< Orientation > getOrientation() const;

    void updatePosition( Position newPosition );
    Position getPosition() const;
    static Position getOtherPosition( gz::sim::Entity roficom );

    static std::recursive_mutex positionMutex;
    static std::map< gz::sim::Entity, Position > positionsMap;

private:
    void loadJoint( gz::sim::EntityComponentManager & ecm );
    void initCommunication( const gz::sim::EntityComponentManager & ecm );
    void startListening();
    void jointPositionReachedCallback( Position newPosition );
    void onConnectorCmd( const ConnectorCmdPtr & msg );
    void handleConnectorCmd( const rofi::messages::ConnectorCmd & msg,
                             gz::sim::EntityComponentManager & ecm );
    rofi::messages::ConnectorResp getConnectorResp(
            rofi::messages::ConnectorCmd::Type cmdtype ) const;
    void removePosition();

    gz::sim::Entity _entity = gz::sim::kNullEntity;
    gz::sim::Model _model;
    std::shared_ptr< const sdf::Element > _sdf;

    rofi::gz::NodePtr _node;
    rofi::gz::PublisherPtr _pubRofi;
    rofi::gz::SubscriberPtr _subRofi;

    std::unique_ptr< JointData< RoficomController > > extendJoint;
    RoficomConnection roficomConnection;

    int connectorNumber = 0;
    bool _connectToNearbyRequested = false;

    std::mutex _queueMutex;
    std::vector< rofi::messages::ConnectorCmd > _pendingCommands;
};

} // namespace gazebo
