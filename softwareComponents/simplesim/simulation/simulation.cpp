#include "simulation.hpp"

#include <iostream>


using namespace rofi::simplesim;
using namespace rofi::messages;


std::vector< RofiResp > Simulation::simulateOneIteration()
{
    // TODO
    return {};
}

std::optional< RofiResp > Simulation::processRofiCommand( const RofiCmd & cmd )
{
    switch ( cmd.cmdtype() ) {
        case RofiCmd::NO_CMD:
        {
            return {};
        }
        case RofiCmd::JOINT_CMD:
        {
            return processJointCommand( cmd.rofiid(), cmd.jointcmd() );
        }
        case RofiCmd::CONNECTOR_CMD:
        {
            return processConnectorCommand( cmd.rofiid(), cmd.connectorcmd() );
        }
        case RofiCmd::DESCRIPTION:
        {
            if ( auto description = _moduleStates.getDescription( cmd.rofiid() ) ) {
                RofiResp resp;
                resp.set_rofiid( cmd.rofiid() );
                resp.set_resptype( cmd.cmdtype() );
                *resp.mutable_rofidescription() = std::move( *description );
                return resp;
            }
            std::cerr << "Could not get RoFI descriptor (RoFI " << cmd.rofiid()
                      << " doesn't exist)\n";
            return {};
        }
        case RofiCmd::WAIT_CMD:
        {
            // TODO
            return {};
        }
        default:
        {
            std::cerr << "Unknown rofi command type: " << cmd.cmdtype() << "\n";
            return {};
        }
    }
}

