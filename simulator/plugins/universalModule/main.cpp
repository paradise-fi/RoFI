#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>
#include <iostream>

#include <jointPosCmd.pb.h>
#include <jointVelCmd.pb.h>

namespace gazebo {

using JointVelCmdPtr = const boost::shared_ptr< const rofi::messages::JointVelCmd >;

class UniversalModulePlugin : public ModelPlugin {
public:
    UniversalModulePlugin():
        _pid(0.1, 0, 0),
        _node(new transport::Node())
    {}

    virtual void Load(physics::ModelPtr model, sdf::ElementPtr sdf) {
        std::cerr << "\nThe UM plugin is attached to model ["
                << model->GetName() << "]\n";

        _model = model;
        _joint = model->GetJoints()[0];
        _model->GetJointController()->SetVelocityPID(_joint->GetScopedName(), _pid);
        _model->GetJointController()->SetVelocityTarget(_joint->GetScopedName(), 10.0);

        _node->Init(_model->GetWorld()->Name());
        std::string topicName = "~/" + _model->GetName() + "/control";
        _topic = _node->Subscribe(topicName, &UniversalModulePlugin::onJointVelCmd, this);
    }
private:
    void onJointVelCmd( JointVelCmdPtr &msg ) {
        auto& joints = _model->GetJoints();
        if ( msg->joint() < joints.size() ) {
            _model->GetJointController()->SetVelocityTarget(
                joints[ msg->joint() ]->GetScopedName(), msg->velocity());
            std::cerr << "Setting velocity of joint " << msg->joint() << " to " << msg->velocity() << "\n";
        }
        else {
            std::cerr << "Warning: invalid joint " << msg->joint() << " specified\n";
        }
    }

    physics::ModelPtr _model;
    physics::JointPtr _joint;
    common::PID _pid;
    transport::NodePtr _node;
    transport::SubscriberPtr _topic;
};

GZ_REGISTER_MODEL_PLUGIN(UniversalModulePlugin)
} // namespace gazebo