#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <gazebo/gazebo.hh>
#include <gazebo/gazebo_client.hh>

#include <connectorCmd.pb.h>


namespace msgs = rofi::messages;

std::string_view ltrim( std::string_view line, std::string_view ws = " \t" )
{
    auto first = line.find_first_not_of( ws );
    return first == std::string_view::npos ? "" : line.substr( first );
}

std::string_view rtrim( std::string_view line, std::string_view ws = " \t" )
{
    auto last = line.find_last_not_of( ws );
    return last == std::string_view::npos ? "" : line.substr( 0, last + 1 );
}

std::string_view trim( std::string_view line, std::string_view ws = " \t" )
{
    return rtrim( ltrim( line, ws ), ws );
}

int main( int argc, char ** argv )
{
    gazebo::client::setup( argc, argv );


    std::cout << "Write the world name:\n";

    std::string worldName;
    std::getline( std::cin, worldName );
    worldName = trim( std::move( worldName ) );

    if ( worldName.empty() )
    {
        std::cerr << "World name empty. Using name 'default'.\n";
        worldName = "default";
    }

    auto node = boost::make_shared< gazebo::transport::Node >();
    node->Init( worldName );


    while ( std::cin )
    {
        std::string roficomName;
        std::cout << "Write the scoped name of RoFICoM model (delimetered by '/'):" << std::endl;
        std::getline( std::cin, roficomName );
        roficomName = trim( std::move( roficomName ) );

        if ( roficomName.empty() )
        {
            std::cerr << "RoFICoM name cannot be empty\n";
            continue;
        }


        std::string cmd;
        std::cout << "Write connect (c) or disconnect (d):" << std::endl;
        std::getline( std::cin, cmd );
        cmd = trim( std::move( cmd ) );
        if ( cmd.empty() )
        {
            continue;
        }

        msgs::ConnectorCmd msg;
        if ( cmd == "c" || cmd == "connect" )
        {
            msg.set_cmdtype( msgs::ConnectorCmd::CONNECT );
        }
        else if ( cmd == "d" || cmd == "disconnect" )
        {
            msg.set_cmdtype( msgs::ConnectorCmd::DISCONNECT );
        }
        else
        {
            std::cerr << "Command not recognized\n";
            continue;
        }


        auto pub = node->Advertise< msgs::ConnectorCmd >( "~/" + roficomName + "/control" );
        if ( !pub->WaitForConnection( gazebo::common::Time::Second ) )
        {
            std::cerr << "Could not connect to topic '" << pub->GetTopic() << "'\n\n";
            continue;
        }
        std::cout << "Advertizing on topic '" << pub->GetTopic() << "'\n\n";

        pub->Publish( msg, true );
    }

    gazebo::client::shutdown();
}
