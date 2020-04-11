#include "universalModulePlugin.hpp"

#include <cmath>
#include <functional>


namespace gazebo
{
using UMP = UniversalModulePlugin;

void UMP::Load( physics::ModelPtr model, sdf::ElementPtr /*sdf*/ )
{
    _model = model;
    gzmsg << "The UM plugin is attached to model [" << _model->GetScopedName() << "]\n";

    initCommunication();

    findAndInitJoints();
    findAndInitConnectors();

    onUpdateConnection =
            event::Events::ConnectWorldUpdateBegin( std::bind( &UMP::onUpdate, this ) );

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
    if ( !_node )
    {
        gzerr << "Could not create new Node\n";
        throw std::runtime_error( "Could not create new Node" );
    }
    _node->Init( getElemPath( _model ) );

    _sub = _node->Subscribe( "~/control", &UMP::onRofiCmd, this );
    _pub = _node->Advertise< rofi::messages::RofiResp >( "~/response" );
    if ( !_sub || !_pub )
    {
        gzerr << "Subcriber or Publisher not created\n";
        throw std::runtime_error( "Subcriber or Publisher not created" );
    }

    gzmsg << "Model is listening on topic '" << _sub->GetTopic() << "'\n";
}

void UMP::addConnector( gazebo::physics::ModelPtr connectorModel )
{
    assert( connectorModel );

    if ( !_node || !_node->IsInitialized() )
    {
        gzerr << "Init communication before adding connectors\n";
        return;
    }

    std::string topicName = "/gazebo/" + getElemPath( connectorModel );

    auto pub = _node->Advertise< rofi::messages::ConnectorCmd >( topicName + "/control" );
    auto sub = _node->Subscribe( topicName + "/response", &UMP::onConnectorResp, this );

    if ( !sub || !pub )
    {
        gzerr << "Connector Subcriber or Publisher not created\n";
        return;
    }

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
        if ( pair.first )
        {
            pair.first->Fini();
        }
    }
    connectors.clear();
}

void UMP::findAndInitJoints()
{
    if ( !_node || !_node->IsInitialized() )
    {
        gzerr << "Init communication before adding joints\n";
        return;
    }

    const auto callback = [ this ]( int joint, double desiredPosition ) {
        gzmsg << "Returning position reached of joint " << joint << ": " << desiredPosition << "\n";

        _pub->Publish( getJointRofiResp( rofi::messages::JointCmd::SET_POS_WITH_SPEED,
                                         joint,
                                         desiredPosition ) );
    };

    auto pidValuesVector = PIDLoader::loadControllerValues(
            getPluginSdf( _model->GetSDF(), "libuniversalModulePlugin.so" ) );

    joints.clear();

    for ( auto & pidValues : pidValuesVector )
    {
        if ( auto joint = _model->GetJoint( pidValues.jointName ) )
        {
            assert( joint );
            joints.emplace_back( std::move( joint ),
                                 std::move( pidValues ),
                                 std::bind( callback, joints.size(), std::placeholders::_1 ) );
            assert( joints.back() );
            assert( joints.back().joint->GetMsgType() == msgs::Joint::REVOLUTE );
        }
    }
}

void UMP::findAndInitConnectors()
{
    if ( !_node || !_node->IsInitialized() )
    {
        gzerr << "Init communication before adding connectors\n";
        return;
    }

    clearConnectors();

    for ( auto nested : _model->NestedModels() )
    {
        if ( isRoFICoM( nested ) )
        {
            addConnector( nested );
        }
    }
}

rofi::messages::RofiResp UMP::getJointRofiResp( rofi::messages::JointCmd::Type cmdtype,
                                                int joint,
                                                float value ) const
{
    rofi::messages::RofiResp resp;
    resp.set_rofiid( rofiId.value_or( 0 ) );
    resp.set_resptype( rofi::messages::RofiCmd::JOINT_CMD );

    auto & jointResp = *resp.mutable_jointresp();
    jointResp.set_joint( joint );
    jointResp.set_cmdtype( cmdtype );

    jointResp.add_values( value );

    return resp;
}

