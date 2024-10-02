#include "simplesim/locked_module_communication.hpp"

#include <fmt/format.h>

using namespace rofi::simplesim;

LockedModuleCommunication::LockedModuleCommunication( CommandHandler & commandHandler,
                                                      gazebo::transport::Node & node,
                                                      std::string moduleTopicName,
                                                      ModuleId moduleId,
                                                      bool verbose )
        : _commandHandler( commandHandler )
        , _moduleId( moduleId )
        , _topic( "/gazebo/" + node.GetTopicNamespace() + "/" + moduleTopicName )
        , _logger( verbose )
        , _pub( node.Advertise< rofi::messages::RofiResp >( "~/" + moduleTopicName + "/response" ) )
        , _sub( node.Subscribe( "~/" + moduleTopicName + "/control",
                                &LockedModuleCommunication::onRofiCmd,
                                this ) )
{
    assert( !moduleTopicName.empty() );
    assert( _pub );
    assert( _sub );
    _logger.logSubscribe( "~/" + moduleTopicName + "/control" );
}

void LockedModuleCommunication::onRofiCmd( const LockedModuleCommunication::RofiCmdPtr & msg )
{
    assert( msg );
    auto msgCopy = msg;
    assert( msgCopy );

    _logger.logReceived( _sub->GetTopic(), *msgCopy );

    if ( msgCopy->rofiid() != _moduleId ) {
        std::cerr << fmt::format( "Got a command in Module {} for Module {}. Ignoring...\n",
                                  _moduleId,
                                  msgCopy->rofiid() );
        return;
    }

    if ( auto resp = _commandHandler.onRofiCmd( std::move( msgCopy ) ) ) {
        assert( resp->rofiid() == _moduleId && "Immediate responses have to have same rofi id" );
        sendResponse( *resp );
    }
}
