#include <gazebo/gazebo_config.h>
#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/gazebo_client.hh>

#include <jointCmd.pb.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>


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

rofi::messages::JointCmd processJointCmd( const std::vector< std::string_view > & tokens )
{
    using rofi::messages::JointCmd;

    if ( tokens.size() < 3 )
        throw std::runtime_error("Wrong number of arguments");

    rofi::messages::JointCmd jointCmd;
    jointCmd.set_joint( readInt( tokens[1] ) );

    auto jointCmdType = getJointCmdType( tokens[2] );
    jointCmd.set_cmdtype( jointCmdType );

    switch ( jointCmdType )
    {
        case JointCmd::SET_POS_WITH_SPEED:
        {
            if ( tokens.size() != 5 )
                throw std::runtime_error("Wrong number of arguments");

            jointCmd.mutable_setposwithspeed()->set_position( readFloat( tokens[3] ) );
            jointCmd.mutable_setposwithspeed()->set_speed( readFloat( tokens[4] ) );
            break;
        }
        case JointCmd::SET_SPEED:
        {
            if ( tokens.size() != 4 )
                throw std::runtime_error("Wrong number of arguments");

            jointCmd.mutable_setspeed()->set_speed( readFloat( tokens[3] ) );
            break;
        }
        case JointCmd::SET_TORQUE:
        {
            if ( tokens.size() != 4 )
                throw std::runtime_error("Wrong number of arguments");

            jointCmd.mutable_settorque()->set_torque( readFloat( tokens[3] ) );
            break;
        }
        default:
            if ( tokens.size() != 3 )
                throw std::runtime_error("Wrong number of arguments");
            break;
    }

    return jointCmd;
}


int main( int argc, char **argv )
{
    try
    {
#if CONNECT
        if ( !gazebo::client::setup( argc, argv ) )
        {
            std::cerr << "Error on gazebo client setup\n";
        }

        gazebo::transport::NodePtr node( new gazebo::transport::Node() );
        node->Init();

        auto pub = node->Advertise< rofi::messages::JointCmd >( "~/universalModule/control" );
        if ( !pub )
        {
            std::cerr << "No found publisher\n";
        }

        std::cerr << "Waiting for connection...\n";
        pub->WaitForConnection();
        std::cerr << "Connected\n";
#endif

        for ( std::string line; std::getline( std::cin, line ); )
        {
            auto tokens = split( line );
            if ( tokens.empty() )
                continue;

            if ( tokens.front() == "quit" )
                break;

            try
            {
                if ( tokens.front() == "joint" )
                {
                    auto msg = processJointCmd( tokens );
#if CONNECT
                    pub->Publish( msg );
#else
                    std::cout << "Sending...\n" << msg.DebugString();
#endif
                    continue;
                }
            }
            catch ( const std::exception & e )
            {
                std::cerr << e.what() << '\n';
                continue;
            }

            std::cerr << "Not yet implemented\n\n";
            printHelp();
        }

#if CONNECT
        gazebo::client::shutdown();
#endif
    }
    catch ( const gazebo::common::Exception & e )
    {
        std::cerr << e.GetErrorStr() << "\n";
    }
}
