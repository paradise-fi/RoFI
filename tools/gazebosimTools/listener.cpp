#include <cassert>
#include <chrono>
#include <string_view>
#include <thread>

#include <rofi/gz_transport.hpp>
#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


template < typename Message >
void printMessage( const boost::shared_ptr< const Message > & msg )
{
    std::cout << "Got message:\n" << msg->DebugString() << std::endl;
}

int main( int argc, char ** argv )
{
    using namespace std::string_view_literals;
    if ( argc != 2 || argv[ 1 ] == "-h"sv || argv[ 1 ] == "--help"sv )
    {
        std::cerr << "Usage: " << argv[ 0 ] << " <topic>" << std::endl;
        return 1;
    }

    auto node = std::make_shared< rofi::gz::Node >();
    node->Init();
    auto sub = node->Subscribe< rofi::messages::RofiCmd >(
            argv[ 1 ], printMessage< rofi::messages::RofiCmd > );
    assert( sub );
    std::cout << "Listening on " << sub->GetTopic() << std::endl;

    while ( true )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
}
