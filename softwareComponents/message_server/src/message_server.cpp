#include "message_server.hpp"

#include <gazebo/gazebo.hh>


namespace rofi::msgs
{
Server Server::createAndLoopInThread( std::string_view logName )
{
    std::string host; // Host is ignored for master
    unsigned port = 0;
    if ( !gazebo::transport::get_master_uri( host, port ) ) {
        std::cerr << "Unable to read master uri\n";
    }
    if ( port > UINT16_MAX ) {
        std::cerr << "Port is too high (" << port << ")\n";
        port = 0;
    }

    Server result;
    result._impl = std::make_unique< gazebo::Master >();
    assert( port <= UINT16_MAX );
    result._impl->Init( static_cast< uint16_t >( port ) );
    // Runs the master in a new thread.
    result._impl->RunThread();


    gazebo::common::load();

    // Initialize the informational logger. This will log warnings, and errors.
    gzLogInit( std::string( logName ) + "-", std::string( logName ) + ".log" );

    if ( !gazebo::transport::init() ) {
        throw std::runtime_error( "Unable to initialize transport." );
    }

    // Run transport loop. Starts a thread.
    gazebo::transport::run();

    return result;
}

} // namespace rofi::msgs
