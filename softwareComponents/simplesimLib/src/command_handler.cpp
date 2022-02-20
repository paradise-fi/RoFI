#include "simplesim/command_handler.hpp"


namespace rofi::simplesim
{
using namespace rofi::messages;

RofiResp getJointResp( ModuleId moduleId,
                       int joint,
                       JointCmd::Type type,
                       std::optional< float > value = {} )
{
    RofiResp rofiResp;
    rofiResp.set_rofiid( moduleId );
    rofiResp.set_resptype( RofiCmd::JOINT_CMD );
    auto & jointResp = *rofiResp.mutable_jointresp();
    jointResp.set_joint( joint );
    jointResp.set_resptype( type );
    if ( value ) {
        jointResp.set_value( *value );
    }
    return rofiResp;
}

RofiResp getConnectorResp( ModuleId moduleId, int connector, ConnectorCmd::Type type )
{
    RofiResp rofiResp;
    rofiResp.set_rofiid( moduleId );
    rofiResp.set_resptype( RofiCmd::CONNECTOR_CMD );
    auto & connectorResp = *rofiResp.mutable_connectorresp();
    connectorResp.set_connector( connector );
    connectorResp.set_resptype( type );
    return rofiResp;
}

RofiResp CommandHandler::Connector::getRofiResp( ConnectorCmd::Type type ) const
{
    return getConnectorResp( moduleId, connector, type );
}


std::optional< RofiResp > getJointCapabilities( const ModuleStates & moduleStates,
                                                const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::GET_CAPABILITIES );

    ModuleId moduleId = rofiCmd.rofiid();
    int joint = rofiCmd.jointcmd().joint();

    if ( auto capabilities = moduleStates.getJointCapabilities( moduleId, joint ) ) {
        auto resp = getJointResp( moduleId, joint, JointCmd::GET_CAPABILITIES );
        *resp.mutable_jointresp()->mutable_capabilities() = *capabilities;
        return resp;
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have joint " << joint
              << " (joint command type: " << JointCmd::GET_CAPABILITIES << ")\n";
    return std::nullopt;
}

std::optional< RofiResp > getJointPosition( const ModuleStates & moduleStates,
                                            const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::GET_POSITION );

    ModuleId moduleId = rofiCmd.rofiid();
    int joint = rofiCmd.jointcmd().joint();

    if ( auto position = moduleStates.getJointPosition( moduleId, joint ) ) {
        return getJointResp( moduleId, joint, JointCmd::GET_POSITION, *position );
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have joint " << joint
              << " (joint command type: " << JointCmd::GET_POSITION << ")\n";
    return std::nullopt;
}

std::optional< RofiResp > getJointVelocity( const ModuleStates & moduleStates,
                                            const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::GET_VELOCITY );

    ModuleId moduleId = rofiCmd.rofiid();
    int joint = rofiCmd.jointcmd().joint();

    if ( auto velocity = moduleStates.getJointVelocity( moduleId, joint ) ) {
        return getJointResp( moduleId, joint, JointCmd::GET_VELOCITY, *velocity );
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have joint " << joint
              << " (joint command type: " << JointCmd::GET_VELOCITY << ")\n";
    return std::nullopt;
}

std::nullopt_t setJointPosWithSpeed( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::SET_POS_WITH_SPEED );

    ModuleId moduleId = rofiCmd.rofiid();
    int joint = rofiCmd.jointcmd().joint();

    const auto & setPosWithSpeed = rofiCmd.jointcmd().setposwithspeed();
    auto pos = setPosWithSpeed.position();
    auto speed = setPosWithSpeed.speed();
    if ( auto currentPosition = moduleStates.getJointPosition( moduleId, joint ) ) {
        if ( speed < 0.f ) {
            std::cerr << "Got set position command with negative speed. Setting to zero.\n";
            speed = 0.f;
        }
        auto velocity = std::copysign( speed, pos - *currentPosition );

        // TODO clamp pos and speed
        auto control = ModuleStates::JointPositionControl{ .position = pos, .velocity = velocity };
        if ( moduleStates.setPositionControl( moduleId, joint, control ) ) {
            return std::nullopt;
        }
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have joint " << joint
              << " (joint command type: " << JointCmd::SET_POS_WITH_SPEED << ")\n";
    return std::nullopt;
}

std::nullopt_t setJointVelocity( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::SET_VELOCITY );

    ModuleId moduleId = rofiCmd.rofiid();
    int joint = rofiCmd.jointcmd().joint();

