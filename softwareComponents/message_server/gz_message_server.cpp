#include <gazebo/gazebo.hh>

#include "message_server.hpp"


namespace rofi::msgs
{
MessageServer MessageServer::createAndLoopInThread( std::string_view /* logName */ )
{
    std::string host; // Host is ignored for master
    unsigned port = {};
    gazebo::transport::get_master_uri( host, port );

    MessageServer result;
    result._impl = std::make_unique< gazebo::Master >();
    result._impl->Init( port );
    result._impl->RunThread();


    gazebo::common::load();

    // TODO: use logName
    // Initialize the informational logger. This will log warnings, and errors.
    gzLogInit( "simplesim-", "default.log" );

    if ( !gazebo::transport::init() ) {
        throw std::runtime_error( "Unable to initialize transport." );
    }

    // Run transport loop. Starts a thread.
    gazebo::transport::run();

    return result;
}

} // namespace rofi::msgs
