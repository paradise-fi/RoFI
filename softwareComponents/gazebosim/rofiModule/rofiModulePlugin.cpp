#include "rofiModulePlugin.hpp"

#include <cassert>
#include <cmath>
#include <functional>


namespace gazebo
{
using RMP = RoFIModulePlugin;

void RMP::Load( physics::ModelPtr model, sdf::ElementPtr sdf )
{
    _model = std::move( model );
    _sdf = std::move( sdf );
    assert( _model );
    assert( _sdf );

    gzmsg << "The UM plugin is attached to model [" << _model->GetScopedName() << "]\n";

    initCommunication();

    findAndInitJoints();
    findAndInitConnectors();

    onUpdateConnection =
            event::Events::ConnectWorldUpdateBegin( std::bind( &RMP::onUpdate, this ) );

    gzmsg << "Number of joints is " << joints.size() << "\n";
    gzmsg << "Number of connectors is " << connectors.size() << "\n";

    startListening();
    gzmsg << "UM plugin ready (" << _model->GetScopedName() << ")\n";
}

void RMP::initCommunication()
{
    if ( !_model )
    {
        gzerr << "Model has to be set before initializing communication\n";
        throw std::runtime_error( "Model has to be set before initializing communication" );
        ;
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

    _pub = _node->Advertise< rofi::messages::RofiResp >( "~/response" );
    if ( !_pub )
    {
        gzerr << "Publisher could not be created\n";
        throw std::runtime_error( "Publisher could not be created" );
    }
}

void RMP::startListening()
{
    if ( !_node || !_node->IsInitialized() )
    {
        gzerr << "Init communication before starting to listen\n";
        throw std::runtime_error( "Init communication before starting to listen" );
    }

    _sub = _node->Subscribe( "~/control", &RMP::onRofiCmd, this );
    if ( !_sub )
    {
        gzerr << "Subcriber could not be created\n";
        throw std::runtime_error( "Subcriber could not be created" );
    }

    gzmsg << "Model is listening on topic '" << _sub->GetTopic() << "'\n";
}

void RMP::addConnector( gazebo::physics::ModelPtr connectorModel )
{
    assert( connectorModel );

    if ( !_node || !_node->IsInitialized() )
    {
        gzerr << "Init communication before adding connectors\n";
        throw std::runtime_error( "Init communication before adding connectors" );
    }

    std::string topicName = "/gazebo/" + getElemPath( connectorModel );

    auto pub = _node->Advertise< rofi::messages::ConnectorCmd >( topicName + "/control" );
    auto sub = _node->Subscribe( topicName + "/response", &RMP::onConnectorResp, this );

    if ( !sub )
    {
        gzerr << "Connector subcriber could not be created\n";
        return;
    }

    if ( !pub )
    {
        gzerr << "Connector publisher could not be created\n";
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

void RMP::clearConnectors()
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

void RMP::findAndInitJoints()
{
    assert( _sdf );

    if ( !_node || !_node->IsInitialized() )
    {
        gzerr << "Init communication before adding joints\n";
        throw std::runtime_error( "Init communication before adding joints" );
    }

    joints.clear();

    const auto callback = [ this ]( int joint, double desiredPosition ) {
        gzmsg << "Returning position reached of joint " << joint << ": " << desiredPosition << "\n";

        _pub->Publish( getJointRofiResp( rofi::messages::JointCmd::SET_POS_WITH_SPEED,
                                         joint,
                                         desiredPosition ) );
    };

    checkChildrenNames( _sdf, { "controller" } );
    auto pidValuesVector = PIDLoader::loadControllerValues( _sdf );
    for ( auto & pidValues : pidValuesVector )
    {
        auto joint = _model->GetJoint( pidValues.jointName );
        if ( !joint )
        {
            gzerr << "No joint '" + pidValues.jointName + "' found in module\n";
            continue;
        }

        assert( joints.size() <= INT_MAX );
        joints.emplace_back( std::move( joint ),
                             nullptr,
                             pidValues,
                             std::bind( callback, joints.size(), std::placeholders::_1 ),
                             static_cast< int >( joints.size() ) );
        assert( joints.back() );
        assert( joints.back().joint->GetMsgType() == msgs::Joint::REVOLUTE );
    }
}

void RMP::findAndInitConnectors()
{
    if ( !_node || !_node->IsInitialized() )
    {
        gzerr << "Init communication before adding connectors\n";
        throw std::runtime_error( "Init communication before adding connectors" );
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

rofi::messages::RofiResp RMP::getJointRofiResp( rofi::messages::JointCmd::Type resptype,
                                                int joint,
                                                float value ) const
{
    rofi::messages::RofiResp resp;
    resp.set_rofiid( rofiId.value_or( 0 ) );
    resp.set_resptype( rofi::messages::RofiCmd::JOINT_CMD );

    auto & jointResp = *resp.mutable_jointresp();
    jointResp.set_joint( joint );
    jointResp.set_resptype( resptype );

    jointResp.set_value( value );

    return resp;
}

rofi::messages::RofiResp RMP::getConnectorRofiResp(
        const rofi::messages::ConnectorResp & connectorResp ) const
{
    rofi::messages::RofiResp resp;
    resp.set_rofiid( rofiId.value_or( 0 ) );
    resp.set_resptype( rofi::messages::RofiCmd::CONNECTOR_CMD );
    *resp.mutable_connectorresp() = connectorResp;
    return resp;
}

void RMP::onRofiCmd( const RMP::RofiCmdPtr & msg )
{
    using rofi::messages::RofiCmd;

    if ( rofiId && *rofiId != msg->rofiid() )
    {
        gzwarn << "Had ID: " << *rofiId << ", but got cmd with ID: " << msg->rofiid() << "\n";
    }
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
        case RofiCmd::DESCRIPTION:
        {
            if ( !rofiId )
            {
                rofiId = msg->rofiid();
                for ( auto & joint : joints )
                {
                    joint.controller.setRofiId( rofiId.value() );
                }
            }

            rofi::messages::RofiResp resp;
            resp.set_rofiid( rofiId.value() );
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
            gzmsg << "Starting waiting (" << msg->waitcmd().waitms()
                  << " ms, ID: " << msg->waitcmd().waitid() << ")\n";
            int32_t sec = msg->waitcmd().waitms() / 1000;
            int32_t nsec = ( msg->waitcmd().waitms() % 1000 ) * 1000000;
            auto afterWaited = _model->GetWorld()->SimTime() + common::Time( sec, nsec );

            {
                std::lock_guard< std::mutex > lock( waitCallbacksMapMutex );
                waitCallbacksMap.emplace(
                        afterWaited,
                        [ this, waitId = msg->waitcmd().waitid() ]() {
                            rofi::messages::RofiResp resp;
                            resp.set_rofiid( rofiId.value_or( 0 ) );
                            resp.set_resptype( rofi::messages::RofiCmd::WAIT_CMD );
                            resp.set_waitid( waitId );
                            gzmsg << "Returning waiting ended (ID: " << waitId << ")\n";
                            _pub->Publish( std::move( resp ) );
                        } );
            }

            break;
        }
        default:
            gzwarn << "Unknown RoFI command type\n";
    }
}

void RMP::onJointCmd( const rofi::messages::JointCmd & msg )
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
            break;
        case JointCmd::GET_CAPABILITIES:
        {
            auto & jointData = joints.at( joint );

            rofi::messages::RofiResp resp;
            resp.set_rofiid( rofiId.value_or( 0 ) );
            resp.set_resptype( rofi::messages::RofiCmd::JOINT_CMD );

            auto & jointResp = *resp.mutable_jointresp();
            jointResp.set_joint( joint );
            jointResp.set_resptype( JointCmd::GET_CAPABILITIES );

            auto & capabilities = *jointResp.mutable_capabilities();
            capabilities.set_maxposition( jointData.getMaxPosition() );
            capabilities.set_minposition( jointData.getMinPosition() );
            capabilities.set_maxspeed( jointData.getMaxVelocity() );
            capabilities.set_minspeed( jointData.getMinVelocity() );
            capabilities.set_maxtorque( jointData.getMaxEffort() );

            gzmsg << "Returning capabilities of joint (" << joint << ")\n";

            _pub->Publish( resp );
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
            gzwarn << "Unknown command type " << msg.cmdtype() << " of joint " << joint << "\n";
            break;
    }
}

void RMP::onConnectorCmd( const rofi::messages::ConnectorCmd & msg )
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

void RMP::onConnectorResp( const RMP::ConnectorRespPtr & msg )
{
    _pub->Publish( getConnectorRofiResp( *msg ) );
}

void RMP::onUpdate()
{
    assert( _model );
    auto actualTime = _model->GetWorld()->SimTime();

    std::multimap< common::Time, std::function< void() > > tmpMap;

    {
        std::lock_guard< std::mutex > lock( waitCallbacksMapMutex );

        auto it = waitCallbacksMap.begin();
        while ( it != waitCallbacksMap.end() )
        {
            if ( it->first > actualTime )
            {
                break;
            }
            tmpMap.insert( tmpMap.end(), waitCallbacksMap.extract( it++ ) );
        }
    }

    for ( auto & elem : tmpMap )
    {
        elem.second();
    }
}

void RMP::setVelocity( int joint, double velocity )
{
    auto & jointData = joints.at( joint );
    auto targets = PIDLoader::InitTargets::newVelocity( jointData.joint->GetName(), velocity );

    PIDLoader::updateInitTargetsSdf( _sdf, targets );
    jointData.controller.setTargetVelocity( velocity );
}

void RMP::setTorque( int joint, double torque )
{
    auto & jointData = joints.at( joint );
    auto targets = PIDLoader::InitTargets::newForce( jointData.joint->GetName(), torque );

    PIDLoader::updateInitTargetsSdf( _sdf, targets );
    jointData.controller.setTargetForce( torque );
}

void RMP::setPositionWithSpeed( int joint, double desiredPosition, double speed )
{
    using InitTargets = PIDLoader::InitTargets;

    auto & jointData = joints.at( joint );
    auto targets = InitTargets::newPosition( jointData.joint->GetName(), desiredPosition, speed );

    PIDLoader::updateInitTargetsSdf( _sdf, targets );
    jointData.controller.setTargetPositionWithSpeed( desiredPosition, speed );
}

GZ_REGISTER_MODEL_PLUGIN( RoFIModulePlugin )

} // namespace gazebo
