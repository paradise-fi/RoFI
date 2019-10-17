#include <gazebo/gazebo_config.h>
#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/gazebo_client.hh>

#include <jointCmd.pb.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "rofi_hal.hpp"

#define CONNECT 1 // Want to connect to gazebo?


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


inline rofi::messages::JointCmd::Type getJointCmdType( std::string_view token )
{
    using rofi::messages::JointCmd;

    const std::map< std::string_view, JointCmd::Type > map = {
        { "getmaxposition", JointCmd::GET_MAX_POSITION },
        { "getminposition", JointCmd::GET_MIN_POSITION },
        { "getmaxspeed", JointCmd::GET_MAX_SPEED },
        { "getminspeed", JointCmd::GET_MIN_SPEED },
        { "getmaxtorque", JointCmd::GET_MAX_TORQUE },
        { "getposition", JointCmd::GET_POSITION },
        { "getspeed", JointCmd::GET_SPEED },
        { "gettorque", JointCmd::GET_TORQUE },
        { "setposwithspeed", JointCmd::SET_POS_WITH_SPEED },
        { "setspeed", JointCmd::SET_SPEED },
        { "settorque", JointCmd::SET_TORQUE }
    };

    auto it = map.find( token );
    if ( it == map.end() )
        throw std::runtime_error("Unknown joint cmd type");
    return it->second;
}

void printHelp()
{
    std::cerr << "Help not yet implemented\n  write 'quit' for quit\nExample of usage:\n"
              << "\tjoint 2 setspeed 8.5\n"
              << "\tjoint 1 settorque -3.4\n"
              << "\tjoint 2 getmaxposition\n";
}

void processJointCmd( rofi::hal::RoFI & rofi, const std::vector< std::string_view > & tokens )
{
    using rofi::messages::JointCmd;

    if ( tokens.size() < 3 )
        throw std::runtime_error("Wrong number of arguments");

    auto joint = rofi.getJoint( readInt( tokens[1] ) );

    switch ( getJointCmdType( tokens[2] ) )
    {
        case JointCmd::GET_MAX_POSITION:
            std::cout << "Max position: " << joint.maxPosition() << "\n";
            break;
        case JointCmd::GET_MIN_POSITION:
            std::cout << "Min position: " << joint.minPosition() << "\n";
            break;
        case JointCmd::GET_MAX_SPEED:
            std::cout << "Max speed: " << joint.maxSpeed() << "\n";
            break;
        case JointCmd::GET_MIN_SPEED:
            std::cout << "Min speed: " << joint.minSpeed() << "\n";
            break;
        case JointCmd::GET_MAX_TORQUE:
            std::cout << "Max torque: " << joint.maxTorque() << "\n";
            break;
        case JointCmd::GET_SPEED:
            std::cout << "Current speed: " << joint.getSpeed() << "\n";
            break;
        case JointCmd::SET_SPEED:
            if ( tokens.size() != 4 )
                throw std::runtime_error("Wrong number of arguments");
            joint.setSpeed( readFloat( tokens[3] ) );
            break;
        case JointCmd::GET_POSITION:
            std::cout << "Current position: " << joint.getPosition() << "\n";
            break;
        case JointCmd::SET_POS_WITH_SPEED:
            if ( tokens.size() != 5 )
                throw std::runtime_error("Wrong number of arguments");
            joint.setPosition( readFloat( tokens[3] ), readFloat( tokens[4] ), nullptr ); // TODO callback
            break;
        case JointCmd::GET_TORQUE:
            std::cout << "Current torque: " << joint.getTorque() << "\n";
            break;
        case JointCmd::SET_TORQUE:
            if ( tokens.size() != 4 )
                throw std::runtime_error("Wrong number of arguments");
            joint.setTorque( readFloat( tokens[3] ) );
            break;
        default:
            printHelp();
            break;
    }
}


int main( int argc, char **argv )
{
    try
    {
        gazebo::client::setup( argc, argv );

        auto rofi = rofi::hal::RoFI();

        for ( std::string line; std::getline( std::cin, line ); )
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

            std::cerr << "Not yet implemented\n\n";
            printHelp();
        }

        gazebo::client::shutdown();
    }
    catch ( const gazebo::common::Exception & e )
    {
        std::cerr << e.GetErrorStr() << "\n";
    }
}
