#include "modules_info.hpp"


using namespace rofi::simplesim;

using RofiId = ModulesInfo::RofiId;


LockedModuleInfo::LockedModuleInfo( ModulesInfo & modulesInfo, RofiId rofiId )
        : _modulesInfo( modulesInfo )
        , _rofiId( rofiId )
        , _topicName( _modulesInfo.getNewTopicName() )
        , _pub( _modulesInfo._node->Advertise< rofi::messages::RofiResp >( "~/" + _topicName
                                                                           + "/response" ) )
        , _sub( _modulesInfo._node->Subscribe( "~/" + _topicName + "/control",
                                               &LockedModuleInfo::onRofiCmd,
                                               this ) )
{
    assert( !_topicName.empty() );
    assert( _pub );
    assert( _sub );
}

void LockedModuleInfo::onRofiCmd( const LockedModuleInfo::RofiCmdPtr & msg )
{
    assert( msg );

    if ( msg->rofiid() != _rofiId ) {
        std::cerr << "Got a command from Module " << _rofiId << " for Module " << msg->rofiid()
                  << ". Ignoring...\n";
        return;
    }
    _modulesInfo.onRofiCmd( msg );
}


bool ModulesInfo::addNewRofi( RofiId rofiId )
{
    std::lock_guard< std::shared_mutex > lock( _modulesMutex );

    if ( _lockedModules.find( rofiId ) != _lockedModules.end() ) {
        return false;
    }
    return _freeModules.insert( rofiId ).second;
}

std::optional< RofiId > ModulesInfo::lockFreeRofi()
{
    std::lock_guard< std::shared_mutex > lock( _modulesMutex );

    auto it = _freeModules.begin();
    if ( it == _freeModules.end() ) {
        return {};
    }

    auto rofiId = *it;
    _lockedModules.emplace( rofiId, LockedModuleInfo( *this, rofiId ) );
    _freeModules.erase( it );
    return rofiId;
}

bool ModulesInfo::tryLockRofi( RofiId rofiId )
{
    std::lock_guard< std::shared_mutex > lock( _modulesMutex );

    auto it = _freeModules.find( rofiId );
    if ( it == _freeModules.end() ) {
        return false;
    }

    _lockedModules.emplace( rofiId, LockedModuleInfo( *this, rofiId ) );
    _freeModules.erase( it );
    return true;
}

bool ModulesInfo::unlockRofi( RofiId rofiId )
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

std::optional< std::string > ModulesInfo::getTopic( RofiId rofiId ) const
{
    std::shared_lock< std::shared_mutex > lock( _modulesMutex );

    auto it = _lockedModules.find( rofiId );
    if ( it != _lockedModules.end() ) {
        return it->second.topic( *_node );
    }
    return {};
}

bool ModulesInfo::isLocked( RofiId rofiId ) const
{
    std::shared_lock< std::shared_mutex > lock( _modulesMutex );

    return _lockedModules.find( rofiId ) != _lockedModules.end();
}
