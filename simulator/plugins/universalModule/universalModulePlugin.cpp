#include "universalModulePlugin.hpp"

#include <cmath>
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

double trim( double value, double min, double max )
{
    if ( value < min )
        return min;
    if ( value > max )
        return max;
    return value;
}

double trim( double value, const UMP::limitPair & limits )
{
    return trim( value, limits.first, limits.second );
}

bool equal( double first, double second, double precision = UMP::doublePrecision )
{
    return first <= second + precision && second <= first + precision;
}

void UMP::Load( physics::ModelPtr model, sdf::ElementPtr sdf ) {
    std::cerr << "\nThe UM plugin is attached to model ["
            << model->GetName() << "]\n";

    std::cerr << "Number of joints is " << numOfJoints << "\n";
    if ( numOfJoints > model->GetJoints().size() ) {
        std::cerr << "Number of joints in gazebo model is lower than expected\n";
        std::cerr << "Aborting...\n";
        return;
    }

    _model = model;
    for ( int i = 0; i < numOfJoints; i++ )
    {
        setVelocity( i, 0 );
    }

    _node->Init( _model->GetWorld()->Name() );
    std::string topicName = "~/" + _model->GetName() + "/control";
    _sub = _node->Subscribe( topicName, &UniversalModulePlugin::onRofiCmd, this );
    _pub = _node->Advertise< rofi::messages::RofiResp >( "~/" + _model->GetName() + "/response" );

    std::cerr << "Listening...\n";
}

void UMP::onRofiCmd( const UMP::RofiCmdPtr & msg )
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

void UMP::onJointCmd( const rofi::messages::JointCmd & msg )
{
    using rofi::messages::JointCmd;

    int joint = msg.joint();
    if ( joint < 0 || joint >= numOfJoints )
    {
        std::cerr << "Warning: invalid joint " << joint << " specified\n";
        return;
    }

    switch ( msg.cmdtype() )
    {

    case JointCmd::GET_MAX_POSITION:
    {
        double maxPosition = jointPositionBoundaries[ joint ].second;
        std::cout << "Returning max position of joint " << joint << ": " << maxPosition << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_MAX_POSITION, joint, maxPosition ) );
        break;
    }
    case JointCmd::GET_MIN_POSITION:
    {
        double minPosition = jointPositionBoundaries[ joint ].first;
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
        double velocity = _model->GetJoints()[ joint ]->GetVelocity( 0 );
        std::cout << "Returning current velocity of joint " << joint << ": " << velocity << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_VELOCITY, joint, velocity ) );
        break;
    }
    case JointCmd::SET_VELOCITY:
    {
        double velocity = msg.setvelocity().velocity();

        setVelocity( joint, velocity );
        std::cerr << "Setting velocity of joint " << joint << " to " << velocity << "\n";
        break;
    }
    case JointCmd::GET_POSITION:
    {
        double position = _model->GetJoints()[ joint ]->Position( 0 );
        std::cout << "Returning current position of joint " << joint << ": " << position << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_POSITION, joint, position ) );
        break;
    }
    case JointCmd::SET_POS_WITH_SPEED:
    {
        double position = msg.setposwithspeed().position();
        double speed = msg.setposwithspeed().speed();

        setPositionWithSpeed( joint, position, speed );

        std::cerr << "Setting position of joint " << joint << " to " << position << " with speed " << speed << "\n";
        break;
    }
    case JointCmd::GET_TORQUE:
    {
        double torque = 0; // TODO get torque
        std::cerr << "Torque:\n" << to_string( _model->GetJoints()[ joint ]->GetForceTorque( 0 ) ) << "\n";

        std::cout << "Returning current torque of joint " << joint << ": " << torque << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_TORQUE, joint, torque ) );
        break;
    }
    case JointCmd::SET_TORQUE:
    {
        double torque = msg.settorque().torque();
        setTorque( joint, torque );
        std::cerr << "Setting torque of joint " << joint << " to " << torque << "\n";
        break;
    }
    default:
        std::cerr << "Unknown command type: " << msg.cmdtype() << " of joint " << joint << "\n";
        break;
    }
}

void UMP::setVelocity( int joint, double velocity )
{
    velocity = trim( velocity, -jointSpeedBoundaries.second, jointSpeedBoundaries.second );
    auto modelJoint = _model->GetJoints()[ joint ];
    modelJoint->SetParam( "fmax", 0, maxJointTorque );
    modelJoint->SetParam( "vel", 0, velocity );
    modelJoint->SetParam( "lo_stop", 0, jointPositionBoundaries[ joint ].first );
    modelJoint->SetParam( "hi_stop", 0, jointPositionBoundaries[ joint ].second );
}

void UMP::setTorque( int joint, double torque )
{
    torque = trim( torque, -maxJointTorque, maxJointTorque );
    auto modelJoint = _model->GetJoints()[ joint ];
    modelJoint->SetParam( "fmax", 0, std::abs( torque ) );
    modelJoint->SetParam( "vel", 0, std::copysign( jointSpeedBoundaries.second, torque ) );
    modelJoint->SetParam( "lo_stop", 0, jointPositionBoundaries[ joint ].first );
    modelJoint->SetParam( "hi_stop", 0, jointPositionBoundaries[ joint ].second );
}

void UMP::setPositionWithSpeed( int joint, double position, double speed )
{
    position = trim( position, jointPositionBoundaries[ joint ] );
    speed = trim( speed, 0, jointSpeedBoundaries.second );

    auto modelJoint = _model->GetJoints()[ joint ];

    double actualPosition = modelJoint->Position( 0 );
    if ( position > actualPosition )
    {
        modelJoint->SetParam( "lo_stop", 0, jointPositionBoundaries[ joint ].first );
        modelJoint->SetParam( "hi_stop", 0, position );
        modelJoint->SetParam( "vel", 0, speed );
    }
    else
    {
        modelJoint->SetParam( "lo_stop", 0, position );
        modelJoint->SetParam( "hi_stop", 0, jointPositionBoundaries[ joint ].second );
        modelJoint->SetParam( "vel", 0, -speed );
    }

    modelJoint->SetParam( "fmax", 0, maxJointTorque );

    setPositionHandle[ joint ] = event::Events::ConnectWorldUpdateEnd( std::bind( &UMP::setPositionCheck, this, joint, position ) );
}

void UMP::setPositionCheck( int joint, double position )
{
    using rofi::messages::JointCmd;

    double currentPosition = _model->GetJoints()[ joint ]->Position( 0 );

    if ( !equal( position, currentPosition ) )
        return;

    setPositionHandle[ joint ] = {};

    std::cout << "Returning position reached of joint " << joint << ": " << currentPosition << "\n";
    _pub->Publish( getJointRofiResp( JointCmd::SET_POS_WITH_SPEED, joint, currentPosition ) );
}

GZ_REGISTER_MODEL_PLUGIN(UniversalModulePlugin)

} // namespace gazebo