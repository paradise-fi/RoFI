#include "command_handler.hpp"


using namespace rofi::simplesim;
using namespace rofi::messages;
using RofiCmdPtr = CommandHandler::RofiCmdPtr;


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


std::optional< RofiResp > getJointCapabilities( const ModuleStates & moduleStates,
                                                const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::GET_CAPABILITIES );

    ModuleId moduleId = rofiCmd.rofiid();
    int joint = rofiCmd.jointcmd().joint();

    // TODO get from configuration
    auto resp = getJointResp( moduleId, joint, JointCmd::GET_CAPABILITIES );
    *resp.mutable_jointresp()->mutable_capabilities() = {}; // TODO
    return resp;
}

std::optional< RofiResp > getJointPosition( const ModuleStates & moduleStates,
                                            const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::GET_POSITION );

    ModuleId moduleId = rofiCmd.rofiid();
    int joint = rofiCmd.jointcmd().joint();

    // TODO get from configuration
    auto position = 0.f; // TODO
    return getJointResp( moduleId, joint, JointCmd::GET_POSITION, position );
}

std::optional< RofiResp > getJointVelocity( const ModuleStates & moduleStates,
                                            const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::JOINT_CMD );
    assert( rofiCmd.jointcmd().cmdtype() == JointCmd::GET_VELOCITY );

    ModuleId moduleId = rofiCmd.rofiid();
    int joint = rofiCmd.jointcmd().joint();

    // TODO get from configuration
    auto velocity = 0.f; // TODO
    return getJointResp( moduleId, joint, JointCmd::GET_VELOCITY, velocity );
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
    if ( speed < 0.f ) {
        std::cerr << "Got set position command with negative speed. Setting to zero.\n";
        speed = 0.f;
    }
    auto control = ModuleStates::JointPositionControl{ .position = pos, .speed = speed };

    // TODO clamp pos and speed
    if ( !moduleStates.setPositionControl( moduleId, joint, control ) ) {
        std::cerr << "Rofi " << moduleId << " doesn't exist or does not have joint " << joint
                  << " (joint command type: " << JointCmd::SET_POS_WITH_SPEED << ")\n";
    }
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
    if ( !moduleStates.setVelocityControl( moduleId, joint, control ) ) {
        std::cerr << "Rofi " << moduleId << " doesn't exist or does not have joint " << joint
                  << " (joint command type: " << JointCmd::SET_VELOCITY << ")\n";
    }
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

    // TODO check somewhere for new connections
    if ( !moduleStates.extendConnector( moduleId, connector ) ) {
        std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                  << connector << " (connector command type: " << ConnectorCmd::CONNECT << ")\n";
    }
    return std::nullopt;
}

std::nullopt_t retractConnector( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::DISCONNECT );

    ModuleId moduleId = rofiCmd.rofiid();
    int connector = rofiCmd.connectorcmd().connector();

    // TODO can you get a message after disconnect command?

    // TODO send disconnect event to both sides
    if ( !moduleStates.retractConnector( moduleId, connector ) ) {
        std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                  << connector << " (connector command type: " << ConnectorCmd::DISCONNECT << ")\n";
    }
    return std::nullopt;
}

std::optional< RofiResp > sendConnectorPacket( ModuleStates & moduleStates,
                                               const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::PACKET );

    ModuleId moduleId = rofiCmd.rofiid();
    int connector = rofiCmd.connectorcmd().connector();

    const Packet & packet = rofiCmd.connectorcmd().packet();

    if ( auto connectedToOpt = moduleStates.getConnectedTo( moduleId, connector ) ) {
        if ( auto connectedTo = *connectedToOpt ) {
            auto resp = getConnectorResp( connectedTo->moduleId,
                                          connectedTo->connector,
                                          ConnectorCmd::PACKET );
            *resp.mutable_connectorresp()->mutable_packet() = packet;
            return resp;
        }

        std::cerr << "Connector " << connector << "of module " << moduleId
                  << " is not connected (connector command type: " << ConnectorCmd::PACKET << ")\n";
        return std::nullopt;
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector " << connector
              << " (connector command type: " << ConnectorCmd::PACKET << ")\n";
    return std::nullopt;
}

std::optional< RofiResp > setConnectorPower( ModuleStates & moduleStates, const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::CONNECT_POWER );

    ModuleId moduleId = rofiCmd.rofiid();
    int connector = rofiCmd.connectorcmd().connector();
    ConnectorCmd::Line line = rofiCmd.connectorcmd().line();

    if ( !moduleStates.setConnectorPower( moduleId, connector, line, true ) ) {
        std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                  << connector << " (connector command type: " << ConnectorCmd::CONNECT_POWER
                  << ")\n";
    }
    return std::nullopt;
}

std::optional< RofiResp > setDisconnectorPower( ModuleStates & moduleStates,
                                                const RofiCmd & rofiCmd )
{
    assert( rofiCmd.cmdtype() == RofiCmd::CONNECTOR_CMD );
    assert( rofiCmd.connectorcmd().cmdtype() == ConnectorCmd::DISCONNECT_POWER );

    ModuleId moduleId = rofiCmd.rofiid();
    int connector = rofiCmd.connectorcmd().connector();
    ConnectorCmd::Line line = rofiCmd.connectorcmd().line();

    if ( !moduleStates.setConnectorPower( moduleId, connector, line, false ) ) {
        std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                  << connector << " (connector command type: " << ConnectorCmd::DISCONNECT_POWER
                  << ")\n";
    }
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
    return {};
}


CommandHandler::CommandCallbacks CommandHandler::onJointCmdCallbacks( JointCmd::Type cmdType )
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

CommandHandler::CommandCallbacks CommandHandler::onConnectorCmdCallbacks(
        ConnectorCmd::Type cmdType )
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
            return { .delayed = sendConnectorPacket };

        case ConnectorCmd::CONNECT_POWER:
            return { .delayed = setConnectorPower };

        case ConnectorCmd::DISCONNECT_POWER:
            return { .delayed = setDisconnectorPower };

        default:
            std::cerr << "Unknown connector command type: " << cmdType << "\n";
            return {};
    }
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
            return { .immediate = getModuleDescription };

        case RofiCmd::WAIT_CMD:
            // TODO
            std::cerr << "Waiting is not yet implemented. Ignoring... (The user code will halt.)\n";
            return {};

        default:
            std::cerr << "Unknown rofi command type: " << cmd.cmdtype() << "\n";
            return {};
    }
}
