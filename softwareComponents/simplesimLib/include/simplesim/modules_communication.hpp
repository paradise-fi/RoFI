#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <vector>

#include <gazebo/transport/transport.hh>

#include "atoms/guarded.hpp"
#include "command_handler.hpp"
#include "distributor.hpp"
#include "locked_module_communication.hpp"

#include <rofiCmd.pb.h>


namespace rofi::simplesim
{
class ModulesCommunication
{
    using LockedModuleCommunicationPtr = std::unique_ptr< LockedModuleCommunication >;

public:
    ModulesCommunication( std::shared_ptr< CommandHandler > commandHandler,
                          gazebo::transport::NodePtr node )
            : _commandHandler( std::move( commandHandler ) )
            , _node( std::move( node ) )
            , _distributor( *this->_node, *this )
    {
        assert( _node );
        assert( _commandHandler );
        _modules.visit( [ moduleIds = _commandHandler->getModuleIds() ]( auto & modules ) {
            for ( auto moduleId : moduleIds ) {
                [[maybe_unused]] auto result = modules.emplace( moduleId, nullptr );
                assert( result.second );
            }
        } );
    }

    ModulesCommunication( const ModulesCommunication & ) = delete;
    ModulesCommunication & operator=( const ModulesCommunication & ) = delete;

    // Returns true if the insertion was succesful
    // Returns false if the moduleId was already registered
    bool addNewModule( ModuleId moduleId );

    std::optional< ModuleId > lockFreeModule();
    bool tryLockModule( ModuleId moduleId );
    void unlockModule( ModuleId moduleId );

    std::optional< std::string > getTopic( ModuleId moduleId ) const;
    bool isLocked( ModuleId moduleId ) const;

    template < typename F >
    void forEachLockedModule( F && function ) const
    {
        _modules.visit_shared( [ &function ]( const auto & modules ) {
            for ( auto & [ moduleId, moduleComm ] : modules ) {
                if ( moduleComm ) {
                    function( moduleId, moduleComm->topic() );
                }
            }
        } );
    }

    template < typename F >
    void forEachFreeModule( F && function ) const
    {
        _modules.visit_shared( [ &function ]( const auto & modules ) {
            for ( const auto & [ moduleId, moduleComm ] : modules ) {
                if ( !moduleComm ) {
                    function( moduleId );
                }
            }
        } );
    }

    template < typename ResponsesContainer >
    void sendRofiResponses( ResponsesContainer && responses )
    {
        _modules.visit( [ &responses ]( auto & modules ) {
            for ( auto & resp : responses ) {
                if ( auto it = modules.find( resp.rofiid() ); it != modules.end() && it->second ) {
                    it->second->sendResponse( resp );
                }
            }
        } );
    }

private:
    std::atomic_int topicNameCounter = 0;
    std::string getNewTopicName()
    {
        return "rofi_uid_" + std::to_string( topicNameCounter++ );
    }


    LockedModuleCommunicationPtr getNewLockedModule( ModuleId moduleId )
    {
        // Cannot lock any mutexes (in particular cannot access `_modules`)
        assert( _commandHandler );
        assert( _node );
        return std::make_unique< LockedModuleCommunication >( *_commandHandler,
                                                              *_node,
                                                              getNewTopicName(),
                                                              moduleId );
    }

private:
    std::shared_ptr< CommandHandler > _commandHandler;

    gazebo::transport::NodePtr _node;

    atoms::Guarded< std::map< ModuleId, LockedModuleCommunicationPtr >, std::shared_mutex >
            _modules;

    Distributor _distributor;
};

} // namespace rofi::simplesim
