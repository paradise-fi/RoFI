#include "locked_module_communication.hpp"

#include "modules_communication.hpp"

#include <rofiResp.pb.h>


using namespace rofi::simplesim;

using RofiId = ModulesCommunication::RofiId;


LockedModuleCommunication::LockedModuleCommunication( ModulesCommunication & modulesCommunication,
                                                      RofiId rofiId )
        : _modulesCommunication( modulesCommunication )
        , _rofiId( rofiId )
        , _topicName( _modulesCommunication.getNewTopicName() )
        , _pub( _modulesCommunication._node->Advertise< rofi::messages::RofiResp >(
                  "~/" + _topicName + "/response" ) )
        , _sub( _modulesCommunication._node->Subscribe( "~/" + _topicName + "/control",
                                                        &LockedModuleCommunication::onRofiCmd,
                                                        this ) )
{
    assert( !_topicName.empty() );
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
    _modulesCommunication.onRofiCmd( msg );
}