rofi::messages::RofiResp UMP::getConnectorRofiResp(
        const rofi::messages::ConnectorResp & connectorResp ) const
{
    rofi::messages::RofiResp resp;
    resp.set_rofiid( rofiId.value_or( 0 ) );
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
            if ( rofiId && *rofiId != msg->rofiid() )
            {
                gzerr << "Had ID: " << *rofiId << ", but got cmd with ID: " << msg->rofiid()
                      << "\n";
            }
            onJointCmd( msg->jointcmd() );
            break;
        case RofiCmd::CONNECTOR_CMD:
            if ( rofiId && *rofiId != msg->rofiid() )
            {
                gzerr << "Had ID: " << *rofiId << ", but got cmd with ID: " << msg->rofiid()
                      << "\n";
            }
            onConnectorCmd( msg->connectorcmd() );
            break;
        case RofiCmd::DESCRIPTION:
        {
            if ( rofiId && *rofiId != msg->rofiid() )
            {
                gzerr << "Had ID: " << *rofiId << ", but got cmd with ID: " << msg->rofiid()
                      << "\n";
            }
            if ( !rofiId )
            {
                rofiId = msg->rofiid();
            }

            rofi::messages::RofiResp resp;
            resp.set_rofiid( rofiId.value_or( 0 ) );
            resp.set_resptype( rofi::messages::RofiCmd::DESCRIPTION );
            auto description = resp.mutable_rofidescription();
            description->set_jointcount( joints.size() );
            description->set_connectorcount( connectors.size() );
            gzmsg << "Returning description (" << resp.rofidescription().jointcount() << " joints, "
                  << resp.rofidescription().connectorcount() << " connectors)\n";
            _pub->Publish( std::move( resp ) );
            break;
        }
        case RofiCmd::WAIT_CMD:
        {
            if ( rofiId && *rofiId != msg->rofiid() )
            {
                gzerr << "Had ID: " << *rofiId << ", but got cmd with ID: " << msg->rofiid()
                      << "\n";
            }

            gzmsg << "Starting waiting (" << msg->waitcmd().waitms()
                  << " s, ID: " << msg->waitcmd().waitid() << ")\n";
            auto afterWaited = _model->GetWorld()->SimTime()
                               + common::Time( 0, msg->waitcmd().waitms() * 1000000 );

            std::lock_guard< std::mutex > lock( waitCallbacksMapMutex );
            waitCallbacksMap.emplace( afterWaited, [ this, waitId = msg->waitcmd().waitid() ]() {
                rofi::messages::RofiResp resp;
                resp.set_rofiid( rofiId.value_or( 0 ) );
                resp.set_resptype( rofi::messages::RofiCmd::WAIT_CMD );
                resp.set_waitid( waitId );
                gzmsg << "Returning waiting ended (ID: " << waitId << ")\n";
                _pub->Publish( std::move( resp ) );
            } );

            break;
        }
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
            double maxPosition = joints.at( joint ).getMaxPosition();
            gzmsg << "Returning max position of joint " << joint << ": " << maxPosition << "\n";
            _pub->Publish( getJointRofiResp( JointCmd::GET_MAX_POSITION, joint, maxPosition ) );
            break;
        }
        case JointCmd::GET_MIN_POSITION:
        {
            double minPosition = joints.at( joint ).getMinPosition();
            gzmsg << "Returning min position of joint " << joint << ": " << minPosition << "\n";
            _pub->Publish( getJointRofiResp( JointCmd::GET_MIN_POSITION, joint, minPosition ) );
            break;
        }
        case JointCmd::GET_MAX_SPEED:
        {
            double maxSpeed = joints.at( joint ).getMaxVelocity();
            gzmsg << "Returning max speed of joint " << joint << ": " << maxSpeed << "\n";
            _pub->Publish( getJointRofiResp( JointCmd::GET_MAX_SPEED, joint, maxSpeed ) );
            break;
        }
        case JointCmd::GET_MIN_SPEED:
        {
            double minSpeed = joints.at( joint ).getMinVelocity();
            gzmsg << "Returning min speed of joint " << joint << ": " << minSpeed << "\n";
            _pub->Publish( getJointRofiResp( JointCmd::GET_MIN_SPEED, joint, minSpeed ) );
            break;
        }
        case JointCmd::GET_MAX_TORQUE:
        {
            double maxTorque = joints.at( joint ).getMaxEffort();
            gzmsg << "Returning max torque of joint " << joint << ": " << maxTorque << "\n";
            _pub->Publish( getJointRofiResp( JointCmd::GET_MAX_TORQUE, joint, maxTorque ) );
            break;
        }
        case JointCmd::GET_VELOCITY:
        {
            double velocity = joints.at( joint ).joint->GetVelocity( 0 );
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
            double position = joints.at( joint ).joint->Position();
            gzmsg << "Returning current position of joint " << joint << ": " << position << "\n";
            _pub->Publish( getJointRofiResp( JointCmd::GET_POSITION, joint, position ) );
            break;
        }
        case JointCmd::SET_POS_WITH_SPEED:
        {
            double position = msg.setposwithspeed().position();
            double speed = msg.setposwithspeed().speed();

            setPositionWithSpeed( joint, position, speed );

            gzmsg << "Setting position of joint " << joint << " to " << position << " with speed "
                  << speed << "\n";
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

void UMP::onUpdate()
{
    std::lock_guard< std::mutex > lock( waitCallbacksMapMutex );

    assert( _model );
    auto actualTime = _model->GetWorld()->SimTime();

    auto it = waitCallbacksMap.begin();
    for ( ; it != waitCallbacksMap.end(); it++ )
    {
        if ( it->first > actualTime )
        {
            break;
        }
        ( it->second )();
    }
    waitCallbacksMap.erase( waitCallbacksMap.begin(), it );
}

void UMP::setVelocity( int joint, double velocity )
{
    joints.at( joint ).pid.setTargetVelocity( velocity );
}

void UMP::setTorque( int joint, double torque )
{
    joints.at( joint ).pid.setTargetForce( torque );
}

void UMP::setPositionWithSpeed( int joint, double desiredPosition, double speed )
{
    joints.at( joint ).pid.setTargetPositionWithSpeed( desiredPosition, speed );
}

GZ_REGISTER_MODEL_PLUGIN( UniversalModulePlugin )

} // namespace gazebo