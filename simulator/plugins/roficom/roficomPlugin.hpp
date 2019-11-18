#include <gazebo/gazebo.hh>
#include <gazebo/common/Events.hh>
#include <gazebo/physics/physics.hh>

#include <mutex>

#include <connectorCmd.pb.h>
#include <connectorResp.pb.h>

namespace gazebo
{

class RoFICoMPlugin : public ModelPlugin
{
public:
    static constexpr double maxJointSpeed = 0.012; // [m/s]
    static constexpr double maxJointForce = 1.5; // [Nm]
    static constexpr double positionPrecision = 0.0001;

    using PacketPtr = boost::shared_ptr< const rofi::messages::Packet >;

    enum class Position : signed char
    {
        Retracted = 0,
        Extending = 1,
        Extended = 2,
        Connected = 3,
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
    void onExtended();
    // Called while holding connectionMutex
    void onRetracted();
    // Called while holding connectionMutex
    void checkConnection();
    // Called while holding connectionMutex
    void endConnection();

    void onConnectorCmd( const ConnectorCmdPtr & msg );
    void onSensorMessage( const ContactsMsgPtr & contacts );

    physics::ModelPtr getModelOfOther( const msgs::Contact & ) const;
    physics::CollisionPtr getCollisionByScopedName( const std::string & collisionName ) const;
    rofi::messages::ConnectorResp getConnectorResp( rofi::messages::ConnectorCmd::Type cmdtype ) const;

    bool canBeConnected( physics::ModelPtr otherConnector ) const;

    physics::ModelPtr _model;

    transport::NodePtr _node;
    transport::PublisherPtr _pubRofi;
    transport::SubscriberPtr _subRofi;
    transport::PublisherPtr _pubOutside;
    transport::SubscriberPtr _subOutside;
    transport::SubscriberPtr _subSensor;

    physics::JointPtr extendJoint;
    double currentVelocity = 0.0;
    physics::JointPtr connection;

    int connectorNumber = 0;

    event::ConnectionPtr onUpdateConnection;
    std::mutex connectionMutex;
    Position position = Position::Retracted;
    rofi::messages::ConnectorState::Orientation orientation{};
};

} // namespace gazebo
