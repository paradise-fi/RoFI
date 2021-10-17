#include "modules_communication.hpp"


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


bool ModulesCommunication::addNewRofi( RofiId rofiId )
{
    std::lock_guard< std::shared_mutex > lock( _modulesMutex );

    if ( _lockedModules.find( rofiId ) != _lockedModules.end() ) {
        return false;
    }
    return _freeModules.insert( rofiId ).second;
}

std::optional< RofiId > ModulesCommunication::lockFreeRofi()
{
    std::lock_guard< std::shared_mutex > lock( _modulesMutex );

    auto it = _freeModules.begin();
    if ( it == _freeModules.end() ) {
        return {};
    }

    auto rofiId = *it;
    _lockedModules.emplace( rofiId, LockedModuleCommunication( *this, rofiId ) );
    _freeModules.erase( it );
    return rofiId;
}

bool ModulesCommunication::tryLockRofi( RofiId rofiId )
{
    std::lock_guard< std::shared_mutex > lock( _modulesMutex );

    auto it = _freeModules.find( rofiId );
    if ( it == _freeModules.end() ) {
        return false;
    }

    _lockedModules.emplace( rofiId, LockedModuleCommunication( *this, rofiId ) );
    _freeModules.erase( it );
    return true;
}

bool ModulesCommunication::unlockRofi( RofiId rofiId )
{
    std::lock_guard< std::shared_mutex > lock( _modulesMutex );

    auto it = _lockedModules.find( rofiId );
    if ( it == _lockedModules.end() ) {
        return true;
    }

    it->second.sub().Unsubscribe();
    _lockedModules.erase( it );
    _freeModules.insert( rofiId );
    return true;
}

std::optional< std::string > ModulesCommunication::getTopic( RofiId rofiId ) const
{
    std::shared_lock< std::shared_mutex > lock( _modulesMutex );

    auto it = _lockedModules.find( rofiId );
    if ( it != _lockedModules.end() ) {
        return it->second.topic( *_node );
    }
    return {};
}

bool ModulesCommunication::isLocked( RofiId rofiId ) const
{
    std::shared_lock< std::shared_mutex > lock( _modulesMutex );

    return _lockedModules.find( rofiId ) != _lockedModules.end();
}
