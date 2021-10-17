#include "modules_communication.hpp"


using namespace rofi::simplesim;

using RofiId = ModulesCommunication::RofiId;


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
    _lockedModules.emplace( rofiId, getNewLockedModule( rofiId ) );
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

    _lockedModules.emplace( rofiId, getNewLockedModule( rofiId ) );
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
