#include "rofiModulePlugin.hpp"

#include <algorithm>

#include <gz/plugin/Register.hh>

namespace gazebo
{
void RoFIModulePlugin::Configure( const gz::sim::Entity & entity,
                                  const std::shared_ptr< const sdf::Element > & sdf,
                                  gz::sim::EntityComponentManager & ecm,
                                  gz::sim::EventManager & )
{
    _entity = entity;
    _model = gz::sim::Model( entity );
    _sdf = sdf;

    initCommunication( ecm );
    findAndInitJoints( ecm );
    findAndInitConnectors( ecm );
    startListening();
}

void RoFIModulePlugin::PreUpdate( const gz::sim::UpdateInfo & info,
                                  gz::sim::EntityComponentManager & ecm )
{
    std::vector< rofi::messages::RofiCmd > pendingCommands;
    std::vector< rofi::messages::ConnectorResp > pendingConnectorResponses;
    {
        std::lock_guard lock( _queueMutex );
        pendingCommands.swap( _pendingCommands );
        pendingConnectorResponses.swap( _pendingConnectorResponses );
    }

    for ( const auto & cmd : pendingCommands )
    {
        handleRofiCmd( cmd, info, ecm );
    }

    for ( const auto & response : pendingConnectorResponses )
    {
        _pub->Publish( getConnectorRofiResp( response ) );
    }

    processWaitCallbacks( info );

    if ( !info.paused )
    {
        for ( auto & joint : joints )
        {
            joint.controller.update( info, ecm );
        }
    }
}

void RoFIModulePlugin::initCommunication( const gz::sim::EntityComponentManager & ecm )
{
    _node = std::make_shared< rofi::gz::Node >();
    _node->Init( getElemPath( _entity, ecm ) );
    _pub = _node->Advertise< rofi::messages::RofiResp >( "~/response" );
}

void RoFIModulePlugin::startListening()
{
    _sub = _node->Subscribe( "~/control", &RoFIModulePlugin::onRofiCmd, this );
}

void RoFIModulePlugin::addConnector( gz::sim::Entity connectorModel,
                                     const gz::sim::EntityComponentManager & ecm )
{
    std::string topicName = "/gazebo/" + getElemPath( connectorModel, ecm );
    auto pub = _node->Advertise< rofi::messages::ConnectorCmd >( topicName + "/control" );
    auto sub = _node->Subscribe( topicName + "/response", &RoFIModulePlugin::onConnectorResp, this );

    connectors.emplace_back( std::move( pub ), std::move( sub ) );

    rofi::messages::ConnectorCmd emptyCmd;
    emptyCmd.set_connector( static_cast< int >( connectors.size() ) - 1 );
    emptyCmd.set_cmdtype( rofi::messages::ConnectorCmd::NO_CMD );
    connectors.back().first->Publish( emptyCmd );
}

void RoFIModulePlugin::clearConnectors()
{
    connectors.clear();
}

void RoFIModulePlugin::findAndInitJoints( gz::sim::EntityComponentManager & ecm )
{
    joints.clear();

    auto controllerValues = PIDLoader::loadControllerValues(
            std::const_pointer_cast< sdf::Element >( _sdf ) );
    for ( const auto & pidValues : controllerValues )
    {
        auto jointEntity = _model.JointByName( ecm, pidValues.jointName );
        if ( jointEntity == gz::sim::kNullEntity )
        {
            gzerr << "No joint '" << pidValues.jointName << "' found in module\n";
            continue;
        }

        joints.emplace_back( jointEntity,
                             nullptr,
                             ecm,
                             pidValues,
                             [ this, index = joints.size() ]( double desiredPosition ) {
                                 auto resp = getJointRofiResp(
                                         rofi::messages::JointCmd::SET_POS_WITH_SPEED,
                                         static_cast< int >( index ),
                                         static_cast< float >( desiredPosition ) );
                                 _pub->Publish( resp );
                             },
                             static_cast< uint8_t >( joints.size() ) );
    }
}

void RoFIModulePlugin::findAndInitConnectors( const gz::sim::EntityComponentManager & ecm )
{
    clearConnectors();
    for ( auto nested : _model.Models( ecm ) )
    {
        if ( isRoFICoM( nested, ecm ) )
        {
            addConnector( nested, ecm );
        }
    }
}

rofi::messages::RofiResp RoFIModulePlugin::getJointRofiResp(
        rofi::messages::JointCmd::Type resptype,
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

rofi::messages::RofiResp RoFIModulePlugin::getConnectorRofiResp(
        const rofi::messages::ConnectorResp & connectorResp ) const
{
    rofi::messages::RofiResp resp;
    resp.set_rofiid( rofiId.value_or( 0 ) );
    resp.set_resptype( rofi::messages::RofiCmd::CONNECTOR_CMD );
    *resp.mutable_connectorresp() = connectorResp;
    return resp;
}

void RoFIModulePlugin::onRofiCmd( const RofiCmdPtr & msg )
{
    if ( !msg )
    {
        return;
    }
    std::lock_guard lock( _queueMutex );
    _pendingCommands.push_back( *msg );
}

void RoFIModulePlugin::onConnectorResp( const ConnectorRespPtr & msg )
{
    if ( !msg )
    {
        return;
    }
    std::lock_guard lock( _queueMutex );
    _pendingConnectorResponses.push_back( *msg );
}

void RoFIModulePlugin::handleRofiCmd( const rofi::messages::RofiCmd & msg,
                                      const gz::sim::UpdateInfo & info,
                                      gz::sim::EntityComponentManager & ecm )
{
    using rofi::messages::RofiCmd;

    if ( rofiId && *rofiId != msg.rofiid() )
    {
        gzwarn << "Had ID: " << *rofiId << ", but got cmd with ID: " << msg.rofiid() << "\n";
    }

    switch ( msg.cmdtype() )
    {
        case RofiCmd::NO_CMD:
            break;
        case RofiCmd::JOINT_CMD:
            onJointCmd( msg.jointcmd(), ecm );
            break;
        case RofiCmd::CONNECTOR_CMD:
            onConnectorCmd( msg.connectorcmd() );
            break;
        case RofiCmd::DESCRIPTION:
        {
            if ( !rofiId )
            {
                rofiId = msg.rofiid();
                for ( auto & joint : joints )
                {
                    joint.controller.setRofiId( rofiId.value() );
                }
            }

            rofi::messages::RofiResp resp;
            resp.set_rofiid( rofiId.value_or( 0 ) );
            resp.set_resptype( rofi::messages::RofiCmd::DESCRIPTION );
            auto * description = resp.mutable_rofidescription();
            description->set_jointcount( static_cast< int >( joints.size() ) );
            description->set_connectorcount( static_cast< int >( connectors.size() ) );
            _pub->Publish( resp );
            break;
        }
        case RofiCmd::WAIT_CMD:
        {
            const auto afterWaited =
                    std::chrono::duration< double >( info.simTime ).count()
                    + static_cast< double >( msg.waitcmd().waitms() ) / 1000.0;
            std::lock_guard lock( waitCallbacksMapMutex );
            waitCallbacksMap.emplace(
                    afterWaited,
                    [ this, waitId = msg.waitcmd().waitid() ]() {
                        rofi::messages::RofiResp resp;
                        resp.set_rofiid( rofiId.value_or( 0 ) );
                        resp.set_resptype( rofi::messages::RofiCmd::WAIT_CMD );
                        resp.set_waitid( waitId );
                        _pub->Publish( resp );
                    } );
            break;
        }
        default:
            gzwarn << "Unknown RoFI command type\n";
            break;
    }
}

void RoFIModulePlugin::onJointCmd( const rofi::messages::JointCmd & msg,
                                   gz::sim::EntityComponentManager & ecm )
{
    using rofi::messages::JointCmd;

    int jointIndex = msg.joint();
    if ( jointIndex < 0 || static_cast< size_t >( jointIndex ) >= joints.size() )
    {
        gzwarn << "Invalid joint " << jointIndex << " specified\n";
        return;
    }

    auto & jointData = joints.at( static_cast< size_t >( jointIndex ) );
    switch ( msg.cmdtype() )
    {
        case JointCmd::NO_CMD:
            break;
        case JointCmd::GET_CAPABILITIES:
        {
            rofi::messages::RofiResp resp;
            resp.set_rofiid( rofiId.value_or( 0 ) );
            resp.set_resptype( rofi::messages::RofiCmd::JOINT_CMD );
            auto & jointResp = *resp.mutable_jointresp();
            jointResp.set_joint( jointIndex );
            jointResp.set_resptype( JointCmd::GET_CAPABILITIES );
            auto & capabilities = *jointResp.mutable_capabilities();
            capabilities.set_maxposition( static_cast< float >( jointData.getMaxPosition() ) );
            capabilities.set_minposition( static_cast< float >( jointData.getMinPosition() ) );
            capabilities.set_maxspeed( static_cast< float >( jointData.getMaxVelocity() ) );
            capabilities.set_minspeed( static_cast< float >( jointData.getMinVelocity() ) );
            capabilities.set_maxtorque( static_cast< float >( jointData.getMaxEffort() ) );
            _pub->Publish( resp );
            break;
        }
        case JointCmd::GET_VELOCITY:
            _pub->Publish( getJointRofiResp( JointCmd::GET_VELOCITY,
                                             jointIndex,
                                             static_cast< float >( jointData.velocity( ecm ) ) ) );
            break;
        case JointCmd::SET_VELOCITY:
            setVelocity( jointIndex, msg.setvelocity().velocity() );
            break;
        case JointCmd::GET_POSITION:
            _pub->Publish( getJointRofiResp( JointCmd::GET_POSITION,
                                             jointIndex,
                                             static_cast< float >( jointData.position( ecm ) ) ) );
            break;
        case JointCmd::SET_POS_WITH_SPEED:
            setPositionWithSpeed( jointIndex,
                                  msg.setposwithspeed().position(),
                                  msg.setposwithspeed().speed(),
                                  ecm );
            break;
        case JointCmd::GET_TORQUE:
            _pub->Publish( getJointRofiResp( JointCmd::GET_TORQUE, jointIndex, -1.F ) );
            break;
        case JointCmd::SET_TORQUE:
            setTorque( jointIndex, msg.settorque().torque() );
            break;
        default:
            gzwarn << "Unknown command type " << msg.cmdtype() << " of joint " << jointIndex
                   << "\n";
            break;
    }
}

void RoFIModulePlugin::onConnectorCmd( const rofi::messages::ConnectorCmd & msg )
{
    int connector = msg.connector();
    if ( connector < 0 || static_cast< size_t >( connector ) >= connectors.size() )
    {
        gzwarn << "Invalid connector " << connector << " specified\n";
        return;
    }

    connectors.at( static_cast< size_t >( connector ) ).first->Publish( msg );
}

void RoFIModulePlugin::processWaitCallbacks( const gz::sim::UpdateInfo & info )
{
    const auto now = std::chrono::duration< double >( info.simTime ).count();

    std::multimap< double, std::function< void() > > ready;
    {
        std::lock_guard lock( waitCallbacksMapMutex );
        auto it = waitCallbacksMap.begin();
        while ( it != waitCallbacksMap.end() && it->first <= now )
        {
            ready.insert( ready.end(), waitCallbacksMap.extract( it++ ) );
        }
    }

    for ( auto & callback : ready )
    {
        callback.second();
    }
}

void RoFIModulePlugin::setVelocity( int joint, double velocity )
{
    joints.at( static_cast< size_t >( joint ) ).controller.setTargetVelocity( velocity );
}

void RoFIModulePlugin::setTorque( int joint, double torque )
{
    joints.at( static_cast< size_t >( joint ) ).controller.setTargetForce( torque );
}

void RoFIModulePlugin::setPositionWithSpeed( int joint,
                                             double desiredPosition,
                                             double speed,
                                             const gz::sim::EntityComponentManager & ecm )
{
    joints.at( static_cast< size_t >( joint ) )
            .controller.setTargetPositionWithSpeed( desiredPosition, speed, ecm );
}

} // namespace gazebo

GZ_ADD_PLUGIN( gazebo::RoFIModulePlugin,
               gz::sim::System,
               gz::sim::ISystemConfigure,
               gz::sim::ISystemPreUpdate )
GZ_ADD_PLUGIN_ALIAS( gazebo::RoFIModulePlugin, "rofi::sim::systems::RoFIModulePlugin" )
