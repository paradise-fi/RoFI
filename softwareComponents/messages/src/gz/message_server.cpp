#include "message_server.hpp"

#include <gazebo/gazebo.hh>

#include "gazebo/util/LogRecord.hh"


namespace rofi::msgs
{
namespace details
{
    class MessageServerImpl : public gazebo::Master
    {};

} // namespace details

MessageServer::MessageServer( std::string_view logName )
{
    gazebo::common::load();
    if ( !gazebo::transport::init() ) {
        throw std::runtime_error( "Unable to initialize transport." );
    }
    // Run transport loop. Starts a thread.
    gazebo::transport::run();


    std::string host; // Host is ignored for master
    unsigned port = {};
    gazebo::transport::get_master_uri( host, port );

    _impl->Init( port );

    auto logNameString = std::string( logName );
    gzLogInit( logNameString + "-", logNameString + ".log" );
    gazebo::util::LogRecord::Instance()->Init( logNameString );
}

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
