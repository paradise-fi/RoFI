#pragma once

#include <gazebo/gazebo.hh>
#include <gazebo/common/Events.hh>
#include <gazebo/physics/physics.hh>

#include <memory>
#include <mutex>
#include <optional>

#include <connectorCmd.pb.h>
#include <connectorResp.pb.h>

#include "utils.hpp"
#include "pidController.hpp"
#include "pidLoader.hpp"
#include "roficomUtils.hpp"
#include "roficomConnection.hpp"


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
    void loadJoint();

    void connect();
    void disconnect();
    void sendPacket( const rofi::messages::Packet & packet );
    void onPacket( const rofi::messages::Packet & packet );

    bool isConnected() const;
    std::optional< Orientation > getOrientation() const;

    void updatePosition( Position newPosition );
    Position getPosition() const;
    static Position getOtherPosition( physics::ModelPtr roficom );

private:
    using ConnectorCmdPtr = boost::shared_ptr< const rofi::messages::ConnectorCmd >;
    using ContactsMsgPtr = boost::shared_ptr< const msgs::Contacts >;

    void initCommunication();

    void extend();
    void retract();

    void onConnectorCmd( const ConnectorCmdPtr & msg );

    rofi::messages::ConnectorResp getConnectorResp( rofi::messages::ConnectorCmd::Type cmdtype ) const;

    void jointPositionReachedCallback( double desiredPosition );

    void removePosition();

    physics::ModelPtr _model;

    transport::NodePtr _node;
    transport::PublisherPtr _pubRofi;
    transport::SubscriberPtr _subRofi;

    std::unique_ptr< JointData< PIDController > > extendJoint;
    RoficomConnection roficomConnection;

    int connectorNumber = 0;

public: // TODO remove (only for getting all roficoms)
    static std::recursive_mutex positionMutex;
    static std::map< const physics::Model *, Position > positionsMap;
};

} // namespace gazebo
