#include "simplesim/modules_communication.hpp"


using namespace rofi::simplesim;

using RofiId = ModulesCommunication::RofiId;


bool ModulesCommunication::addNewRofi( RofiId rofiId )
{
    return _modules.visit(
            [ rofiId ]( auto & modules ) { return modules.emplace( rofiId, nullptr ).second; } );
}

std::optional< RofiId > ModulesCommunication::lockFreeRofi()
{
    return _modules.visit( [ this ]( auto & modules ) -> std::optional< RofiId > {
        for ( auto & [ rofiId, moduleComm ] : modules ) {
            if ( !moduleComm ) {
                moduleComm = this->getNewLockedModule( rofiId );
                return rofiId;
            }
        }
        return std::nullopt;
    } );
}

bool ModulesCommunication::tryLockRofi( RofiId rofiId )
{
    return _modules.visit( [ this, rofiId ]( auto & modules ) {
        if ( auto it = modules.find( rofiId ); it != modules.end() && it->second == nullptr ) {
            it->second = this->getNewLockedModule( rofiId );
            return true;
        }
        return false;
    } );
}

void ModulesCommunication::unlockRofi( RofiId rofiId )
{
    return _modules.visit( [ rofiId ]( auto & modules ) {
        if ( auto it = modules.find( rofiId ); it != modules.end() ) {
            it->second.reset();
        }
    } );
}

std::optional< std::string > ModulesCommunication::getTopic( RofiId rofiId ) const
{
    return _modules.visit_shared( [ rofiId, &node = std::as_const( *_node ) ](
                                          const auto & modules ) -> std::optional< std::string > {
        if ( auto it = modules.find( rofiId ); it != modules.end() && it->second != nullptr ) {
            return it->second->topic( node );
        }
        return std::nullopt;
    } );
}

bool ModulesCommunication::isLocked( RofiId rofiId ) const
{
    return _modules.visit_shared( [ rofiId ]( const auto & modules ) {
        auto it = modules.find( rofiId );
        return it != modules.end() && it->second != nullptr;
    } );
}
