#include "locked_module_communication.hpp"


using namespace rofi::simplesim;

using RofiId = LockedModuleCommunication::RofiId;


LockedModuleCommunication::LockedModuleCommunication( CommandHandler & commandHandler,
                                                      gazebo::transport::Node & node,
                                                      std::string moduleTopicName,
                                                      RofiId rofiId )
        : _commandHandler( commandHandler )
        , _rofiId( rofiId )
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

    if ( msg->rofiid() != _rofiId ) {
        std::cerr << "Got a command from Module " << _rofiId << " for Module " << msg->rofiid()
                  << ". Ignoring...\n";
        return;
    }

    _commandHandler.onRofiCmd( msg );
}
