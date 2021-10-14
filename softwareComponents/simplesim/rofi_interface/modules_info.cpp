#include "modules_info.hpp"


using namespace rofi::simplesim;

using RofiId = ModulesInfo::RofiId;
using SessionId = ModulesInfo::SessionId;


ModulesInfo::LockedModuleInfo::LockedModuleInfo( ModulesInfo & modulesInfo,
                                                 RofiId rofiId,
                                                 SessionId sessionId )
        : _modulesInfo( modulesInfo )
        , _rofiId( rofiId )
        , _sessionId( sessionId )
        , _topicName( _modulesInfo.getNewTopicName() )
        , _pub( _modulesInfo._node->Advertise< rofi::messages::RofiResp >( "~/" + _topicName
                                                                           + "/response" ) )
        , _sub( _modulesInfo._node->Subscribe( "~/" + _topicName + "/control",
                                               &ModulesInfo::onRofiCmd,
                                               &_modulesInfo ) )
{
    assert( _pub );
    assert( _sub );
}

bool ModulesInfo::addNewRofi( RofiId rofiId )
{
    std::lock_guard< std::shared_mutex > lock( _modulesMutex );

    if ( _lockedModules.find( rofiId ) != _lockedModules.end() ) {
        return false;
    }
    return _freeModules.insert( rofiId ).second;
}

std::optional< RofiId > ModulesInfo::lockFreeRofi( SessionId sessionId )
{
    std::lock_guard< std::shared_mutex > lock( _modulesMutex );

    auto it = _freeModules.begin();
    if ( it == _freeModules.end() ) {
        return {};
    }

    auto rofiId = *it;
    _lockedModules.emplace( rofiId, LockedModuleInfo( *this, rofiId, std::move( sessionId ) ) );
    _freeModules.erase( it );
    return rofiId;
}

bool ModulesInfo::tryLockRofi( RofiId rofiId, SessionId sessionId )
{
    std::lock_guard< std::shared_mutex > lock( _modulesMutex );

    auto it = _freeModules.find( rofiId );
    if ( it == _freeModules.end() ) {
        return false;
    }

    _lockedModules.emplace( rofiId, LockedModuleInfo( *this, rofiId, std::move( sessionId ) ) );
    _freeModules.erase( it );
    return true;
}

bool ModulesInfo::unlockRofi( RofiId rofiId, SessionId sessionId )
{
    std::lock_guard< std::shared_mutex > lock( _modulesMutex );

    auto it = _lockedModules.find( rofiId );
    if ( it == _lockedModules.end() ) {
        return true;
    }

    if ( it->second.sessionId() != sessionId ) {
        return false;
    }

    it->second.sub().Unsubscribe();
    _lockedModules.erase( it );
    _freeModules.insert( rofiId );
    return true;
}

std::optional< SessionId > ModulesInfo::getSessionId( RofiId rofiId ) const
{
    std::shared_lock< std::shared_mutex > lock( _modulesMutex );

    auto it = _lockedModules.find( rofiId );
    if ( it != _lockedModules.end() ) {
        return it->second.sessionId();
    }
    return {};
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
