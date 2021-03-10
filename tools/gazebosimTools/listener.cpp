#include <cassert>

#include <gazebo/gazebo_client.hh>
#include <gazebo/transport/transport.hh>

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>

class GazeboClientHolder
{
public:
    GazeboClientHolder()
    {
        std::cerr << "Starting gazebo client" << std::endl;
        gazebo::client::setup();
    }

    GazeboClientHolder( const GazeboClientHolder & ) = delete;
    GazeboClientHolder & operator=( const GazeboClientHolder & ) = delete;

    ~GazeboClientHolder()
    {
        gazebo::client::shutdown();
        std::cerr << "Ending gazebo client" << std::endl;
    }
};


template < typename Message >
void printMessage( const boost::shared_ptr< const Message > & msg )
{
    std::cout << "Got message:\n" << msg->DebugString() << std::endl;
}

int main( int argc, char ** argv )
{
    if ( argc != 2 )
    {
        std::cerr << "Usage: " << argv[ 0 ] << " <topic>" << std::endl;
        return 1;
    }

    auto clientHolder = GazeboClientHolder();

    auto node = boost::make_shared< gazebo::transport::Node >();
    node->Init();
    auto sub = node->Subscribe( argv[ 1 ], printMessage< rofi::messages::RofiCmd > );
    assert( sub );
    std::cout << "Listening on " << sub->GetTopic() << std::endl;

    while ( true )
    {
        gazebo::common::Time::MSleep( 10 );
    }
}
