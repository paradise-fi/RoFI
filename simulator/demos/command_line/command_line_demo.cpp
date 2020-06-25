#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <gazebo/gazebo_client.hh>

#include "rofi_hal.hpp"


enum class JointCmd
{
    NO_CMD,
    GET_CAPABILITIES,
    GET_VELOCITY,
    SET_VELOCITY,
    GET_POSITION,
    SET_POS_WITH_SPEED,
    GET_TORQUE,
    SET_TORQUE,
};

enum class ConnectorCmd
{
    NO_CMD,
    CONNECT,
    DISCONNECT,
    GET_STATE,
    PACKET,
    CONNECT_POWER,
    DISCONNECT_POWER,
};

inline std::string_view ltrim( std::string_view line, std::string_view ws = " \t" )
{
    auto first = line.find_first_not_of( ws );
    return first == std::string_view::npos ? "" : line.substr( first );
}

inline std::string_view rtrim( std::string_view line, std::string_view ws = " \t" )
{
    auto last = line.find_last_not_of( ws );
    return last == std::string_view::npos ? "" : line.substr( 0, last + 1 );
}

inline std::string_view trim( std::string_view line, std::string_view ws = " \t" )
{
    return rtrim( ltrim( line, ws ), ws );
}

inline std::vector< std::string_view > split( std::string_view line )
{
    const std::string_view ws = " \t";

    line = trim( line, ws );
    std::vector< std::string_view > tokens;
    while ( !line.empty() && line.front() != '#' )
    {
        auto last = line.find_first_of( ws );
        tokens.push_back( line.substr( 0, last ) );

        auto nextFirst = line.find_first_not_of( ws, last );
        if ( nextFirst == std::string_view::npos )
        {
            break;
        }

        line.remove_prefix( nextFirst );
    }

    return tokens;
}

float readFloat( std::string_view str )
{
    return std::atof( std::string( str ).data() );
}

int readInt( std::string_view str )
{
    return std::atoi( std::string( str ).data() );
}


JointCmd getJointCmdType( std::string_view token )
{
    const std::map< std::string_view, JointCmd > map = {
        { "getmaxposition", JointCmd::GET_CAPABILITIES },
        { "getposition", JointCmd::GET_POSITION },
        { "getvelocity", JointCmd::GET_VELOCITY },
        { "gettorque", JointCmd::GET_TORQUE },
        { "setposwithspeed", JointCmd::SET_POS_WITH_SPEED },
        { "setvelocity", JointCmd::SET_VELOCITY },
        { "settorque", JointCmd::SET_TORQUE }
    };

    auto it = map.find( token );
    if ( it == map.end() )
        throw std::runtime_error( "Unknown joint cmd type" );
    return it->second;
}

inline ConnectorCmd getConnectorCmdType( std::string_view token )
{
    const std::map< std::string_view, ConnectorCmd > map = {
        { "getstate", ConnectorCmd::GET_STATE },
        { "connect", ConnectorCmd::CONNECT },
        { "disconnect", ConnectorCmd::DISCONNECT },
        { "sendpacket", ConnectorCmd::PACKET },
        { "connectpower", ConnectorCmd::CONNECT_POWER },
        { "disconnectpower", ConnectorCmd::DISCONNECT_POWER },
    };

    auto it = map.find( token );
    if ( it == map.end() )
        throw std::runtime_error( "Unknown connector cmd type" );
    return it->second;
}

void printHelp()
{
    std::cerr << "Help not yet implemented\n  write 'quit' for quit\nExample of usage:\n"
              << "\tjoint 2 setvelocity 8.5\n"
              << "\tjoint 1 settorque -3.4\n"
              << "\tjoint 2 getmaxposition\n"
              << "\tconnector 3 connect\n";
}

