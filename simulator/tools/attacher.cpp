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

int main( int argc, char ** argv )
{
    using Info = rofi::messages::ConnectorAttachInfo;

    gazebo::client::setup();

    auto node = boost::make_shared< gazebo::transport::Node >();

    std::string tmpString;

    std::cout << "Write the world name:\n";
    std::getline( std::cin, tmpString );
    tmpString = trim( std::move( tmpString ) );

    node->Init( tmpString );
    auto pubAttach = node->Advertise< Info >( "~/attach" );
    auto pubDetach = node->Advertise< Info >( "~/detach" );

    std::cout << "Advertizing on topic: '" << pubAttach->GetTopic() << "'\n";
    std::cout << "Advertizing on topic: '" << pubDetach->GetTopic() << "'\n";

    while ( true )
    {
        Info info;
        std::optional< bool > attach;

        while ( !attach.has_value() )
        {
            std::cout << "Write if you want to attach or detach: " << std::flush;
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
            }
        }

        std::cout << "RoFICoM model 1: " << std::flush;
        std::getline( std::cin, tmpString );
        tmpString = trim( std::move( tmpString ) );
        info.set_modelname1( tmpString );

        std::cout << "RoFICoM model 2: " << std::flush;
        std::getline( std::cin, tmpString );
        tmpString = trim( std::move( tmpString ) );
        info.set_modelname2( tmpString );

        while ( attach.value() )
        {
            std::cout << "Orientation (north, east, south, west): " << std::flush;
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
            else
            {
                continue;
            }
            break;
        }

        if ( attach.value() )
        {
            pubAttach->Publish( info, true );
            info.PrintDebugString();
            std::cout << "Sent message to '" << pubAttach->GetTopic() << "'\n";
        }
        else
        {
            pubDetach->Publish( info, true );
            info.PrintDebugString();
            std::cout << "Sent message to '" << pubDetach->GetTopic() << "'\n";
        }
        std::cout << std::endl;
    }
}
