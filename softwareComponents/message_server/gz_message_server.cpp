#include <gazebo/gazebo.hh>

#include "message_server.hpp"


namespace rofi::msgs
{
namespace details
{
    class MessageServerImpl : public gazebo::Master
    {};

} // namespace details

MessageServer::MessageServer( std::string_view logName )
{
    std::string host; // Host is ignored for master
    unsigned port = {};
    gazebo::transport::get_master_uri( host, port );

    _impl->Init( port );

    gzLogInit( std::string( logName ) + "-", std::string( logName ) + ".log" );

    gazebo::common::load();
    if ( !gazebo::transport::init() ) {
        throw std::runtime_error( "Unable to initialize transport." );
    }
    // Run transport loop. Starts a thread.
    gazebo::transport::run();
}

MessageServer::MessageServer( MessageServer && ) noexcept = default;
MessageServer::~MessageServer() = default;

void MessageServer::poll()
{
    assert( _impl );
    _impl->RunOnce();
}
void MessageServer::loop()
{
    assert( _impl );
    _impl->Run();
}
void MessageServer::loopInThread()
{
    assert( _impl );
    _impl->RunThread();
}

} // namespace rofi::msgs
