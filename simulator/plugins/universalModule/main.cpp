#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>

#include <array>
#include <iostream>
#include <utility>

#include <jointCmd.pb.h>
#include <jointResp.pb.h>

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

rofi::messages::JointResp resp( rofi::messages::JointCmd::Type cmdtype, int joint, float value )
{
    rofi::messages::JointResp resp;
    resp.set_cmdtype( cmdtype );
    resp.set_joint( joint );

    resp.add_values( value );

    return resp;
}

using JointCmdPtr = const boost::shared_ptr< const rofi::messages::JointCmd >;

class UniversalModulePlugin : public ModelPlugin {
public:

    const double maxJointTorque = 10; // Max torque of joint motors
    const std::pair< double, double > jointSpeedBoundaries = { 0, 10 }; // Max speed of joint motors
    const std::pair< double, double > jointPositionBoundaries = { 0, 10 };

    UniversalModulePlugin():
        _pid( 0.1, 0, 0 ),
        _node( boost::make_shared< transport::Node >() )
    {}

    virtual void Load( physics::ModelPtr model, sdf::ElementPtr sdf ) {
        std::cerr << "\nThe UM plugin is attached to model ["
                << model->GetName() << "]\n";

        std::cerr << "Number of joints is " << model->GetJoints().size() << "\n";
        _model = model;
        auto _bodyJoint = _model->GetJoint( "bodyRev" );
        auto _shoeAJoint = _model->GetJoint( "shoeARev" );
        auto _shoeBJoint = _model->GetJoint( "shoeBRev" );
        if ( !_bodyJoint || !_shoeAJoint || !_shoeBJoint ) {
            std::cerr << "\nCould not get all joints\n";
            return;
        }

        for ( auto &joint : { _shoeAJoint, _shoeBJoint } )
        {
            joint->SetParam( "vel", 0, 0.0 );
            joint->SetParam( "fmax", 0, maxJointTorque );
        }

        _bodyJoint->SetParam( "vel", 0, 0.0 );
        _bodyJoint->SetParam( "fmax", 0, maxJointTorque );

        _node->Init( _model->GetWorld()->Name() );
        std::string topicName = "~/" + _model->GetName() + "/control";
        _sub = _node->Subscribe( topicName, &UniversalModulePlugin::onJointCmd, this );
        _pub = _node->Advertise< rofi::messages::JointResp >( "~/" + _model->GetName() + "/response" );

        std::cerr << "Listening...\n";
    }
private:
    void onJointCmd( JointCmdPtr &msg )
    {
        auto& joints = _model->GetJoints();
        if ( msg->joint() < 0 || msg->joint() >= 3 )
        {
            std::cerr << "Warning: invalid joint " << msg->joint() << " specified\n";
            return;
        }

        switch ( msg->cmdtype() )
        {
            using JointCmd = rofi::messages::JointCmd;

        case JointCmd::GET_MAX_POSITION:
        {
            double maxPosition = jointPositionBoundaries.second;
            std::cout << "Returning max position of joint " << msg->joint() << ": " << maxPosition << "\n";
            _pub->Publish( resp( JointCmd::GET_MAX_POSITION, msg->joint(), maxPosition ) );
            break;
        }
        case JointCmd::GET_MIN_POSITION:
        {
            double minPosition = jointPositionBoundaries.first;
            std::cout << "Returning min position of joint " << msg->joint() << ": " << minPosition << "\n";
            _pub->Publish( resp( JointCmd::GET_MIN_POSITION, msg->joint(), minPosition ) );
            break;
        }
        case JointCmd::GET_MAX_SPEED:
        {
            double maxSpeed = jointSpeedBoundaries.second;
            std::cout << "Returning max speed of joint " << msg->joint() << ": " << maxSpeed << "\n";
            _pub->Publish( resp( JointCmd::GET_MAX_SPEED, msg->joint(), maxSpeed ) );
            break;
        }
        case JointCmd::GET_MIN_SPEED:
        {
            double minSpeed = jointSpeedBoundaries.first;
            std::cout << "Returning min speed of joint " << msg->joint() << ": " << minSpeed << "\n";
            _pub->Publish( resp( JointCmd::GET_MIN_SPEED, msg->joint(), minSpeed ) );
            break;
        }
        case JointCmd::GET_MAX_TORQUE:
        {
            double maxTorque = maxJointTorque;
            std::cout << "Returning max torque of joint " << msg->joint() << ": " << maxTorque << "\n";
            _pub->Publish( resp( JointCmd::GET_MAX_TORQUE, msg->joint(), maxTorque ) );
            break;
        }
        case JointCmd::GET_SPEED:
        {
            double speed = 0; // TODO get speed
            std::cout << "Returning current speed of joint " << msg->joint() << ": " << speed << "\n";
            _pub->Publish( resp( JointCmd::GET_SPEED, msg->joint(), speed ) );
            break;
        }
        case JointCmd::SET_SPEED:
        {
            double velocity = msg->setspeed().speed();

            // TODO set max force
            joints[ msg->joint() ]->SetParam( "fmax", 0, maxJointTorque );
            joints[ msg->joint() ]->SetParam( "vel", 0, velocity );
            std::cerr << "Setting speed of joint " << msg->joint() << " to " << velocity << "\n";
            // TODO set boundaries

            std::cerr << to_string( joints[ msg->joint() ]->GetForceTorque( 0 ) ) << "\n";
            break;
        }
        case JointCmd::GET_POSITION:
        {
            double position = 0; // TODO get position
            std::cout << "Returning current position of joint " << msg->joint() << ": " << position << "\n";
            _pub->Publish( resp( JointCmd::GET_POSITION, msg->joint(), position ) );
            break;
        }
        case JointCmd::SET_POS_WITH_SPEED:
        {
            double position = msg->setposwithspeed().position();
            double velocity = msg->setposwithspeed().speed();

            // TODO change sign of velocity if position is on the other side
            // TODO set max force
            joints[ msg->joint() ]->SetParam( "fmax", 0, maxJointTorque );
            joints[ msg->joint() ]->SetParam( "vel", 0, velocity );
            std::cerr << "Setting position of joint " << msg->joint() << " to " << position << " with speed " << velocity << "\n";
            // TODO set boundary (position)

            std::cerr << to_string( joints[ msg->joint() ]->GetForceTorque( 0 ) ) << "\n";
            break;
        }
        case JointCmd::GET_TORQUE:
        {
            double torque = 0; // TODO get torque
            std::cout << "Returning current torque of joint " << msg->joint() << ": " << torque << "\n";
            _pub->Publish( resp( JointCmd::GET_TORQUE, msg->joint(), torque ) );
            break;
        }
        case JointCmd::SET_TORQUE:
        {
            double torque = msg->settorque().torque();

            // TODO set torque
            std::cerr << "Setting torque of joint " << msg->joint() << " to " << torque << "\n";

            std::cerr << to_string( joints[ msg->joint() ]->GetForceTorque( 0 ) ) << "\n";
            break;
        }
        default:
            std::cerr << "Unknown command type: " << msg->cmdtype() << " of joint " << msg->joint() << "\n";
            break;
        }
    }

    physics::ModelPtr _model;
    common::PID _pid;
    transport::NodePtr _node;
    transport::SubscriberPtr _sub;
    transport::PublisherPtr _pub;
};

GZ_REGISTER_MODEL_PLUGIN(UniversalModulePlugin)
} // namespace gazebo