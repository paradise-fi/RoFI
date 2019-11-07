#include "universalModulePlugin.hpp"

#include <iostream>

#include <rofiResp.pb.h>

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

rofi::messages::RofiResp getJointRofiResp( rofi::messages::JointCmd::Type cmdtype, int joint, float value )
{
    rofi::messages::RofiResp resp;
    resp.set_resptype( rofi::messages::RofiCmd::JOINT_CMD );
    auto & jointResp = *resp.mutable_jointresp();
    jointResp.set_joint( joint );
    jointResp.set_cmdtype( cmdtype );

    jointResp.add_values( value );

    return resp;
}


using UMP = UniversalModulePlugin;

void UMP::Load( physics::ModelPtr model, sdf::ElementPtr sdf ) {
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
    _sub = _node->Subscribe( topicName, &UniversalModulePlugin::onRofiCmd, this );
    _pub = _node->Advertise< rofi::messages::RofiResp >( "~/" + _model->GetName() + "/response" );

    std::cerr << "Listening...\n";
}

void UMP::onRofiCmd( const UMP::RofiCmdPtr &msg)
{
    using rofi::messages::RofiCmd;

    switch ( msg->cmdtype() )
    {
        case RofiCmd::NO_CMD:
            break;
        case RofiCmd::JOINT_CMD:
            onJointCmd( msg->jointcmd() );
            break;
        case RofiCmd::DOCK_CMD:
            std::cerr << "Dock commands not implemented\n";
            break;
    }
}

void UMP::onJointCmd( const rofi::messages::JointCmd &msg )
{
    int joint = msg.joint();
    if ( joint < 0 || joint >= 3 )
    {
        std::cerr << "Warning: invalid joint " << joint << " specified\n";
        return;
    }

    switch ( msg.cmdtype() )
    {
        using JointCmd = rofi::messages::JointCmd;

    case JointCmd::GET_MAX_POSITION:
    {
        double maxPosition = jointPositionBoundaries.second;
        std::cout << "Returning max position of joint " << joint << ": " << maxPosition << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_MAX_POSITION, joint, maxPosition ) );
        break;
    }
    case JointCmd::GET_MIN_POSITION:
    {
        double minPosition = jointPositionBoundaries.first;
        std::cout << "Returning min position of joint " << joint << ": " << minPosition << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_MIN_POSITION, joint, minPosition ) );
        break;
    }
    case JointCmd::GET_MAX_SPEED:
    {
        double maxSpeed = jointSpeedBoundaries.second;
        std::cout << "Returning max speed of joint " << joint << ": " << maxSpeed << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_MAX_SPEED, joint, maxSpeed ) );
        break;
    }
    case JointCmd::GET_MIN_SPEED:
    {
        double minSpeed = jointSpeedBoundaries.first;
        std::cout << "Returning min speed of joint " << joint << ": " << minSpeed << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_MIN_SPEED, joint, minSpeed ) );
        break;
    }
    case JointCmd::GET_MAX_TORQUE:
    {
        double maxTorque = maxJointTorque;
        std::cout << "Returning max torque of joint " << joint << ": " << maxTorque << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_MAX_TORQUE, joint, maxTorque ) );
        break;
    }
    case JointCmd::GET_VELOCITY:
    {
        double velocity = 0; // TODO get velocity
        std::cout << "Returning current velocity of joint " << joint << ": " << velocity << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_VELOCITY, joint, velocity ) );
        break;
    }
    case JointCmd::SET_VELOCITY:
    {
        double velocity = msg.setvelocity().velocity();

        // TODO set max force
        auto &modelJoint = _model->GetJoints()[ joint ];
        modelJoint->SetParam( "fmax", 0, maxJointTorque );
        modelJoint->SetParam( "vel", 0, velocity );
        std::cerr << "Setting velocity of joint " << joint << " to " << velocity << "\n";
        // TODO set boundaries
        break;
    }
    case JointCmd::GET_POSITION:
    {
        double position = 0; // TODO get position
        std::cout << "Returning current position of joint " << joint << ": " << position << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_POSITION, joint, position ) );
        break;
    }
    case JointCmd::SET_POS_WITH_SPEED:
    {
        double position = msg.setposwithspeed().position();
        double velocity = msg.setposwithspeed().speed();

        // TODO change sign of velocity if position is on the other side
        // TODO set max force
        auto &modelJoint = _model->GetJoints()[ joint ];
        modelJoint->SetParam( "fmax", 0, maxJointTorque );
        modelJoint->SetParam( "vel", 0, velocity );
        std::cerr << "Setting position of joint " << joint << " to " << position << " with speed " << velocity << "\n";
        // TODO set boundary (position)
        break;
    }
    case JointCmd::GET_TORQUE:
    {
        double torque = 0; // TODO get torque
        std::cout << "Returning current torque of joint " << joint << ": " << torque << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_TORQUE, joint, torque ) );
        break;
    }
    case JointCmd::SET_TORQUE:
    {
        double torque = msg.settorque().torque();

        // TODO set torque
        auto &modelJoint = _model->GetJoints()[ joint ];
        std::cerr << "Setting torque of joint " << joint << " to " << torque << "\n";

        std::cerr << "Torque:\n" << to_string( modelJoint->GetForceTorque( 0 ) ) << "\n";
        break;
    }
    default:
        std::cerr << "Unknown command type: " << msg.cmdtype() << " of joint " << joint << "\n";
        break;
    }
}

GZ_REGISTER_MODEL_PLUGIN(UniversalModulePlugin)

} // namespace gazebo