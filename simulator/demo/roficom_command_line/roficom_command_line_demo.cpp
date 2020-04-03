#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <gazebo/gazebo_client.hh>

#include "../../tools/rofi_hal.hpp"

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

inline ConnectorCmd getConnectorCmdType( std::string_view token )
{
    const std::map< std::string_view, ConnectorCmd > map = {
        { "s", ConnectorCmd::GET_STATE },
        { "getstate", ConnectorCmd::GET_STATE },
        { "c", ConnectorCmd::CONNECT },
        { "connect", ConnectorCmd::CONNECT },
        { "d", ConnectorCmd::DISCONNECT },
        { "disconnect", ConnectorCmd::DISCONNECT },
        { "p", ConnectorCmd::PACKET },
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
              << "\tc 2 connect\n"
              << "\tconnector 0 c \n"
              << "\tconnector 2 disconnect\n"
              << "\tc 2 d\n";
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
    gazebo::client::setup( argc, argv );

    try
    {
        auto & rofi = rofi::hal::RoFI::getLocalRoFI();

        for ( std::string line; std::getline( std::cin, line ); )
        {
            try
            {
                auto tokens = split( line );
                if ( tokens.empty() )
                    continue;

                if ( tokens.front() == "quit" || tokens.front() == "q" )
                    break;

                if ( tokens.front() == "connector" || tokens.front() == "c" )
                {
                    processConnectorCmd( rofi, tokens );
                    continue;
                }

                std::cerr << "Not implemented\n\n";
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
    gazebo::client::shutdown();
}
