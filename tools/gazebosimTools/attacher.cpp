#include <iostream>

#include <gazebo/gazebo.hh>
#include <gazebo/gazebo_client.hh>

#include <connectorAttachInfo.pb.h>


std::string ltrim( std::string str, const std::string & chars = "\t\n\v\f\r " )
{
    str.erase( 0, str.find_first_not_of( chars ) );
    return str;
}

std::string rtrim( std::string str, const std::string & chars = "\t\n\v\f\r " )
{
    str.erase( str.find_last_not_of( chars ) + 1 );
    return str;
}

std::string trim( std::string str, const std::string & chars = "\t\n\v\f\r " )
{
    return ltrim( rtrim( str, chars ), chars );
}

int main()
{
    using Info = rofi::messages::ConnectorAttachInfo;

    gazebo::client::setup();

    auto node = boost::make_shared< gazebo::transport::Node >();

    std::string tmpString;

    std::cout << "Write the world name:\n";
    std::getline( std::cin, tmpString );
    tmpString = trim( std::move( tmpString ) );

    node->Init( tmpString );
    auto pub = node->Advertise< Info >( "~/attach_event" );
    assert( pub );
    pub->WaitForConnection();

    std::cout << "Advertizing on topic: '" << pub->GetTopic() << "'\n";

    while ( std::cin )
    {
        Info info;
        std::optional< bool > attach;

        std::cout << "Write if you want to attach or detach:" << std::endl;
        std::getline( std::cin, tmpString );
        tmpString = trim( std::move( tmpString ) );
        if ( tmpString == "attach" || tmpString == "a" )
        {
            attach = true;
        }
        else if ( tmpString == "detach" || tmpString == "d" )
        {
            attach = false;
        }
        else
        {
            std::cerr << "Could not recognize '" << tmpString << "'\n"
                      << "Write 'attach'/'detach' (or 'a'/'d' respectively)\n";
            continue;
        }

        std::cout << "Write the scoped name of RoFICoM model 1 (delimetered by '::'):" << std::endl;
        std::getline( std::cin, tmpString );
        tmpString = trim( std::move( tmpString ) );
        info.set_modelname1( tmpString );

        std::cout << "Write the scoped name of RoFICoM model 2 (delimetered by '::'):" << std::endl;
        std::getline( std::cin, tmpString );
        tmpString = trim( std::move( tmpString ) );
        info.set_modelname2( tmpString );

        info.set_attach( attach.value() );

        if ( attach.value() )
        {
            std::cout << "(Optional) Set orientation (north, east, south, west): " << std::endl;
            std::getline( std::cin, tmpString );
            tmpString = trim( std::move( tmpString ) );
            if ( tmpString == "north" || tmpString == "n" )
            {
                std::cout << "Orientation: north\n";
                info.set_orientation( rofi::messages::ConnectorState::NORTH );
            }
            else if ( tmpString == "east" || tmpString == "e" )
            {
                std::cout << "Orientation: east\n";
                info.set_orientation( rofi::messages::ConnectorState::EAST );
            }
            else if ( tmpString == "south" || tmpString == "s" )
            {
                std::cout << "Orientation: south\n";
                info.set_orientation( rofi::messages::ConnectorState::SOUTH );
            }
            else if ( tmpString == "west" || tmpString == "w" )
            {
                std::cout << "Orientation: west\n";
                info.set_orientation( rofi::messages::ConnectorState::WEST );
            }
        }

        pub->Publish( info, true );
        info.PrintDebugString();
        std::cout << "Sent message to '" << pub->GetTopic() << "'\n" << std::endl;
    }
}
