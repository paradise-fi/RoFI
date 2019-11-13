#include <gazebo/gazebo.hh>
#include <gazebo/common/Events.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>

#include <array>
#include <limits>
#include <utility>

#include <rofiCmd.pb.h>
#include <connectorResp.pb.h>

namespace gazebo {

class UniversalModulePlugin : public ModelPlugin {
public:
    using limitPair = std::pair< double, double >;
    static constexpr double maxJointTorque = 1.5; // [Nm]
    static constexpr limitPair jointSpeedBoundaries = { 0.0, 6.545 }; // [rad/s] // 0, 60/0.16 deg/s
    static constexpr std::array< limitPair, 3 > jointPositionBoundaries = { // [rad]
            limitPair( std::numeric_limits< double >::lowest(), std::numeric_limits< double >::max() ),
            limitPair( -1.5708, 1.5708 ), // -90 deg, 90 deg
            limitPair( -1.5708, 1.5708 ) // -90 deg, 90 deg
        };

    static constexpr double doublePrecision = 0.01;

    UniversalModulePlugin() :
            _pid( 0.1, 0, 0 ),
            _node( boost::make_shared< transport::Node >() )
    {}

    ~UniversalModulePlugin()
    {
        _node->Fini();
    }

    virtual void Load( physics::ModelPtr model, sdf::ElementPtr sdf );

private:
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;
    using ConnectorRespPtr = boost::shared_ptr< const rofi::messages::ConnectorResp >;

    void initCommunication();

    void findAndInitJoints();

    // Connectors have to be models, that start with prefix "connector",
    // have all unique names and have functionality of plugin "connectorPlugin.so"
    void addConnector( gazebo::physics::ModelPtr connectorModel );
    void findAndInitConnectors( sdf::ElementPtr sdf );
    void clearConnectors();

    void onRofiCmd( const RofiCmdPtr & msg );
    void onJointCmd( const rofi::messages::JointCmd & msg );
    void onConnectorCmd( const rofi::messages::ConnectorCmd & msg );

    void onConnectorResp( const ConnectorRespPtr & msg );

    void setVelocity( int joint, double velocity );
    void setTorque( int joint, double torque );
    void setPositionWithSpeed( int joint, double desiredPosition, double speed );

    void setPositionCheck( int joint, double position, double desiredPosition );

    physics::ModelPtr _model;
    common::PID _pid;
    transport::NodePtr _node;
    transport::SubscriberPtr _sub;
    transport::PublisherPtr _pub;

    std::vector< std::pair< physics::JointPtr, event::ConnectionPtr > > joints;
    std::vector< std::pair< transport::PublisherPtr, transport::SubscriberPtr > > connectors;
};

} // namespace gazebo
