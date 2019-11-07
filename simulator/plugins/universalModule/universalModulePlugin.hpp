#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>

#include <utility>

#include <rofiCmd.pb.h>

namespace gazebo {

class UniversalModulePlugin : public ModelPlugin {
public:
    const double maxJointTorque = 10; // Max torque of joint motors
    const std::pair< double, double > jointSpeedBoundaries = { 0, 10 }; // Max speed of joint motors
    const std::pair< double, double > jointPositionBoundaries = { 0, 10 };

    UniversalModulePlugin() :
            _pid( 0.1, 0, 0 ),
            _node( boost::make_shared< transport::Node >() )
    {}

    virtual void Load( physics::ModelPtr model, sdf::ElementPtr sdf );

private:
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;

    void onRofiCmd( const RofiCmdPtr &msg);
    void onJointCmd( const rofi::messages::JointCmd &msg );


    physics::ModelPtr _model;
    common::PID _pid;
    transport::NodePtr _node;
    transport::SubscriberPtr _sub;
    transport::PublisherPtr _pub;
};

} // namespace gazebo