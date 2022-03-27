#include "simplesim/command_handler.hpp"


namespace rofi::simplesim
{
using namespace rofi::messages;

std::optional< RofiResp > getJointCapabilities( const ModuleStates & moduleStates,
                                                const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::GET_CAPABILITIES );

    auto joint = Joint{ .moduleId = rofiCmd.rofiid(), .jointIdx = rofiCmd.jointcmd().joint() };

    if ( auto capabilities = moduleStates.getJointCapabilities( joint ) ) {
        auto resp = joint.getRofiResp( JointCmd::GET_CAPABILITIES );
        *resp.mutable_jointresp()->mutable_capabilities() = *capabilities;
        return resp;
    }
    std::cerr << "Rofi " << joint.moduleId << " doesn't exist or does not have joint "
              << joint.jointIdx << " (joint command type: " << JointCmd::GET_CAPABILITIES << ")\n";
    return std::nullopt;
}

std::optional< RofiResp > getJointPosition( const ModuleStates & moduleStates,
                                            const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::GET_POSITION );

    auto joint = Joint{ .moduleId = rofiCmd.rofiid(), .jointIdx = rofiCmd.jointcmd().joint() };

    if ( auto position = moduleStates.getJointPosition( joint ) ) {
        return joint.getRofiResp( JointCmd::GET_POSITION, *position );
    }
    std::cerr << "Rofi " << joint.moduleId << " doesn't exist or does not have joint "
              << joint.jointIdx << " (joint command type: " << JointCmd::GET_POSITION << ")\n";
    return std::nullopt;
}

std::optional< RofiResp > getJointVelocity( const ModuleStates & moduleStates,
                                            const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::GET_VELOCITY );

    auto joint = Joint{ .moduleId = rofiCmd.rofiid(), .jointIdx = rofiCmd.jointcmd().joint() };

    if ( auto velocity = moduleStates.getJointVelocity( joint ) ) {
        return joint.getRofiResp( JointCmd::GET_VELOCITY, *velocity );
    }
    std::cerr << "Rofi " << joint.moduleId << " doesn't exist or does not have joint "
              << joint.jointIdx << " (joint command type: " << JointCmd::GET_VELOCITY << ")\n";
    return std::nullopt;
}

std::nullopt_t setJointPosWithSpeed( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::SET_POS_WITH_SPEED );

    auto joint = Joint{ .moduleId = rofiCmd.rofiid(), .jointIdx = rofiCmd.jointcmd().joint() };

    const auto & setPosWithSpeed = rofiCmd.jointcmd().setposwithspeed();
    auto pos = setPosWithSpeed.position();
    auto speed = setPosWithSpeed.speed();
    if ( auto currentPosition = moduleStates.getJointPosition( joint ) ) {
        if ( speed < 0.f ) {
            std::cerr << "Got set position command with negative speed. Setting to zero.\n";
            speed = 0.f;
        }
        auto velocity = std::copysign( speed, pos - *currentPosition );

        // TODO clamp pos and speed
        auto control = ModuleStates::JointPositionControl{ .position = pos, .velocity = velocity };
        if ( moduleStates.setPositionControl( joint, control ) ) {
            return std::nullopt;
        }
    }
    std::cerr << "Rofi " << joint.moduleId << " doesn't exist or does not have joint "
              << joint.jointIdx << " (joint command type: " << JointCmd::SET_POS_WITH_SPEED
              << ")\n";
    return std::nullopt;
}

std::nullopt_t setJointVelocity( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::SET_VELOCITY );

    auto joint = Joint{ .moduleId = rofiCmd.rofiid(), .jointIdx = rofiCmd.jointcmd().joint() };

    auto velocity = rofiCmd.jointcmd().setvelocity().velocity();
    auto control = ModuleStates::JointVelocityControl{ .velocity = velocity };

    // TODO clamp velocity
    if ( moduleStates.setVelocityControl( joint, control ) ) {
        return std::nullopt;
    }
    std::cerr << "Rofi " << joint.moduleId << " doesn't exist or does not have joint "
              << joint.jointIdx << " (joint command type: " << JointCmd::SET_VELOCITY << ")\n";
    return std::nullopt;
}


