#include "simplesim/locked_module_communication.hpp"


using namespace rofi::simplesim;

LockedModuleCommunication::LockedModuleCommunication( CommandHandler & commandHandler,
                                                      gazebo::transport::Node & node,
                                                      std::string moduleTopicName,
                                                      ModuleId moduleId )
        : _commandHandler( commandHandler )
        , _moduleId( moduleId )
        , _topic( "/gazebo/" + node.GetTopicNamespace() + "/" + moduleTopicName )
        , _pub( node.Advertise< rofi::messages::RofiResp >( "~/" + moduleTopicName + "/response" ) )
        , _sub( node.Subscribe( "~/" + moduleTopicName + "/control",
                                &LockedModuleCommunication::onRofiCmd,
                                this ) )
{
    assert( !moduleTopicName.empty() );
    assert( _pub );
    assert( _sub );
}

void LockedModuleCommunication::onRofiCmd( const LockedModuleCommunication::RofiCmdPtr & msg )
{
    assert( msg );

    if ( msg->rofiid() != _moduleId ) {
        std::cerr << "Got a command from Module " << _moduleId << " for Module " << msg->rofiid()
                  << ". Ignoring...\n";
        return;
    }

    if ( auto resp = _commandHandler.onRofiCmd( msg ) ) {
        assert( resp->rofiid() == _moduleId && "Immediate responses have to have same rofi id" );
        _pub->Publish( *resp );
    }
}
