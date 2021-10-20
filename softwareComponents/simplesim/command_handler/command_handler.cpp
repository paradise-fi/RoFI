#include "command_handler.hpp"


using namespace rofi::simplesim;
using namespace rofi::messages;
using ModuleStatesPtr = std::shared_ptr< ModuleStates >;

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


std::optional< RofiResp > getJointCapabilities( ModuleStatesPtr moduleStates,
                                                ModuleId moduleId,
                                                int joint )
{
    // TODO get from configuration
    auto resp = getJointResp( moduleId, joint, JointCmd::GET_CAPABILITIES );
    *resp.mutable_jointresp()->mutable_capabilities() = {}; // TODO
    return resp;
}

std::optional< RofiResp > getJointPosition( ModuleStatesPtr moduleStates,
                                            ModuleId moduleId,
                                            int joint )
{
    // TODO get from configuration
    auto position = 0.f; // TODO
    return getJointResp( moduleId, joint, JointCmd::GET_POSITION, position );
}

std::optional< RofiResp > getJointVelocity( ModuleStatesPtr moduleStates,
                                            ModuleId moduleId,
                                            int joint )
{
    // TODO get from configuration
    auto velocity = 0.f; // TODO
    return getJointResp( moduleId, joint, JointCmd::GET_VELOCITY, velocity );
}

std::nullopt_t setJointPosWithSpeed( ModuleStatesPtr moduleStates,
                                     ModuleId moduleId,
                                     int joint,
                                     ModuleStates::JointPositionControl control )
{
    // TODO clamp pos and speed
    assert( moduleStates );
    if ( !moduleStates->setPositionControl( moduleId, joint, control ) ) {
        std::cerr << "Rofi " << moduleId << " doesn't exist or does not have joint " << joint
                  << " (joint command type: " << JointCmd::SET_POS_WITH_SPEED << ")\n";
    }
    return std::nullopt;
}

std::nullopt_t setJointVelocity( ModuleStatesPtr moduleStates,
                                 ModuleId moduleId,
                                 int joint,
                                 ModuleStates::JointVelocityControl control )
{
    // TODO clamp velocity
    assert( moduleStates );
    if ( !moduleStates->setVelocityControl( moduleId, joint, control ) ) {
        std::cerr << "Rofi " << moduleId << " doesn't exist or does not have joint " << joint
                  << " (joint command type: " << JointCmd::SET_VELOCITY << ")\n";
    }
    return std::nullopt;
}


std::optional< RofiResp > getConnectorState( ModuleStatesPtr moduleStates,
                                             ModuleId moduleId,
                                             int connector )
{
    assert( moduleStates );
    if ( auto state = moduleStates->getConnectorState( moduleId, connector ) ) {
        auto resp = getConnectorResp( moduleId, connector, ConnectorCmd::GET_STATE );
        *resp.mutable_connectorresp()->mutable_state() = std::move( *state );
        return resp;
    }
    std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector " << connector
              << " (connector command type: " << ConnectorCmd::GET_STATE << ")\n";
    return std::nullopt;
}

std::nullopt_t extendConnector( ModuleStatesPtr moduleStates, ModuleId moduleId, int connector )
{
    assert( moduleStates );
    // TODO check somewhere for new connections
    if ( !moduleStates->extendConnector( moduleId, connector ) ) {
        std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                  << connector << " (connector command type: " << ConnectorCmd::CONNECT << ")\n";
    }
    return std::nullopt;
}

std::nullopt_t retractConnector( ModuleStatesPtr moduleStates, ModuleId moduleId, int connector )
{
    // TODO can you get a message after disconnect command?
    assert( moduleStates );
    // TODO send disconnect event to both sides
    if ( !moduleStates->retractConnector( moduleId, connector ) ) {
        std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                  << connector << " (connector command type: " << ConnectorCmd::DISCONNECT << ")\n";
    }
    return std::nullopt;
}

