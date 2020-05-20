#pragma once

#include <memory>
#include <mutex>
#include <optional>

#include <gazebo/common/Events.hh>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include "roficomConnection.hpp"
#include "roficomUtils.hpp"
#include "utils.hpp"

#include <connectorCmd.pb.h>
#include <connectorResp.pb.h>


namespace gazebo
{
class RoFICoMPlugin : public ModelPlugin
{
public:
    using Orientation = rofi::messages::ConnectorState::Orientation;
    using Position = RoFICoMPosition;

    RoFICoMPlugin() = default;

    RoFICoMPlugin( const RoFICoMPlugin & ) = delete;
    RoFICoMPlugin & operator=( const RoFICoMPlugin & ) = delete;

    ~RoFICoMPlugin()
    {
        if ( _node )
        {
            _node->Fini();
        }

        removePosition();
    }

    void Load( physics::ModelPtr model, sdf::ElementPtr sdf ) override;
    void loadJoint( sdf::ElementPtr pluginSdf );

    void connect();
    void disconnect();
    void sendPacket( const rofi::messages::Packet & packet );
    void onPacket( const rofi::messages::Packet & packet );
    void onConnectorEvent( rofi::messages::ConnectorCmd::Type eventType );

    bool isConnected() const;
    std::optional< Orientation > getOrientation() const;

    void updatePosition( Position newPosition );
    Position getPosition() const;
    static Position getOtherPosition( physics::ModelPtr roficom );

private:
    using ConnectorCmdPtr = boost::shared_ptr< const rofi::messages::ConnectorCmd >;
    using ContactsMsgPtr = boost::shared_ptr< const msgs::Contacts >;

    void initCommunication();
    void startListening();

    void onConnectorCmd( const ConnectorCmdPtr & msg );

    rofi::messages::ConnectorResp getConnectorResp(
            rofi::messages::ConnectorCmd::Type cmdtype ) const;

    void jointPositionReachedCallback( Position newPosition );

    void removePosition();

    physics::ModelPtr _model;

    transport::NodePtr _node;
    transport::PublisherPtr _pubRofi;
    transport::SubscriberPtr _subRofi;

    std::unique_ptr< JointData< RoficomController > > extendJoint;
    RoficomConnection roficomConnection;

    int connectorNumber = 0;

public: // TODO remove (only for getting all roficoms)
    static std::recursive_mutex positionMutex;
    static std::map< const physics::Model *, Position > positionsMap;
};

} // namespace gazebo
