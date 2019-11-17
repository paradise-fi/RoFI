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
    static constexpr double speed = 1.5; // [m/s]
    static constexpr double doublePrecision = 0.01;

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
        _node->Fini();
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

    void onConnectorCmd( const ConnectorCmdPtr & msg );
    void onSensorMessage( const ContactsMsgPtr & contacts );

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

    physics::JointPtr connection;

    int connectorNumber = 0;

    std::mutex connectionMutex;
    Position position = Position::Retracted;
    rofi::messages::ConnectorState::Orientation orientation{};
};

} // namespace gazebo