void processJointCmd( rofi::hal::RoFI & rofi, const std::vector< std::string_view > & tokens )
{
    if ( tokens.size() < 3 )
        throw std::runtime_error( "Wrong number of arguments" );

    auto joint = rofi.getJoint( readInt( tokens[ 1 ] ) );

    switch ( getJointCmdType( tokens[ 2 ] ) )
    {
        case JointCmd::NO_CMD:
        {
            break;
        }
        case JointCmd::GET_CAPABILITIES:
        {
            auto maxPosition = joint.maxPosition();
            auto minPosition = joint.minPosition();
            auto maxSpeed = joint.maxSpeed();
            auto minSpeed = joint.minSpeed();
            auto maxTorque = joint.maxTorque();
            std::cout << "Max position: " << maxPosition << "\n";
            std::cout << "Min position: " << minPosition << "\n";
            std::cout << "Max speed: " << maxSpeed << "\n";
            std::cout << "Min speed: " << minSpeed << "\n";
            std::cout << "Max torque: " << maxTorque << "\n";
            break;
        }
        case JointCmd::GET_VELOCITY:
        {
            auto result = joint.getVelocity();
            std::cout << "Current velocity: " << result << "\n";
            break;
        }
        case JointCmd::SET_VELOCITY:
        {
            if ( tokens.size() != 4 )
                throw std::runtime_error( "Wrong number of arguments" );
            joint.setVelocity( readFloat( tokens[ 3 ] ) );
            break;
        }
        case JointCmd::GET_POSITION:
        {
            auto result = joint.getPosition();
            std::cout << "Current position: " << result << "\n";
            break;
        }
        case JointCmd::SET_POS_WITH_SPEED:
        {
            if ( tokens.size() != 5 )
                throw std::runtime_error( "Wrong number of arguments" );
            joint.setPosition( readFloat( tokens[ 3 ] ),
                               readFloat( tokens[ 4 ] ),
                               nullptr ); // TODO callback
            break;
        }
        case JointCmd::GET_TORQUE:
        {
            auto result = joint.getTorque();
            std::cout << "Current torque: " << result << "\n";
            break;
        }
        case JointCmd::SET_TORQUE:
        {
            if ( tokens.size() != 4 )
                throw std::runtime_error( "Wrong number of arguments" );
            joint.setTorque( readFloat( tokens[ 3 ] ) );
            break;
        }
        default:
        {
            printHelp();
            break;
        }
    }
}

void processConnectorCmd( rofi::hal::RoFI & rofi, const std::vector< std::string_view > & tokens )
{
    if ( tokens.size() < 3 )
        throw std::runtime_error( "Wrong number of arguments" );

    auto connector = rofi.getConnector( readInt( tokens[ 1 ] ) );

    switch ( getConnectorCmdType( tokens[ 2 ] ) )
    {
        case ConnectorCmd::NO_CMD:
        {
            break;
        }
        case ConnectorCmd::CONNECT:
        {
            connector.connect();
            std::cout << "Connecting\n";
            break;
        }
        case ConnectorCmd::DISCONNECT:
        {
            connector.disconnect();
            std::cout << "Disconnecting\n";
            break;
        }
        case ConnectorCmd::GET_STATE:
        case ConnectorCmd::PACKET:
        case ConnectorCmd::CONNECT_POWER:
        case ConnectorCmd::DISCONNECT_POWER:
        {
            std::cout << "Connector command not implemented\n";
            printHelp();
            break;
        }
        default:
        {
            printHelp();
            break;
        }
    }
}


int main( int argc, char ** argv )
{
    try
    {
        int rofiId = 0;

        if ( argc > 1 )
        {
            rofiId = readInt( argv[ 1 ] );
        }

        std::cerr << "Acquiring RoFI " << rofiId << "\n";
        auto rofi = rofi::hal::RoFI::getRemoteRoFI( rofiId );
        std::cerr << "Acquired RoFI " << rofiId << "\n";

        for ( std::string line; std::getline( std::cin, line ); )
        {
            try
            {
                auto tokens = split( line );
                if ( tokens.empty() )
                    continue;

                if ( tokens.front() == "quit" )
                    break;

                if ( tokens.front() == "joint" )
                {
                    processJointCmd( rofi, tokens );
                    continue;
                }

                if ( tokens.front() == "connector" )
                {
                    processConnectorCmd( rofi, tokens );
                    continue;
                }

                std::cerr << "Not yet implemented\n\n";
                printHelp();
            }
            catch ( const std::runtime_error & e )
            {
                std::cerr << "ERROR: " << e.what() << "\n";
            }
        }
    }
    catch ( const gazebo::common::Exception & e )
    {
        std::cerr << e.GetErrorStr() << "\n";
    }
}
