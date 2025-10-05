#pragma once

#include "messagingWrapper.hpp"
#include "functionRegistry.hpp"
#include "services/memoryService.hpp"
#include "services/workflowService.hpp"
#include "services/electionService.hpp"
#include "electionProtocolBase.hpp"
#include <boost/lockfree/queue.hpp>
#include "services/loggingService.hpp"

using namespace rofi::hal;
using namespace rofi::net;
using namespace rofi::leadership;
using namespace std::chrono_literals;

class DistributedTaskManager
{
    const unsigned int METHOD_ID = 3;

    rofi::net::Ip6Addr _address;
    
    LoggingService _loggingService;
    FunctionRegistry _functionRegistry;
    ElectionService _election;
    MessagingWrapper _messaging;
    DistributedMemoryService _memoryService;
    WorkFlowService _workFlowService;

    std::optional< std::function< bool(DistributedTaskManager& manager, rofi::net::Ip6Addr& requester ) > > _onTaskRequest;
    std::optional< std::function< void(DistributedTaskManager& manager, rofi::net::Ip6Addr& sender, int functionId ) > > _onTaskFailure;

    void onElectionSuccesful( const Ip6Addr& leader )
    {
        if ( _memoryService.isMemoryRegistered() )
        {
            _memoryService.setLeader( _election.getLeader() ); 
        }

        if ( _address == leader )
        {
            _functionRegistry.clearTasks();
            std::cout << "I am the leader." << std::endl;
            return;
        }

        std::cout << "I am the follower." << std::endl;
        requestTask();
    }

    void onMessage( Ip6Addr& sender, DistributionMessageType type, uint8_t* data, unsigned int size )
    {
        if ( type == DistributionMessageType::BlockingTaskRelease )
        {
            _loggingService.logInfo("Blocking task release.");
            _functionRegistry.unblockTaskSchedulers( true );
            return;
        }

        if ( type == DistributionMessageType::TaskRequest )
        {
            std::ostringstream stream;
            stream << "Received task request from " << sender;
            _loggingService.logInfo( stream.str() );

            // Should enqueing the task request be done by the user?
            if ( _onTaskRequest.has_value() && _onTaskRequest.value()( *this, sender ) )
            {
                return;
            }
            
            if ( !_functionRegistry.enqueueTaskRequest( sender ) )
            {
                _loggingService.logError( "Failed to enqueue task request." );
            }
            return;
        }

        if ( type == DistributionMessageType::DataStorageRequest )
        {
            std::ostringstream stream;
            stream << "Received Data Storage request from " << sender;
            _loggingService.logInfo( stream.str() );

            if ( _memoryService.isMemoryRegistered() )
            {
                unsigned int offset = sizeof( DistributionMessageType ) + Ip6Addr::size();
                return _memoryService.onStorageMessage( sender, data + offset, size - offset, false );
            }
            else
            {
                _loggingService.logError("Memory not registered.");
            }
        }

        if ( type == DistributionMessageType::DataRemovalRequest )
        {
            std::ostringstream stream;
            stream << "Received data removal request from " << sender;
            _loggingService.logInfo( stream.str() );

            if ( _memoryService.isMemoryRegistered() )
            {
                unsigned int offset = sizeof( DistributionMessageType ) + Ip6Addr::size();
                return _memoryService.onStorageMessage( sender, data + offset, size - offset, true );
            }
            else
            {
                _loggingService.logWarning( "Memory not registered." );
            }
        }

        int functionId = as< int >( data + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) );

        if ( type == DistributionMessageType::TaskFailed )
        {
            std::ostringstream stream;
            stream << "Received Task Failure from " << sender;
            _loggingService.logInfo( stream.str() );

            // TODO: Consider if this is necessary. Maybe there should be a break in the workflow instead?
            if ( _onTaskFailure.has_value() )
            {
                _onTaskFailure.value()( *this, sender, functionId );
            }
            return;
        }

        std::optional<std::reference_wrapper< FunctionConcept > > fn = _functionRegistry.getFunction( functionId );

        if ( !fn )
        {
            std::ostringstream stream;
            stream << "Function with ID " << functionId << " not found.";
            _loggingService.logError( stream.str() );
            return;
        }

