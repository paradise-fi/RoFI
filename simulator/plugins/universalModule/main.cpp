#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>
#include <iostream>
#include <array>

#include <jointCmd.pb.h>

namespace gazebo {

template <typename T>
std::string to_string(const ignition::math::Vector3<T> &v)
{
    using std::to_string;
    return "X = " + to_string(v.X()) + ", Y = " + to_string(v.Y()) + ", Z = " + to_string(v.Z());
}

std::string to_string(const gazebo::physics::JointWrench &jw)
{
    return "Forces:\n\tbody1: " + to_string(jw.body1Force) + "\n\tbody2: " + to_string(jw.body2Force)
    + "\nTorques:\n\tbody1: " + to_string(jw.body1Torque) + "\n\tbody2: " + to_string(jw.body2Torque);
}

using JointCmdPtr = const boost::shared_ptr< const rofi::messages::JointCmd >;

class UniversalModulePlugin : public ModelPlugin {
public:
    UniversalModulePlugin():
        _pid(0.1, 0, 0),
        _node(new transport::Node())
    {}

    virtual void Load(physics::ModelPtr model, sdf::ElementPtr sdf) {
        std::cerr << "\nThe UM plugin is attached to model ["
                << model->GetName() << "]\n";

        std::cerr << "Number of joints is " << model->GetJoints().size() << "\n";
        _model = model;
        auto _bodyJoint = _model->GetJoint("bodyRev");
        auto _shoeAJoint = _model->GetJoint("shoeARev");
        auto _shoeBJoint = _model->GetJoint("shoeBRev");
        if (!_bodyJoint || !_shoeAJoint || !_shoeBJoint) {
            std::cerr << "\nCould not get all joints\n";
            return;
        }

        for (auto &joint : {_shoeAJoint, _shoeBJoint})
        {
            joint->SetParam("fmax", 0, 100.0);
            joint->SetParam("vel", 0, 0.0);
        }

        _bodyJoint->SetParam("fmax", 0, 100.0);
        _bodyJoint->SetParam("vel", 0, 10.0);

        _node->Init(_model->GetWorld()->Name());
        std::string topicName = "~/" + _model->GetName() + "/control";
        _topic = _node->Subscribe(topicName, &UniversalModulePlugin::onJointCmd, this);
    }
private:
    void onJointCmd( JointCmdPtr &msg )
    {
        auto& joints = _model->GetJoints();
        if ( msg->joint() >= joints.size() || msg->joint() < 0 )
        {
            std::cerr << "Warning: invalid joint " << msg->joint() << " specified\n";
            return;
        }

        switch ( msg->cmdtype() )
        {
            using JointCmd = rofi::messages::JointCmd;

            case JointCmd::SET_TORQUE:
            {
                auto velocity = msg->settorque().torque();

                joints[ msg->joint() ]->SetParam("vel", 0, velocity);
                std::cerr << "Setting velocity of joint " << msg->joint() << " to " << velocity << "\n";
                std::cerr << to_string(joints[ msg->joint() ]->GetForceTorque(0)) << "\n";
                break;
            }
            // default:
            //    std::cerr << "Unknown command type: " << msg->cmdtype() << "\n";
        }
    }

    physics::ModelPtr _model;
    common::PID _pid;
    transport::NodePtr _node;
    transport::SubscriberPtr _topic;
};

GZ_REGISTER_MODEL_PLUGIN(UniversalModulePlugin)
} // namespace gazebo