#include "simplesim/modules_communication.hpp"


using namespace rofi::simplesim;

bool ModulesCommunication::addNewModule( ModuleId moduleId )
{
    return _modules->emplace( moduleId, nullptr ).second;
}

std::optional< ModuleId > ModulesCommunication::lockFreeModule()
{
    return _modules.visit( [ this ]( auto & modules ) -> std::optional< ModuleId > {
        for ( auto & [ moduleId, moduleComm ] : modules ) {
            if ( !moduleComm ) {
                moduleComm = this->getNewLockedModule( moduleId );
                return moduleId;
            }
        }
        return std::nullopt;
    } );
}

bool ModulesCommunication::tryLockModule( ModuleId moduleId )
{
    return _modules.visit( [ this, moduleId ]( auto & modules ) {
        if ( auto it = modules.find( moduleId ); it != modules.end() && it->second == nullptr ) {
            it->second = this->getNewLockedModule( moduleId );
            return true;
        }
        return false;
    } );
}

void ModulesCommunication::unlockModule( ModuleId moduleId )
{
    return _modules.visit( [ moduleId ]( auto & modules ) {
        if ( auto it = modules.find( moduleId ); it != modules.end() ) {
            it->second.reset();
        }
    } );
}

std::optional< std::string > ModulesCommunication::getTopic( ModuleId moduleId ) const
{
    return _modules.visit_shared( [ moduleId ](
                                          const auto & modules ) -> std::optional< std::string > {
        if ( auto it = modules.find( moduleId ); it != modules.end() && it->second != nullptr ) {
            return it->second->topic();
        }
        return std::nullopt;
    } );
}

bool ModulesCommunication::isLocked( ModuleId moduleId ) const
{
    return _modules.visit_shared( [ moduleId ]( const auto & modules ) {
        auto it = modules.find( moduleId );
        return it != modules.end() && it->second != nullptr;
    } );
}
