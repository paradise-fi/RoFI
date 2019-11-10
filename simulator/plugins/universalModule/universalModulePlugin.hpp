#include <gazebo/gazebo.hh>
#include <gazebo/common/Events.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>

#include <array>
#include <limits>
#include <utility>

#include <rofiCmd.pb.h>

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

    static constexpr int numOfJoints = 3;
    static constexpr double doublePrecision = 0.01;

    UniversalModulePlugin() :
            _pid( 0.1, 0, 0 ),
            _node( boost::make_shared< transport::Node >() )
    {}

    // Expects that first #numOfJoints gazebo joints of @model are RoFI Joints
    // and next #numOfConnectors gazebo joints of @model are RoFI Connectors
    virtual void Load( physics::ModelPtr model, sdf::ElementPtr sdf );

private:
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;

    void onRofiCmd( const RofiCmdPtr &msg);
    void onJointCmd( const rofi::messages::JointCmd &msg );

    void setVelocity( int joint, double velocity );
    void setTorque( int joint, double torque );
    void setPositionWithSpeed( int joint, double desiredPosition, double speed );

    void setPositionCheck( int joint, double position, double desiredPosition );

    physics::ModelPtr _model;
    common::PID _pid;
    transport::NodePtr _node;
    transport::SubscriberPtr _sub;
    transport::PublisherPtr _pub;

    std::array< event::ConnectionPtr, numOfJoints > setPositionHandle;
};

} // namespace gazebo