std::optional< RofiResp > sendConnectorPacket( ModuleStatesPtr moduleStates,
                                               ModuleId moduleId,
                                               int connector,
                                               Packet packet )
{
    assert( moduleStates );
    if ( auto connectedToOpt = moduleStates->getConnectedTo( moduleId, connector ) ) {
        if ( auto connectedTo = *connectedToOpt ) {
            auto resp = getConnectorResp( connectedTo->moduleId,
                                          connectedTo->connector,
                                          ConnectorCmd::PACKET );
            *resp.mutable_connectorresp()->mutable_packet() = std::move( packet );
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

std::optional< RofiResp > setConnectorPower( ModuleStatesPtr moduleStates,
                                             ModuleId moduleId,
                                             int connector,
                                             ConnectorCmd::Line line,
                                             bool connect )
{
    assert( moduleStates );
    if ( !moduleStates->setConnectorPower( moduleId, connector, line, connect ) ) {
        std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                  << connector << " (connector command type: "
                  << ( connect ? ConnectorCmd::CONNECT_POWER : ConnectorCmd::DISCONNECT_POWER )
                  << ")\n";
    }
    return std::nullopt;
}


std::optional< RofiResp > getModuleDescription( ModuleStatesPtr moduleStates, ModuleId moduleId )
{
    assert( moduleStates );
    if ( auto description = moduleStates->getDescription( moduleId ) ) {
        RofiResp resp;
        resp.set_rofiid( moduleId );
        resp.set_resptype( RofiCmd::DESCRIPTION );
        *resp.mutable_rofidescription() = std::move( *description );
        return resp;
    }
    std::cerr << "Could not get RoFI descriptor (RoFI " << moduleId << " doesn't exist)\n";
    return {};
}


CommandHandler::CommandCallbacks CommandHandler::onJointCmdCallbacks( ModuleId moduleId,
                                                                      const JointCmd & cmd )
{
    switch ( cmd.cmdtype() ) {
        case JointCmd::NO_CMD:
        {
            return {};
        }
        case JointCmd::GET_CAPABILITIES:
        {
            return { .immediate = std::bind( getJointCapabilities,
                                             _moduleStates,
                                             moduleId,
                                             cmd.joint() ) };
        }
        case JointCmd::GET_POSITION:
        {
            return { .immediate = std::bind( getJointPosition,
                                             _moduleStates,
                                             moduleId,
                                             cmd.joint() ) };
        }
        case JointCmd::GET_VELOCITY:
        {
            return { .immediate = std::bind( getJointVelocity,
                                             _moduleStates,
                                             moduleId,
                                             cmd.joint() ) };
        }
        case JointCmd::SET_POS_WITH_SPEED:
        {
            auto pos = cmd.setposwithspeed().position();
            auto speed = cmd.setposwithspeed().speed();
            if ( speed < 0.f ) {
                std::cerr << "Got set position command with negative speed. Setting to zero.\n";
                speed = 0.f;
            }
            auto control = ModuleStates::JointPositionControl{ .position = pos, .speed = speed };

            return { .delayed = std::bind( setJointPosWithSpeed,
                                           _moduleStates,
                                           moduleId,
                                           cmd.joint(),
                                           control ) };
        }
        case JointCmd::SET_VELOCITY:
        {
            auto velocity = cmd.setvelocity().velocity();
            auto control = ModuleStates::JointVelocityControl{ .velocity = velocity };

            return { .delayed = std::bind( setJointVelocity,
                                           _moduleStates,
                                           moduleId,
                                           cmd.joint(),
                                           control ) };
        }
        case JointCmd::GET_TORQUE:
        case JointCmd::SET_TORQUE:
        {
            std::cerr << "Torque joint command type: " << cmd.cmdtype() << "\n";
            return {};
        }
        case JointCmd::ERROR:
        {
            std::cerr << "Error joint command type: " << cmd.cmdtype() << "\n";
            return {};
        }
        default:
        {
            std::cerr << "Unknown joint command type: " << cmd.cmdtype() << "\n";
            return {};
        }
    }
}

CommandHandler::CommandCallbacks CommandHandler::onConnectorCmdCallbacks( ModuleId moduleId,
                                                                          const ConnectorCmd & cmd )
{
    switch ( cmd.cmdtype() ) {
        case ConnectorCmd::NO_CMD:
        {
            return {};
        }
        case ConnectorCmd::GET_STATE:
        {
            return { .immediate = std::bind( getConnectorState,
                                             _moduleStates,
                                             moduleId,
                                             cmd.connector() ) };
        }
        case ConnectorCmd::CONNECT:
        {
            return { .delayed = std::bind( extendConnector,
                                           _moduleStates,
                                           moduleId,
                                           cmd.connector() ) };
        }
        case ConnectorCmd::DISCONNECT:
        {
            return { .delayed = std::bind( extendConnector,
                                           _moduleStates,
                                           moduleId,
                                           cmd.connector() ) };
        }
        case ConnectorCmd::PACKET:
        {
            return { .delayed = std::bind( sendConnectorPacket,
                                           _moduleStates,
                                           moduleId,
                                           cmd.connector(),
                                           cmd.packet() ) };
        }
        case ConnectorCmd::CONNECT_POWER:
        {
            return { .delayed = std::bind( setConnectorPower,
                                           _moduleStates,
                                           moduleId,
                                           cmd.connector(),
                                           cmd.line(),
                                           true ) };
        }
        case ConnectorCmd::DISCONNECT_POWER:
        {
            return { .delayed = std::bind( setConnectorPower,
                                           _moduleStates,
                                           moduleId,
                                           cmd.connector(),
                                           cmd.line(),
                                           false ) };
        }
        default:
        {
            std::cerr << "Unknown connector command type: " << cmd.cmdtype() << "\n";
            return {};
        }
    }
}

CommandHandler::CommandCallbacks CommandHandler::onRofiCmdCallbacks( const RofiCmd & cmd )
{
    switch ( cmd.cmdtype() ) {
        case RofiCmd::NO_CMD:
        {
            return {};
        }
        case RofiCmd::JOINT_CMD:
        {
            return onJointCmdCallbacks( cmd.rofiid(), cmd.jointcmd() );
        }
        case RofiCmd::CONNECTOR_CMD:
        {
            return onConnectorCmdCallbacks( cmd.rofiid(), cmd.connectorcmd() );
        }
        case RofiCmd::DESCRIPTION:
        {
            return { .immediate = std::bind( getModuleDescription, _moduleStates, cmd.rofiid() ) };
        }
        case RofiCmd::WAIT_CMD:
        {
            // TODO
            std::cerr << "Waiting is not yet implemented. Ignoring... (The user code will halt.)\n";
            return {};
        }
        default:
        {
            std::cerr << "Unknown rofi command type: " << cmd.cmdtype() << "\n";
            return {};
        }
    }
}