std::optional< RofiResp > getConnectorState( const ModuleStates & moduleStates,
                                             const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::GET_STATE );

    auto connector = Connector{ .moduleId = rofiCmd.rofiid(),
                                .connIdx = rofiCmd.connectorcmd().connector() };

    if ( auto state = moduleStates.getConnectorState( connector ) ) {
        auto resp = connector.getRofiResp( ConnectorCmd::GET_STATE );
        *resp.mutable_connectorresp()->mutable_state() = std::move( *state );
        return resp;
    }
    std::cerr << "Rofi " << connector.moduleId << " doesn't exist or does not have connector "
              << connector.connIdx << " (connector command type: " << ConnectorCmd::GET_STATE
              << ")\n";
    return std::nullopt;
}

std::nullopt_t extendConnector( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::CONNECT );

    auto connector = Connector{ .moduleId = rofiCmd.rofiid(),
                                .connIdx = rofiCmd.connectorcmd().connector() };

    if ( moduleStates.extendConnector( connector ) ) {
        return std::nullopt;
    }
    std::cerr << "Rofi " << connector.moduleId << " doesn't exist or does not have connector "
              << connector.connIdx << " (connector command type: " << ConnectorCmd::CONNECT
              << ")\n";
    return std::nullopt;
}

CommandHandler::DelayedEvent retractConnector( ModuleStates & moduleStates,
                                               const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::DISCONNECT );

    auto connector = Connector{ .moduleId = rofiCmd.rofiid(),
                                .connIdx = rofiCmd.connectorcmd().connector() };

    if ( auto connectedTo = moduleStates.retractConnector( connector ) ) {
        if ( auto otherConnector = *connectedTo ) {
            return CommandHandler::DisconnectEvent{ .first = connector,
                                                    .second = otherConnector->connector };
        }
        return std::nullopt;
    }
    std::cerr << "Rofi " << connector.moduleId << " doesn't exist or does not have connector "
              << connector.connIdx << " (connector command type: " << ConnectorCmd::DISCONNECT
              << ")\n";
    return std::nullopt;
}

std::optional< PacketFilter::SendPacketData > sendConnectorPacket(
        const ModuleStates & moduleStates,
        const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::PACKET );

    auto connector = Connector{ .moduleId = rofiCmd.rofiid(),
                                .connIdx = rofiCmd.connectorcmd().connector() };

    if ( auto connectedToOpt = moduleStates.getConnectedTo( connector ) ) {
        if ( auto connectedTo = *connectedToOpt ) {
            return PacketFilter::SendPacketData{ .sender = connector,
                                                 .receiver = connectedTo->connector,
                                                 .packet = rofiCmd.connectorcmd().packet() };
        }

        std::cerr << "Connector " << connector.connIdx << "of module " << connector.moduleId
                  << " is not connected (connector command type: " << ConnectorCmd::PACKET << ")\n";
        return std::nullopt;
    }
    std::cerr << "Rofi " << connector.moduleId << " doesn't exist or does not have connector "
              << connector.connIdx << " (connector command type: " << ConnectorCmd::PACKET << ")\n";
    return std::nullopt;
}

std::nullopt_t connectPower( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::CONNECT_POWER );

    auto connector = Connector{ .moduleId = rofiCmd.rofiid(),
                                .connIdx = rofiCmd.connectorcmd().connector() };
    ConnectorCmd::Line line = rofiCmd.connectorcmd().line();

    if ( moduleStates.setConnectorPower( connector, line, true ) ) {
        return std::nullopt;
    }
    std::cerr << "Rofi " << connector.moduleId << " doesn't exist or does not have connector "
              << connector.connIdx << " (connector command type: " << ConnectorCmd::CONNECT_POWER
              << ", line: " << line << ")\n";
    return std::nullopt;
}