        auto task = _functionRegistry.getTaskFromBuffer(
            data + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ), 
            functionId );

        if ( task == nullptr )
        {
            std::ostringstream stream;
            stream << "Unable to parse task for function with ID " << functionId << ".";
            _loggingService.logError( stream.str() );
            return;
        }

        int taskId = task->id();

        if ( type == DistributionMessageType::TaskAssignment )
        {
            std::ostringstream stream;
            stream << "Received Task Assignment from " << sender << " for task with ID " << taskId;
            _loggingService.logInfo( stream.str() );
            
            if ( !_functionRegistry.enqueueTask( _address, std::move( task ), fn.value().get().completionType() ) )
            {
                stream.clear();
                stream << "Failed to register task assignment for function " << functionId;
                _loggingService.logError( stream.str() );
            }

            return;
        }

        if ( type == DistributionMessageType::TaskResult )
        {
            std::ostringstream stream;
            stream << "Received result from " << sender << " for Task with ID " << taskId;
            _loggingService.logInfo( stream.str() );
            
            if ( task->status() == TaskStatus::RepeatDistributed )
            {
                if ( !_functionRegistry.enqueueTask( sender, std::move( task ), fn.value().get().completionType() ) )
                {
                    stream.clear();
                    stream << "Failed to register task for repeat for function " << functionId;
                    _loggingService.logError( stream.str() );
                    return;
                }
                
                if ( !_functionRegistry.enqueueTaskRequest( sender ) )
                {
                    _loggingService.logError( "Failed to enqueue task request." );
                }

                return;
            }

            if ( !_functionRegistry.enqueueTaskResult( std::move( task ), sender ) )
            {
                stream.clear();
                stream << "Failed to persist result from task " << taskId << " for function " << functionId;
                _loggingService.logError( stream.str() );
            }
        }
    }