    auto velocity = rofiCmd.jointcmd().setvelocity().velocity();
    auto control = ModuleStates::JointVelocityControl{ .velocity = velocity };

    // TODO clamp velocity
    if ( moduleStates.setVelocityControl( moduleId, joint, control ) ) {
        return std::nullopt;
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have joint " << joint
              << " (joint command type: " << JointCmd::SET_VELOCITY << ")\n";
    return std::nullopt;
}


std::optional< RofiResp > getConnectorState( const ModuleStates & moduleStates,
                                             const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::GET_STATE );

    ModuleId moduleId = rofiCmd.rofiid();
    int connector = rofiCmd.connectorcmd().connector();

    if ( auto state = moduleStates.getConnectorState( moduleId, connector ) ) {
        auto resp = getConnectorResp( moduleId, connector, ConnectorCmd::GET_STATE );
        *resp.mutable_connectorresp()->mutable_state() = std::move( *state );
        return resp;
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector " << connector
              << " (connector command type: " << ConnectorCmd::GET_STATE << ")\n";
    return std::nullopt;
}

std::nullopt_t extendConnector( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::CONNECT );

    ModuleId moduleId = rofiCmd.rofiid();
    int connector = rofiCmd.connectorcmd().connector();

    if ( moduleStates.extendConnector( moduleId, connector ) ) {
        return std::nullopt;
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector " << connector
              << " (connector command type: " << ConnectorCmd::CONNECT << ")\n";
    return std::nullopt;
}

CommandHandler::DelayedEvent retractConnector( ModuleStates & moduleStates,
                                               const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::DISCONNECT );

    ModuleId moduleId = rofiCmd.rofiid();
    int connector = rofiCmd.connectorcmd().connector();

    if ( auto connectedTo = moduleStates.retractConnector( moduleId, connector ) ) {
        if ( auto otherConnector = *connectedTo ) {
            return CommandHandler::DisconnectEvent{
                    .first = CommandHandler::Connector{ .moduleId = moduleId,
                                                        .connector = connector },
                    .second = CommandHandler::Connector{ .moduleId = otherConnector->moduleId,
                                                         .connector = otherConnector->connector } };
        }
        return std::nullopt;
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector " << connector
              << " (connector command type: " << ConnectorCmd::DISCONNECT << ")\n";
    return std::nullopt;
}

CommandHandler::DelayedEvent sendConnectorPacket( ModuleStates & moduleStates,
                                                  const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::PACKET );

    ModuleId moduleId = rofiCmd.rofiid();
    int connector = rofiCmd.connectorcmd().connector();

    if ( auto connectedToOpt = moduleStates.getConnectedTo( moduleId, connector ) ) {
        if ( auto connectedTo = *connectedToOpt ) {
            return CommandHandler::SendPacketEvent{
                    .receiver = CommandHandler::Connector{ .moduleId = connectedTo->moduleId,
                                                           .connector = connectedTo->connector },
                    .packet = rofiCmd.connectorcmd().packet() };
        }

        std::cerr << "Connector " << connector << "of module " << moduleId
                  << " is not connected (connector command type: " << ConnectorCmd::PACKET << ")\n";
        return std::nullopt;
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector " << connector
              << " (connector command type: " << ConnectorCmd::PACKET << ")\n";
    return std::nullopt;
}

std::nullopt_t connectPower( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::CONNECT_POWER );

    ModuleId moduleId = rofiCmd.rofiid();
    int connector = rofiCmd.connectorcmd().connector();
    ConnectorCmd::Line line = rofiCmd.connectorcmd().line();

    if ( moduleStates.setConnectorPower( moduleId, connector, line, true ) ) {
        return std::nullopt;
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector " << connector
              << " (connector command type: " << ConnectorCmd::CONNECT_POWER << ", line: " << line
              << ")\n";
    return std::nullopt;
}

std::nullopt_t disconnectPower( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::DISCONNECT_POWER );

    ModuleId moduleId = rofiCmd.rofiid();
    int connector = rofiCmd.connectorcmd().connector();
    ConnectorCmd::Line line = rofiCmd.connectorcmd().line();

    if ( moduleStates.setConnectorPower( moduleId, connector, line, false ) ) {
        return std::nullopt;
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector " << connector
              << " (connector command type: " << ConnectorCmd::DISCONNECT_POWER << ")\n";
    return std::nullopt;
}


std::optional< RofiResp > getModuleDescription( const ModuleStates & moduleStates,
                                                const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::DESCRIPTION );

