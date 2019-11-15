#include "universalModulePlugin.hpp"

#include <cmath>

#include "../common/utils.hpp"

namespace gazebo
{
using UMP = UniversalModulePlugin;

void UMP::Load( physics::ModelPtr model, sdf::ElementPtr /*sdf*/ ) {
    gzmsg << "The UM plugin is attached to model ["
            << model->GetName() << "]\n";

    _model = model;

    initCommunication();

    findAndInitJoints();
    findAndInitConnectors();

    gzmsg << "Number of joints is " << joints.size() << "\n";
    gzmsg << "Number of connectors is " << connectors.size() << "\n";
    gzmsg << "Listening...\n";
}

void UMP::initCommunication()
{
    if ( !_model )
    {
        gzerr << "Model has to be set before initializing communication\n";
        return;
    }

    if ( _node )
    {
        _node->Fini();
    }

    _node = boost::make_shared< transport::Node >();
    _node->Init( _model->GetWorld()->Name() + "/" + _model->GetName() );

    _sub = _node->Subscribe( "~/control", & UMP::onRofiCmd, this );
    _pub = _node->Advertise< rofi::messages::RofiResp >( "~/response" );
}

void UMP::addConnector( gazebo::physics::ModelPtr connectorModel )
{
    if ( !_node->IsInitialized() )
    {
        gzerr << "Init communication before adding connectors\n";
        return;
    }

    std::string topicName = "/gazebo/" + getElemPath( connectorModel );

    auto pub = _node->Advertise< rofi::messages::ConnectorCmd >( topicName + "/control" );
    auto sub = _node->Subscribe( topicName + "/response", & UMP::onConnectorResp, this );

    for ( auto & elem : connectors )
    {
        if ( pub->GetTopic() == elem.first->GetTopic() )
        {
            gzerr << "All connector names have to be different\n";
            return;
        }
    }

    connectors.emplace_back( std::move( pub ), std::move( sub ) );

    rofi::messages::ConnectorCmd emptyCmd;
    emptyCmd.set_connector( connectors.size() - 1 );
    emptyCmd.set_cmdtype( rofi::messages::ConnectorCmd::NO_CMD );
    connectors.back().first->Publish( std::move( emptyCmd ) );
}

void UMP::clearConnectors()
{
    for ( auto & pair : connectors )
    {
        pair.first->Fini();
    }
    connectors = {};
}

void UMP::findAndInitJoints()
{
    if ( !_node->IsInitialized() )
    {
        gzerr << "Init communication before adding joints\n";
        return;
    }

    joints = {};

    for ( auto joint : _model->GetJoints() )
    {
        if ( joint->GetMsgType() == msgs::Joint::REVOLUTE )
        {
            joints.emplace_back( std::move( joint ), event::ConnectionPtr() );
        }
    }

    for ( size_t i = 0; i < joints.size(); i++ )
    {
        setVelocity( i, 0 );
    }
}

void UMP::findAndInitConnectors()
{
    if ( !_node->IsInitialized() )
    {
        gzerr << "Init communication before adding connectors\n";
        return;
    }

    clearConnectors();

    for ( auto nested : _model->NestedModels() )
    {
        if ( isRofiConnector( nested ) )
        {
            addConnector( nested );
        }
    }
}

rofi::messages::RofiResp UMP::getJointRofiResp( rofi::messages::JointCmd::Type cmdtype, int joint, float value ) const
{
    rofi::messages::RofiResp resp;
    resp.set_resptype( rofi::messages::RofiCmd::JOINT_CMD );

    auto & jointResp = *resp.mutable_jointresp();
    jointResp.set_joint( joint );
    jointResp.set_cmdtype( cmdtype );

    jointResp.add_values( value );

    return resp;
}

rofi::messages::RofiResp UMP::getConnectorRofiResp( const rofi::messages::ConnectorResp & connectorResp ) const
{
    rofi::messages::RofiResp resp;
    resp.set_resptype( rofi::messages::RofiCmd::CONNECTOR_CMD );
    *resp.mutable_connectorresp() = connectorResp;
    return resp;
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
            onConnectorCmd( msg->connectorcmd() );
            break;
        default:
            gzwarn << "Unknown RoFI command type\n";
    }
}