public:
    static const int DISTRIBUTION_PORT = 7071;

    DistributedTaskManager(
        std::unique_ptr< ElectionProtocolBase > election,
        Ip6Addr& address,
        MessageDistributor* distributor,
        std::unique_ptr< udp_pcb > pcb) 
    : _address( address ),
      _functionRegistry( _loggingService ),
      _election( std::move( election ),
        [ this ]( const Ip6Addr& leader) {
            onElectionSuccesful( leader );
        }),
      _messaging( address, DISTRIBUTION_PORT, distributor,
        [ this ]( Ip6Addr sender, DistributionMessageType messageType, uint8_t* data, unsigned int size )
        {
            onMessage( sender, messageType, data, size );
        },
        std::move( pcb ) ),
      _memoryService( distributor, _messaging.sender(), address, _loggingService ),
      _workFlowService( _messaging.sender(), _functionRegistry, _memoryService, _loggingService )
    {
        distributor->registerMethod( METHOD_ID, 
            [ this ] ( Ip6Addr sender, uint8_t* data, unsigned int size ) 
            {
                DistributionMessageType type = as< DistributionMessageType >( data );
                onMessage( sender, type, data, size ); 
            },
            [] () { return; } );
    }

    template< std::derived_from< LoggerBase > Logger >
    void useLogger( const Logger& logger )
    {
        _loggingService.useLogger( logger );
    }

    /// @brief Registers a callback that is called when leader election fails.
    /// @param callback The callback function.
    /// @return True if the registration succeeded.
    bool registerLeaderFailureCallback( std::function< void() > callback )
    {
        return _election.registerLeaderFailureCallback( callback );
    }

    /// @brief Unregisters a callback for leader election failure.
    /// @return True if the function was unregistered.
    bool unregisterLeaderFailureCallback()
    {
        return _election.unregisterLeaderFailureCallback();
    }
    
    /// @brief Registers a callback that is called when a task request is received.
    /// @param callback Your custom callback. Returns true if the task request pipeline should not continue after this callback (e.g. to avoid double-scheduling of a task)
    void registerTaskRequestCallback( std::function< bool(DistributedTaskManager& manager, rofi::net::Ip6Addr& requester ) > callback )
    {
        _onTaskRequest = callback;
    }

    /// @brief Unregisters a callback that is called when a task request is received.
    void unregisterTaskRequestCallback()
    {
        _onTaskRequest.reset();
    }

    void registerTaskFailedCallback( std::function< void(DistributedTaskManager& manager, rofi::net::Ip6Addr& sender, int functionId ) > callback )
    {
        _onTaskFailure = callback;
    }

    DistributedMemoryService& memoryService()
    {
        return _memoryService;
    }
    MessageSender& sender()
    {
        return _messaging.sender();
    } 
    FunctionRegistry& functionRegistry()
    {
        return _functionRegistry;
    }
    LoggingService& loggingService()
    {
        return _loggingService;
    }

    void doWork()
    {
        if ( !_election.isRunning() )
        {
            std::cout << "The election protocol is not running!" << std::endl;
            return;
        }

        if ( !_election.isElectionComplete() )
        {
            return;
        }

        if ( _address == _election.getLeader() )
        {
            return _workFlowService.doWorkLeader( METHOD_ID );
        }
        
        _workFlowService.doWorkFollower( _address, _election.getLeader() );
    }
    
    void start( int moduleId )
    {
        _election.start( moduleId );
    }

    void broadcastUnblockSignal()
    {
        _messaging.sender().broadcastMessage(DistributionMessageType::BlockingTaskRelease, METHOD_ID);
    }

    void sendUnblockSignal( Ip6Addr& receiver )
    {
        _messaging.sender().sendMessage(DistributionMessageType::BlockingTaskRelease, receiver );
    }

    /// @brief Used to manually send a function execution order to a module. This should be done from the leader to the follower. You do NOT need to use this if you use functionHandle instead.
    template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments > 
    bool sendFunctionExecutionOrder( int functionId, const Ip6Addr& target, int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments )
    {
        auto functionHandle = _functionRegistry.getFunctionHandle< Result, Arguments... >( functionId );
        if ( functionHandle.has_value() )
        {
            return functionHandle.value()( target, priority, setTopPriority, arguments );
        }
        return false;
    }

    /// @brief Used to manually send a function execution order to a module. This should be done from the leader to the follower. You do NOT need to use this if you use functionHandle instead.
    template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments > 
    bool sendFunctionExecutionOrder( std::string functionName, const Ip6Addr& target, int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments )
    {
        auto functionHandle = _functionRegistry.getFunctionHandle< Result, Arguments... >( functionName );
        if ( functionHandle.has_value() )
        {
            return functionHandle.value()( target, priority, setTopPriority, arguments );
        }
        return false;
    }

    /// @brief  Used to manually send a function request to the leader. You should not need to use this in a typical workflow, but it may be useful for situations where you want the follower to request a task outside of the typical workflow.
    /// @return Returns false if the module is a leader and thus a request was not sent out.
    bool requestTask()
    {
        if ( _election.getLeader() == _address )
        {
            return false;
        }

        auto emptyTask = Task< int >( 0 );
        _messaging.sender().sendMessage( DistributionMessageType::TaskRequest, emptyTask, _election.getLeader() );
        return true;
    }

    /// @brief Retrieves a function handle. The function handle is used for invoking a function over the network.
    /// @tparam Result A trivially copyable type, or a type that implements Serializable. Denotes the type of the function's result. 
    /// @tparam ...Arguments A pack of trivially copyable types, or types that implement Serializable. Denotes the types of the function's parameters.
    /// @param functionName The name of the distributed function.
    /// @return std::nullopt if the function does not exist, otherwise the function handle is returned.
    template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
    std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( std::string functionName )
    {
        return _functionRegistry.getFunctionHandle< Result, Arguments... >( functionName );
    }

    /// @brief Retrieves a function handle. The function handle is used for invoking a function over the network.
    /// @tparam Result A trivially copyable type, or a type that implements Serializable. Denotes the type of the function's result. 
    /// @tparam ...Arguments A pack of trivially copyable types, or types that implement Serializable. Denotes the types of the function's parameters.
    /// @param functionId The ID of the distributed function. You may also use the std::string variant for more human-friendly retrieval.
    /// @return std::nullopt if the function does not exist, otherwise the function handle is returned.
    template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
    std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( int functionId )
    {
        return _functionRegistry.getFunctionHandle< Result, Arguments... >( functionId );
    }

    template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
    bool registerFunction( std::unique_ptr< DistributedFunction< Result, Arguments... > > function )
    {
        if ( function == nullptr )
        {
            return false;
        }

        return _functionRegistry.registerFunction< Result, Arguments... >( std::move( function ) );
    }
};