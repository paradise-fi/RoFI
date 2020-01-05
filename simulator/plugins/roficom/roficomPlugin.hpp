#pragma once

#include <gazebo/gazebo.hh>
#include <gazebo/common/Events.hh>
#include <gazebo/physics/physics.hh>

#include <mutex>

#include <connectorCmd.pb.h>
#include <connectorResp.pb.h>

#include "../common/utils.hpp"

namespace gazebo
{

class RoFICoMPlugin : public ModelPlugin
{
public:
    static constexpr double positionPrecision = 0.0001; // [m]
    static constexpr double minConnectionCosAngle = 0.9992; // cos of maximal angle when connection succeeds
    static constexpr double maxConnectionCenterDistance = 0.004; // [m]

    using PacketPtr = boost::shared_ptr< const rofi::messages::Packet >;

    enum class Position : signed char
    {
        Retracted = 0,
        Retracting = 1,
        Extending = 2,
        Extended = 3,
    };

    RoFICoMPlugin() = default;

    RoFICoMPlugin( const RoFICoMPlugin & ) = delete;
    RoFICoMPlugin & operator=( const RoFICoMPlugin & ) = delete;

    ~RoFICoMPlugin()
    {
        if ( _node )
        {
            _node->Fini();
        }
    }

    virtual void Load( physics::ModelPtr model, sdf::ElementPtr sdf );

    void connect();
    void disconnect();
    void sendPacket( const rofi::messages::Packet & packet );
    void onPacket( const PacketPtr & packet );

    bool isConnected() const;

private:
    using ConnectorCmdPtr = boost::shared_ptr< const rofi::messages::ConnectorCmd >;
    using ContactsMsgPtr = boost::shared_ptr< const msgs::Contacts >;

    void initCommunication();
    void initSensorCommunication();

    void onUpdate();

    // Called while holding connectionMutex
    void stop();
    // Called while holding connectionMutex
    void extend();
    // Called while holding connectionMutex
    void retract();

    // Called while holding connectionMutex
    void startCommunication( physics::ModelPtr otherModel );
    // Called while holding connectionMutex
    void createConnection( physics::LinkPtr otherConnectionLink );
    // Called while holding connectionMutex
    void updateConnection();
    // Called while holding connectionMutex
    void endConnection();
    // Called while holding connectionMutex
    void updatePosition( Position newPosition );

    static Position getOtherPosition( physics::ModelPtr roficom );

    void onConnectorCmd( const ConnectorCmdPtr & msg );
    void onSensorMessage( const ContactsMsgPtr & contacts );

    physics::ModelPtr getModelOfOther( const msgs::Contact & ) const;
    physics::CollisionPtr getCollisionByScopedName( const std::string & collisionName ) const;
    rofi::messages::ConnectorResp getConnectorResp( rofi::messages::ConnectorCmd::Type cmdtype ) const;

    bool canBeConnected( physics::LinkPtr otherConnectionLink ) const;
    static physics::LinkPtr getConnectionLink( physics::ModelPtr roficom );
    static physics::JointPtr getExtendJoint( physics::ModelPtr roficom );

    physics::JointPtr getOtherConnectionJoint( physics::LinkPtr otherConnectionLink ) const;

    physics::ModelPtr _model;

    transport::NodePtr _node;
    transport::PublisherPtr _pubRofi;
    transport::SubscriberPtr _subRofi;
    transport::PublisherPtr _pubOutside;
    transport::SubscriberPtr _subOutside;
    transport::SubscriberPtr _subSensor;

    JointData extendJoint;
    physics::LinkPtr thisConnectionLink;
    double currentVelocity = 0.0;
    physics::JointPtr connectionJoint;
    physics::LinkPtr connectedWith;

    int connectorNumber = 0;

    event::ConnectionPtr onUpdateConnection;
    std::mutex connectionMutex;
    Position position = Position::Retracted;
    rofi::messages::ConnectorState::Orientation orientation{};

    static std::mutex positionsMapMutex;
    static std::map< const physics::Model *, Position > positionsMap;
};

} // namespace gazebo
