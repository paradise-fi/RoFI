#include <gazebo/gazebo.hh>
#include <gazebo/common/Events.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/sensors/sensors.hh>
#include <gazebo/transport/transport.hh>

#include <array>
#include <limits>
#include <utility>

#include <connectorCmd.pb.h>
#include <connectorResp.pb.h>

namespace gazebo
{

class ConnectorPlugin : public ModelPlugin
{
public:
    static constexpr double speed = 1.5; // [m/s]
    static constexpr double doublePrecision = 0.01;

    using PacketPtr = boost::shared_ptr< const rofi::messages::Packet >;

    ConnectorPlugin()
        : _node( boost::make_shared< transport::Node >() ),
        _sensorNode( boost::make_shared< transport::Node >() )
    {}

    ~ConnectorPlugin()
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

    rofi::messages::ConnectorResp getConnectorResp( rofi::messages::ConnectorCmd::Type cmdtype ) const;

    physics::ModelPtr _model;

    transport::NodePtr _node;
    transport::PublisherPtr _pubRofi;
    transport::SubscriberPtr _subRofi;
    transport::PublisherPtr _pubOutside;
    transport::SubscriberPtr _subOutside;

    transport::NodePtr _sensorNode;
    transport::SubscriberPtr _sensorSub;

    physics::JointPtr connection;

    int connectorNumber = 0;
};

} // namespace gazebo
