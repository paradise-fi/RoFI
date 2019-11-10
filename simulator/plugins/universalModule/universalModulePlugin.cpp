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

double trim( double value, double min, double max, std::string debugName )
{
    if ( value < min )
    {
        gzwarn << "Value of " << debugName << " trimmed from " << value << " to " << min << "\n";
        return min;
    }
    if ( value > max )
    {
        gzwarn << "Value of " << debugName << " trimmed from " << value << " to " << max << "\n";
        return max;
    }
    return value;
}

double trim( double value, const UMP::limitPair & limits, std::string debugName )
{
    return trim( value, limits.first, limits.second, std::move( debugName ) );
}

bool equal( double first, double second, double precision = UMP::doublePrecision )
{
    return first <= second + precision && second <= first + precision;
}

void UMP::Load( physics::ModelPtr model, sdf::ElementPtr sdf ) {
    gzmsg << "The UM plugin is attached to model ["
            << model->GetName() << "]\n";

    gzmsg << "Number of joints is " << numOfJoints << "\n";
    if ( numOfJoints > model->GetJoints().size() ) {
        gzerr << "Number of joints in gazebo model is lower than expected\nAborting...\n";
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

    gzmsg << "Listening...\n";
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
        case RofiCmd::CONNECTOR_CMD:
            gzwarn << "Connector commands not implemented\n";
            break;
    }
}

void UMP::onJointCmd( const rofi::messages::JointCmd & msg )
{
    using rofi::messages::JointCmd;

    int joint = msg.joint();
    if ( joint < 0 || joint >= numOfJoints )
    {
        gzwarn << "Invalid joint " << joint << " specified\n";
        return;
    }

    switch ( msg.cmdtype() )
    {

    case JointCmd::GET_MAX_POSITION:
    {
        double maxPosition = jointPositionBoundaries[ joint ].second;
        gzmsg << "Returning max position of joint " << joint << ": " << maxPosition << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_MAX_POSITION, joint, maxPosition ) );
        break;
    }
    case JointCmd::GET_MIN_POSITION:
    {
        double minPosition = jointPositionBoundaries[ joint ].first;
        gzmsg << "Returning min position of joint " << joint << ": " << minPosition << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_MIN_POSITION, joint, minPosition ) );
        break;
    }
    case JointCmd::GET_MAX_SPEED:
    {
        double maxSpeed = jointSpeedBoundaries.second;
        gzmsg << "Returning max speed of joint " << joint << ": " << maxSpeed << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_MAX_SPEED, joint, maxSpeed ) );
        break;
    }
    case JointCmd::GET_MIN_SPEED:
    {
        double minSpeed = jointSpeedBoundaries.first;
        gzmsg << "Returning min speed of joint " << joint << ": " << minSpeed << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_MIN_SPEED, joint, minSpeed ) );
        break;
    }
    case JointCmd::GET_MAX_TORQUE:
    {
        double maxTorque = maxJointTorque;
        gzmsg << "Returning max torque of joint " << joint << ": " << maxTorque << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_MAX_TORQUE, joint, maxTorque ) );
        break;
    }
    case JointCmd::GET_VELOCITY:
    {
        double velocity = _model->GetJoints()[ joint ]->GetVelocity( 0 );
        gzmsg << "Returning current velocity of joint " << joint << ": " << velocity << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_VELOCITY, joint, velocity ) );
        break;
    }
    case JointCmd::SET_VELOCITY:
    {
        double velocity = msg.setvelocity().velocity();

        setVelocity( joint, velocity );
        gzmsg << "Setting velocity of joint " << joint << " to " << velocity << "\n";
        break;
    }
    case JointCmd::GET_POSITION:
    {
        double position = _model->GetJoints()[ joint ]->Position( 0 );
        gzmsg << "Returning current position of joint " << joint << ": " << position << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_POSITION, joint, position ) );
        break;
    }
    case JointCmd::SET_POS_WITH_SPEED:
    {
        double position = msg.setposwithspeed().position();
        double speed = msg.setposwithspeed().speed();

        setPositionWithSpeed( joint, position, speed );

        gzmsg << "Setting position of joint " << joint << " to " << position << " with speed " << speed << "\n";
        break;
    }
    case JointCmd::GET_TORQUE:
    {
        double torque = -1.0;
        gzwarn << "Get torque joint command not implemented\n";

        gzmsg << "Returning current torque of joint " << joint << ": " << torque << "\n";
        _pub->Publish( getJointRofiResp( JointCmd::GET_TORQUE, joint, torque ) );
        break;
    }
    case JointCmd::SET_TORQUE:
    {
        double torque = msg.settorque().torque();
        setTorque( joint, torque );
        gzmsg << "Setting torque of joint " << joint << " to " << torque << "\n";
        break;
    }
    default:
        gzwarn << "Unknown command type: " << msg.cmdtype() << " of joint " << joint << "\n";
        break;
    }
}

void UMP::setVelocity( int joint, double velocity )
{
    velocity = trim( velocity, -jointSpeedBoundaries.second, jointSpeedBoundaries.second, "velocity" );
    auto modelJoint = _model->GetJoints()[ joint ];
    modelJoint->SetParam( "fmax", 0, maxJointTorque );
    modelJoint->SetParam( "vel", 0, velocity );
    modelJoint->SetParam( "lo_stop", 0, jointPositionBoundaries[ joint ].first );
    modelJoint->SetParam( "hi_stop", 0, jointPositionBoundaries[ joint ].second );
}

void UMP::setTorque( int joint, double torque )
{
    torque = trim( torque, -maxJointTorque, maxJointTorque, "torque" );
    auto modelJoint = _model->GetJoints()[ joint ];
    modelJoint->SetParam( "fmax", 0, std::abs( torque ) );
    modelJoint->SetParam( "vel", 0, std::copysign( jointSpeedBoundaries.second, torque ) );
    modelJoint->SetParam( "lo_stop", 0, jointPositionBoundaries[ joint ].first );
    modelJoint->SetParam( "hi_stop", 0, jointPositionBoundaries[ joint ].second );
}

void UMP::setPositionWithSpeed( int joint, double desiredPosition, double speed )
{
    double position = trim( desiredPosition, jointPositionBoundaries[ joint ], "position" );
    speed = trim( speed, 0, jointSpeedBoundaries.second, "speed" );

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

    setPositionHandle[ joint ] = event::Events::ConnectWorldUpdateEnd(
        std::bind( &UMP::setPositionCheck, this, joint, position, desiredPosition ) );
}

void UMP::setPositionCheck( int joint, double position, double desiredPosition )
{
    using rofi::messages::JointCmd;

    double currentPosition = _model->GetJoints()[ joint ]->Position( 0 );

    if ( !equal( position, currentPosition ) )
        return;

    setPositionHandle[ joint ] = {};

    setVelocity( joint, 0 );

    gzmsg << "Returning position reached of joint " << joint << ": " << desiredPosition
        << " (actual position: " << currentPosition << ")\n";

    _pub->Publish( getJointRofiResp( JointCmd::SET_POS_WITH_SPEED, joint, desiredPosition ) );
}

GZ_REGISTER_MODEL_PLUGIN(UniversalModulePlugin)

} // namespace gazebo