void UMP::onJointCmd( const rofi::messages::JointCmd & msg )
{
    using rofi::messages::JointCmd;

    int joint = msg.joint();
    if ( joint < 0 || static_cast< size_t >( joint ) >= joints.size() )
    {
        gzwarn << "Invalid joint " << joint << " specified\n";
        return;
    }

    switch ( msg.cmdtype() )
    {
    case JointCmd::NO_CMD:
    {
        _pub->Publish( getJointRofiResp( JointCmd::NO_CMD, joint, 0 ) );
        break;
    }
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
        double velocity = joints.at( joint ).first->GetVelocity( 0 );
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
        double position = joints.at( joint ).first->Position();
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

void UMP::onConnectorCmd( const rofi::messages::ConnectorCmd & msg )
{
    using rofi::messages::ConnectorCmd;

    int connector = msg.connector();
    if ( connector < 0 || static_cast< size_t >( connector ) >= connectors.size() )
    {
        gzwarn << "Invalid connector " << connector << " specified\n";
        return;
    }

    connectors[ connector ].first->Publish( msg );
}

void UMP::onConnectorResp( const UMP::ConnectorRespPtr & msg )
{
    _pub->Publish( getConnectorRofiResp( *msg ) );
}

void UMP::setVelocity( int joint, double velocity )
{
    velocity = trim( velocity, -jointSpeedBoundaries.second, jointSpeedBoundaries.second, "velocity" );
    auto modelJoint = joints.at( joint ).first;
    modelJoint->SetParam( "fmax", 0, maxJointTorque );
    modelJoint->SetParam( "vel", 0, velocity );
    modelJoint->SetParam( "lo_stop", 0, jointPositionBoundaries[ joint ].first );
    modelJoint->SetParam( "hi_stop", 0, jointPositionBoundaries[ joint ].second );
}

void UMP::setTorque( int joint, double torque )
{
    torque = trim( torque, -maxJointTorque, maxJointTorque, "torque" );
    auto modelJoint = joints.at( joint ).first;
    modelJoint->SetParam( "fmax", 0, std::abs( torque ) );
    modelJoint->SetParam( "vel", 0, std::copysign( jointSpeedBoundaries.second, torque ) );
    modelJoint->SetParam( "lo_stop", 0, jointPositionBoundaries[ joint ].first );
    modelJoint->SetParam( "hi_stop", 0, jointPositionBoundaries[ joint ].second );
}

void UMP::setPositionWithSpeed( int joint, double desiredPosition, double speed )
{
    double position = trim( desiredPosition, jointPositionBoundaries[ joint ], "position" );
    speed = trim( speed, 0, jointSpeedBoundaries.second, "speed" );

    auto modelJoint = joints.at( joint ).first;

    double actualPosition = modelJoint->Position();
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

    joints.at( joint ).second = event::Events::ConnectWorldUpdateEnd(
        std::bind( &UMP::setPositionCheck, this, joint, position, desiredPosition ) );
}

void UMP::setPositionCheck( int joint, double position, double desiredPosition )
{
    using rofi::messages::JointCmd;

    double currentPosition = joints.at( joint ).first->Position();

    if ( !equal( position, currentPosition, doublePrecision ) )
        return;

    joints.at( joint ).second = {};

    setVelocity( joint, 0 );

    gzmsg << "Returning position reached of joint " << joint << ": " << desiredPosition
        << " (actual position: " << currentPosition << ")\n";

    _pub->Publish( getJointRofiResp( JointCmd::SET_POS_WITH_SPEED, joint, desiredPosition ) );
}

GZ_REGISTER_MODEL_PLUGIN( UniversalModulePlugin )

} // namespace gazebo