    ModuleId moduleId = rofiCmd.rofiid();

    if ( auto description = moduleStates.getDescription( moduleId ) ) {
        RofiResp resp;
        resp.set_rofiid( moduleId );
        resp.set_resptype( RofiCmd::DESCRIPTION );
        *resp.mutable_rofidescription() = std::move( *description );
        return resp;
    }
    std::cerr << "Could not get RoFI descriptor (RoFI " << moduleId << " doesn't exist)\n";
    return std::nullopt;
}


CommandHandler::CommandCallbacks onJointCmdCallbacks( JointCmd::Type cmdType )
{
    switch ( cmdType ) {
        case JointCmd::NO_CMD:
            return {};

        case JointCmd::GET_CAPABILITIES:
            return { .immediate = getJointCapabilities, .delayed = {} };

        case JointCmd::GET_POSITION:
            return { .immediate = getJointPosition, .delayed = {} };

        case JointCmd::GET_VELOCITY:
            return { .immediate = getJointVelocity, .delayed = {} };

        case JointCmd::SET_POS_WITH_SPEED:
            return { .immediate = {}, .delayed = setJointPosWithSpeed };

        case JointCmd::SET_VELOCITY:
            return { .immediate = {}, .delayed = setJointVelocity };

        case JointCmd::GET_TORQUE:
        case JointCmd::SET_TORQUE:
            std::cerr << "Torque joint command type: " << cmdType << "\n";
            return {};

        case JointCmd::ERROR:
            std::cerr << "Error joint command type: " << cmdType << "\n";
            return {};

        default:
            std::cerr << "Unknown joint command type: " << cmdType << "\n";
            return {};
    }
}

CommandHandler::CommandCallbacks onConnectorCmdCallbacks( ConnectorCmd::Type cmdType )
{
    switch ( cmdType ) {
        case ConnectorCmd::NO_CMD:
            return {};

        case ConnectorCmd::GET_STATE:
            return { .immediate = getConnectorState, .delayed = {} };

        case ConnectorCmd::CONNECT:
            return { .immediate = {}, .delayed = extendConnector };

        case ConnectorCmd::DISCONNECT:
            return { .immediate = {}, .delayed = retractConnector };

        case ConnectorCmd::PACKET:
            return { .immediate = {}, .delayed = sendConnectorPacket };

        case ConnectorCmd::CONNECT_POWER:
            return { .immediate = {}, .delayed = connectPower };

        case ConnectorCmd::DISCONNECT_POWER:
            return { .immediate = {}, .delayed = disconnectPower };

        default:
            std::cerr << "Unknown connector command type: " << cmdType << "\n";
            return {};
    }
}

void CommandHandler::onWaitCmd( const RofiCmd & cmd )
{
    assert( cmd.cmdtype() == RofiCmd::WAIT_CMD );

    auto waitDuration = std::chrono::milliseconds( cmd.waitcmd().waitms() );
    _waitHandler->registerWait( cmd.rofiid(), cmd.waitcmd().waitid(), waitDuration );
}

CommandHandler::CommandCallbacks CommandHandler::onRofiCmdCallbacks( const RofiCmd & cmd )
{
    switch ( cmd.cmdtype() ) {
        case RofiCmd::NO_CMD:
            return {};

        case RofiCmd::JOINT_CMD:
            return onJointCmdCallbacks( cmd.jointcmd().cmdtype() );

        case RofiCmd::CONNECTOR_CMD:
            return onConnectorCmdCallbacks( cmd.connectorcmd().cmdtype() );

        case RofiCmd::DESCRIPTION:
            return { .immediate = getModuleDescription, .delayed = {} };

        case RofiCmd::WAIT_CMD:
            this->onWaitCmd( cmd );
            return {};

        default:
            std::cerr << "Unknown rofi command type: " << cmd.cmdtype() << "\n";
            return {};
    }
}

std::optional< RofiResp > CommandHandler::onRofiCmd( const CommandHandler::RofiCmdPtr & rofiCmdPtr )
{
    assert( rofiCmdPtr );
    assert( _moduleStates );

    auto callbacks = onRofiCmdCallbacks( *rofiCmdPtr );

    if ( callbacks.delayed ) {
        _rofiCmdCallbacks->emplace_back( std::move( callbacks.delayed ), rofiCmdPtr );
    }

    if ( callbacks.immediate ) {
        return callbacks.immediate( *_moduleStates, *rofiCmdPtr );
    }
    return std::nullopt;
}

} // namespace rofi::simplesim