std::nullopt_t disconnectPower( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::DISCONNECT_POWER );

    auto connector = Connector{ .moduleId = rofiCmd.rofiid(),
                                .connIdx = rofiCmd.connectorcmd().connector() };
    ConnectorCmd::Line line = rofiCmd.connectorcmd().line();

    if ( moduleStates.setConnectorPower( connector, line, false ) ) {
        return std::nullopt;
    }
    std::cerr << "Rofi " << connector.moduleId << " doesn't exist or does not have connector "
              << connector.connIdx << " (connector command type: " << ConnectorCmd::DISCONNECT_POWER
              << ")\n";
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
            return { .immediate = getJointCapabilities };

        case JointCmd::GET_POSITION:
            return { .immediate = getJointPosition };

        case JointCmd::GET_VELOCITY:
            return { .immediate = getJointVelocity };

        case JointCmd::SET_POS_WITH_SPEED:
            return { .delayed = setJointPosWithSpeed };

        case JointCmd::SET_VELOCITY:
            return { .delayed = setJointVelocity };

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
            return { .immediate = getConnectorState };

        case ConnectorCmd::CONNECT:
            return { .delayed = extendConnector };

        case ConnectorCmd::DISCONNECT:
            return { .delayed = retractConnector };

        case ConnectorCmd::PACKET:
            return { .delayedDataType = CommandHandler::DelayedDataType::SendPacket };

        case ConnectorCmd::CONNECT_POWER:
            return { .delayed = connectPower };

        case ConnectorCmd::DISCONNECT_POWER:
            return { .delayed = disconnectPower };

        default:
            std::cerr << "Unknown connector command type: " << cmdType << "\n";
            return {};
    }
}

CommandHandler::CommandCallbacks getOnRofiCmdCallbacks( const RofiCmd & cmd )
{
    switch ( cmd.cmdtype() ) {
        case RofiCmd::NO_CMD:
            return {};

        case RofiCmd::JOINT_CMD:
            return onJointCmdCallbacks( cmd.jointcmd().cmdtype() );

        case RofiCmd::CONNECTOR_CMD:
            return onConnectorCmdCallbacks( cmd.connectorcmd().cmdtype() );

        case RofiCmd::DESCRIPTION:
            return { .immediate = getModuleDescription };

        case RofiCmd::WAIT_CMD:
            return { .delayedDataType = CommandHandler::DelayedDataType::Wait };

        default:
            std::cerr << "Unknown rofi command type: " << cmd.cmdtype() << "\n";
            return {};
    }
}

void CommandHandler::onDelayedData( CommandHandler::DelayedDataType type, const RofiCmd & cmd )
{
    switch ( type ) {
        case CommandHandler::DelayedDataType::None:
            return;
        case CommandHandler::DelayedDataType::SendPacket:
        {
            assert( cmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
            assert( cmd.connectorcmd().cmdtype() == ConnectorCmd::PACKET );
            if ( auto packetData = sendConnectorPacket( std::as_const( *_moduleStates ), cmd ) ) {
                _packetFilter->registerPacket( std::move( *packetData ) );
            }
            return;
        }
        case CommandHandler::DelayedDataType::Wait:
        {
            assert( cmd.cmdtype() == RofiCmd::WAIT_CMD );
            using WaitData = CommandHandler::WaitData;
            auto waitData = WaitData{ .moduleId = cmd.rofiid(), .waitId = cmd.waitcmd().waitid() };
            auto waitTime = std::chrono::milliseconds( cmd.waitcmd().waitms() );
            _waitHandler->registerDelayedData( waitData, waitTime );
            return;
        }
    }
    assert( false );
}

std::optional< RofiResp > CommandHandler::onRofiCmd( const CommandHandler::RofiCmdPtr & rofiCmdPtr )
{
    assert( rofiCmdPtr );
    assert( _moduleStates );

    auto callbacks = getOnRofiCmdCallbacks( *rofiCmdPtr );

    onDelayedData( callbacks.delayedDataType, *rofiCmdPtr );

    if ( callbacks.delayed ) {
        _rofiCmdCallbacks->emplace_back( std::move( callbacks.delayed ), rofiCmdPtr );
    }

    if ( callbacks.immediate ) {
        return callbacks.immediate( *_moduleStates, *rofiCmdPtr );
    }
    return std::nullopt;
}

} // namespace rofi::simplesim
