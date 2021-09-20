#include "simulation.hpp"

#include <iostream>


using namespace rofi::simplesim;

using RofiId = Simulation::RofiId;


RofiId Simulation::addModule()
{
    // TODO
    return {};
}

void Simulation::addModule( RofiId rofiId, int joints, int connectors )
{
    // TODO use some kind of descriptor
    // TODO add RoFI to configuration
    _database.addRofi( rofiId, joints, connectors );
}

std::vector< rofi::messages::RofiResp > Simulation::moveRofisOneIteration()
{
    // TODO
    return {};
}

Simulation::OptionalRofiResp Simulation::processRofiCommand( const rofi::messages::RofiCmd & cmd )
{
    using RofiCmd = rofi::messages::RofiCmd;

    switch ( cmd.cmdtype() )
    {
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
            if ( auto description = _database.getDescription( cmd.rofiid() ) )
            {
                rofi::messages::RofiResp resp;
                resp.set_rofiid( cmd.rofiid() );
                resp.set_resptype( cmd.cmdtype() );
                *resp.mutable_rofidescription() = std::move( *description );
                return resp;
            }
            std::cerr << "Rofi " << cmd.rofiid()
                      << " doesn't exist (rofi command type: " << cmd.cmdtype() << ")\n";
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

Simulation::OptionalRofiResp Simulation::processJointCommand( RofiId rofiId,
                                                              const rofi::messages::JointCmd & cmd )
{
    using JointCmd = rofi::messages::JointCmd;

    switch ( cmd.cmdtype() )
    {
        case JointCmd::NO_CMD:
        {
            return {};
        }
        case JointCmd::GET_CAPABILITIES:
        {
            // TODO get capabilities
            auto resp = getJointResp( rofiId, cmd.joint(), cmd.cmdtype() );
            *resp.mutable_jointresp()->mutable_capabilities() = {}; // TODO
            return resp;
        }
        case JointCmd::GET_POSITION:
        {
            // TODO get position
            auto position = 0.f; // TODO
            return getJointResp( rofiId, cmd.joint(), cmd.cmdtype(), position );
        }
        case JointCmd::GET_VELOCITY:
        {
            if ( auto velocity = _database.getVelocity( rofiId, cmd.joint() ) )
            {
                return getJointResp( rofiId, cmd.joint(), cmd.cmdtype(), *velocity );
            }
            std::cerr << "Rofi " << rofiId << " doesn't exist or does not have joint "
                      << cmd.joint() << " (joint command type: " << cmd.cmdtype() << ")\n";
            return {};
        }
        case JointCmd::SET_POS_WITH_SPEED:
        {
            // TODO clamp pos and speed
            auto pos = cmd.setposwithspeed().position();
            auto speed = cmd.setposwithspeed().speed();
            if ( pos < 0 )
            {
                std::cerr << "Got set position command with negative speed. Setting to zero\n";
                pos = 0;
            }
            if ( !_database.setPositionWithSpeed( rofiId, cmd.joint(), pos, speed ) )
            {
                std::cerr << "Rofi " << rofiId << " doesn't exist or does not have joint "
                          << cmd.joint() << " (joint command type: " << cmd.cmdtype() << ")\n";
            }
            return {};
        }
        case JointCmd::SET_VELOCITY:
        {
            // TODO clamp velocity
            auto velocity = cmd.setvelocity().velocity();
            if ( !_database.setVelocity( rofiId, cmd.joint(), velocity ) )
            {
                std::cerr << "Rofi " << rofiId << " doesn't exist or does not have joint "
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

Simulation::OptionalRofiResp Simulation::processConnectorCommand(
        RofiId rofiId,
        const rofi::messages::ConnectorCmd & cmd )
{
    using ConnectorCmd = rofi::messages::ConnectorCmd;

    switch ( cmd.cmdtype() )
    {
        case ConnectorCmd::NO_CMD:
        {
            return {};
        }
        case ConnectorCmd::GET_STATE:
        {
            if ( auto state = _database.getConnectorState( rofiId, cmd.connector() ) )
            {
                auto resp = getConnectorResp( rofiId, cmd.connector(), cmd.cmdtype() );
                *resp.mutable_connectorresp()->mutable_state() = std::move( *state );
                return resp;
            }
            std::cerr << "Rofi " << rofiId << " doesn't exist or does not have connector "
                      << cmd.connector() << " (connector command type: " << cmd.cmdtype() << ")\n";
            return {};
        }
        case ConnectorCmd::CONNECT:
        {
            // TODO check somewhere for new connections
            if ( !_database.extendConnector( rofiId, cmd.connector() ) )
            {
                std::cerr << "Rofi " << rofiId << " doesn't exist or does not have connector "
                          << cmd.connector() << " (connector command type: " << cmd.cmdtype()
                          << ")\n";
            }
            return {};
        }
        case ConnectorCmd::DISCONNECT:
        {
            // TODO send disconnect event
            if ( !_database.retractConnector( rofiId, cmd.connector() ) )
            {
                std::cerr << "Rofi " << rofiId << " doesn't exist or does not have connector "
                          << cmd.connector() << " (connector command type: " << cmd.cmdtype()
                          << ")\n";
            }
            return {};
        }
        case ConnectorCmd::PACKET:
        {
            if ( auto connectedTo = _database.getConnectedTo( rofiId, cmd.connector() ) )
            {
                auto resp = getConnectorResp( connectedTo->rofiId,
                                              connectedTo->connector,
                                              cmd.cmdtype() );
                *resp.mutable_connectorresp()->mutable_packet() = cmd.packet();
                return resp;
            }
            // TODO packet could not be sent
            return {};
        }
        case ConnectorCmd::CONNECT_POWER:
        {
            if ( !_database.setConnectorPower( rofiId, cmd.connector(), cmd.line(), true ) )
            {
                std::cerr << "Rofi " << rofiId << " doesn't exist or does not have connector "
                          << cmd.connector() << " (connector command type: " << cmd.cmdtype()
                          << ")\n";
            }
            return {};
        }
        case ConnectorCmd::DISCONNECT_POWER:
        {
            if ( !_database.setConnectorPower( rofiId, cmd.connector(), cmd.line(), false ) )
            {
                std::cerr << "Rofi " << rofiId << " doesn't exist or does not have connector "
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

rofi::messages::RofiResp Simulation::getJointResp( RofiId rofiId,
                                                   int joint,
                                                   rofi::messages::JointCmd::Type type,
                                                   float value )
{
    rofi::messages::RofiResp rofiResp;
    rofiResp.set_rofiid( rofiId );
    rofiResp.set_resptype( rofi::messages::RofiCmd::JOINT_CMD );
    auto & jointResp = *rofiResp.mutable_jointresp();
    jointResp.set_joint( joint );
    jointResp.set_resptype( type );
    jointResp.set_value( value );
    return rofiResp;
}

rofi::messages::RofiResp Simulation::getConnectorResp( RofiId rofiId,
                                                       int connector,
                                                       rofi::messages::ConnectorCmd::Type type )
{
    rofi::messages::RofiResp rofiResp;
    rofiResp.set_rofiid( rofiId );
    rofiResp.set_resptype( rofi::messages::RofiCmd::CONNECTOR_CMD );
    auto & connectorResp = *rofiResp.mutable_connectorresp();
    connectorResp.set_connector( connector );
    connectorResp.set_resptype( type );
    return rofiResp;
}