std::optional< RofiResp > Simulation::processJointCommand( ModuleId moduleId, const JointCmd & cmd )
{
    switch ( cmd.cmdtype() ) {
        case JointCmd::NO_CMD:
        {
            return {};
        }
        case JointCmd::GET_CAPABILITIES:
        {
            // TODO get from configuration
            auto resp = getJointResp( moduleId, cmd.joint(), cmd.cmdtype() );
            *resp.mutable_jointresp()->mutable_capabilities() = {}; // TODO
            return resp;
        }
        case JointCmd::GET_POSITION:
        {
            // TODO get from configuration
            auto position = 0.f; // TODO
            return getJointResp( moduleId, cmd.joint(), cmd.cmdtype(), position );
        }
        case JointCmd::GET_VELOCITY:
        {
            // TODO get from configuration
            auto velocity = 0.f; // TODO
            return getJointResp( moduleId, cmd.joint(), cmd.cmdtype(), velocity );
        }
        case JointCmd::SET_POS_WITH_SPEED:
        {
            auto pos = cmd.setposwithspeed().position();
            auto speed = cmd.setposwithspeed().speed();
            if ( pos < 0.f ) {
                std::cerr << "Got set position command with negative speed. Setting to zero\n";
                pos = 0.f;
            }
            // TODO clamp pos and speed

            using JointPositionControl = ModuleStates::JointPositionControl;
            if ( !_moduleStates.setPositionControl( moduleId,
                                                    cmd.joint(),
                                                    JointPositionControl{ .position = pos,
                                                                          .speed = speed } ) )
            {
                std::cerr << "Rofi " << moduleId << " doesn't exist or does not have joint "
                          << cmd.joint() << " (joint command type: " << cmd.cmdtype() << ")\n";
            }
            return {};
        }
        case JointCmd::SET_VELOCITY:
        {
            auto velocity = cmd.setvelocity().velocity();
            // TODO clamp velocity

            using JointVelocityControl = ModuleStates::JointVelocityControl;
            if ( !_moduleStates.setVelocityControl( moduleId,
                                                    cmd.joint(),
                                                    JointVelocityControl{ .velocity = velocity } ) )
            {
                std::cerr << "Rofi " << moduleId << " doesn't exist or does not have joint "
                          << cmd.joint() << " (joint command type: " << cmd.cmdtype() << ")\n";
            }
            return {};
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

std::optional< RofiResp > Simulation::processConnectorCommand( ModuleId moduleId,
                                                               const ConnectorCmd & cmd )
{
    switch ( cmd.cmdtype() ) {
        case ConnectorCmd::NO_CMD:
        {
            return {};
        }
        case ConnectorCmd::GET_STATE:
        {
            if ( auto state = _moduleStates.getConnectorState( moduleId, cmd.connector() ) ) {
                auto resp = getConnectorResp( moduleId, cmd.connector(), cmd.cmdtype() );
                *resp.mutable_connectorresp()->mutable_state() = std::move( *state );
                return resp;
            }
            std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                      << cmd.connector() << " (connector command type: " << cmd.cmdtype() << ")\n";
            return {};
        }
        case ConnectorCmd::CONNECT:
        {
            // TODO check somewhere for new connections
            if ( !_moduleStates.extendConnector( moduleId, cmd.connector() ) ) {
                std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                          << cmd.connector() << " (connector command type: " << cmd.cmdtype()
                          << ")\n";
            }
            return {};
        }
        case ConnectorCmd::DISCONNECT:
        {
            // TODO send disconnect event to both sides
            if ( !_moduleStates.retractConnector( moduleId, cmd.connector() ) ) {
                std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                          << cmd.connector() << " (connector command type: " << cmd.cmdtype()
                          << ")\n";
            }
            return {};
        }
        case ConnectorCmd::PACKET:
        {
            if ( auto state = _moduleStates.getConnectorState( moduleId, cmd.connector() ) ) {
                auto resp = getConnectorResp( moduleId, cmd.connector(), cmd.cmdtype() );
                *resp.mutable_connectorresp()->mutable_state() = std::move( *state );
                return resp;
            }
            if ( auto connectedToOpt = _moduleStates.getConnectedTo( moduleId, cmd.connector() ) ) {
                if ( auto connectedTo = *connectedToOpt ) {
                    auto resp = getConnectorResp( connectedTo->moduleId,
                                                  connectedTo->connector,
                                                  cmd.cmdtype() );
                    *resp.mutable_connectorresp()->mutable_packet() = cmd.packet();
                    return resp;
                }

                std::cerr << "Connector " << cmd.connector() << "of module " << moduleId
                          << " is not connected (connector command type: " << cmd.cmdtype()
                          << ")\n";
                return {};
            }
            std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                      << cmd.connector() << " (connector command type: " << cmd.cmdtype() << ")\n";
            return {};
        }
        case ConnectorCmd::CONNECT_POWER:
        {
            if ( !_moduleStates.setConnectorPower( moduleId, cmd.connector(), cmd.line(), true ) ) {
                std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                          << cmd.connector() << " (connector command type: " << cmd.cmdtype()
                          << ")\n";
            }
            return {};
        }
        case ConnectorCmd::DISCONNECT_POWER:
        {
            if ( !_moduleStates.setConnectorPower( moduleId, cmd.connector(), cmd.line(), false ) )
            {
                std::cerr << "Rofi " << moduleId << " doesn't exist or does not have connector "
                          << cmd.connector() << " (connector command type: " << cmd.cmdtype()
                          << ")\n";
            }
            return {};
        }
        default:
        {
            std::cerr << "Unknown connector command type: " << cmd.cmdtype() << "\n";
            return {};
        }
    }
}

RofiResp Simulation::getJointResp( ModuleId moduleId, int joint, JointCmd::Type type, float value )
{
    RofiResp rofiResp;
    rofiResp.set_rofiid( moduleId );
    rofiResp.set_resptype( RofiCmd::JOINT_CMD );
    auto & jointResp = *rofiResp.mutable_jointresp();
    jointResp.set_joint( joint );
    jointResp.set_resptype( type );
    jointResp.set_value( value );
    return rofiResp;
}

RofiResp Simulation::getConnectorResp( ModuleId moduleId, int connector, ConnectorCmd::Type type )
{
    RofiResp rofiResp;
    rofiResp.set_rofiid( moduleId );
    rofiResp.set_resptype( RofiCmd::CONNECTOR_CMD );
    auto & connectorResp = *rofiResp.mutable_connectorresp();
    connectorResp.set_connector( connector );
    connectorResp.set_resptype( type );
    return rofiResp;
}
