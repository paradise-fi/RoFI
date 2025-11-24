#pragma once

#include "messagingService.hpp"
#include "functionRegistry.hpp"
#include "services/memoryService.hpp"
#include "services/workflowService.hpp"
#include "services/electionService.hpp"
#include "electionProtocolBase.hpp"
#include <boost/lockfree/queue.hpp>
#include "services/loggingService.hpp"
#include "messaging/userCallbackInvoker.hpp"
#include "messaging/messageDispatcher.hpp"
#include "messaging/customMessageQueueManager.hpp"

class DistributedTaskManager : public UserCallbackInvoker
{
    rofi::net::Ip6Addr _address;
    
    LoggingService _loggingService;
    FunctionRegistry _functionRegistry;
    ElectionService _election;
    MessagingService _messaging;
    DistributedMemoryService _memoryService;
    CustomMessageQueueManager _customMessageQueueManager;
    WorkFlowService _workFlowService;
    MessageDispatcher _messageDispatcher;
    MessageDistributor& _messageDistributor;

    int _blockingMessageTimeoutMs;

    std::function< bool( DistributedTaskManager& manager, const rofi::net::Ip6Addr& requester ) > _onTaskRequest;
    std::function< void( DistributedTaskManager& manager, const rofi::net::Ip6Addr& sender, const int functionId ) > _onTaskFailure;
    std::function< void( DistributedTaskManager& manager, const rofi::net::Ip6Addr& sender, uint8_t* data, const size_t size ) > _onCustomMessage;
    std::function< MessagingResult( DistributedTaskManager& manager, const rofi::net::Ip6Addr& sender, uint8_t* data, const size_t size ) > _onCustomMessageBlocking;

    void onElectionSuccesful( const Ip6Addr& leader )
    {
        if ( _memoryService.isMemoryRegistered() )
        {
            _memoryService.setLeader( _election.getLeader() ); 
        }

        if ( _address == leader )
        {
            _functionRegistry.clearTasks();
            _loggingService.logInfo( "I have been elected as the leader." );
            return;
        }

        std::ostringstream stream;
        stream << "I am a follower of " << leader;
        _loggingService.logInfo( stream.str() );
        requestTask();
    }

    virtual MessagingResult invokeUserCallback( CallbackType cbType, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size ) override
    {
        MessagingResult result;
        result.success = true;
        switch ( cbType )
        {
            case CallbackType::CompleteBlockingMessageCb:
                _messaging.completeBlockingMessage( data, size );
                break;
            case CallbackType::CustomMessageCb:
                if ( _onCustomMessage )
                {
                    _onCustomMessage( *this, sender, data, size );
                }
                else
                {
                    _loggingService.logError("Received custom message but no callback to handle it is registered.");
                    result.success = false;
                }
                break;
            
            case CallbackType::CustomMessageBlockingCb:
                if ( _onCustomMessageBlocking )
                {
                    return _onCustomMessageBlocking( *this, sender, data, size );
                }
                else
                {
                    _loggingService.logError("Received blocking custom message but no callback to handle it is registered.");
                    result.success = false;
                }
                break;

            case CallbackType::TaskFailureCb:
                if ( _onTaskFailure )
                {
                    int taskId = 0;
                    if ( size == sizeof( int ) )
                    {
                        taskId = as< int >( data );
                    }
                    _onTaskFailure( *this, sender, taskId );
                }
                break;

            case CallbackType::TaskRequestCb:
                if ( _onTaskRequest )
                {
                    // Semantics -> true means that the task request pipeline ends with this call. False means we still need to queue the task request.
                    result.success = _onTaskRequest( *this, sender );
                }
                else
                {
                    result.success = false;
                }
                break;

            default:
                std::ostringstream errorMessageStream;
                errorMessageStream << "Callback type not registered. " << std::endl;
                _loggingService.logError( errorMessageStream.str() );
                result.success = false;
                break;
        }
        return result;
    }

    void onMessage( const Ip6Addr& sender, const DistributionMessageType type, uint8_t* data, unsigned int size )
    {
        _messageDispatcher.dispatchMessage( sender, type, data, size );
    }

public:
    static constexpr unsigned int METHOD_ID = 3;
    static constexpr int DISTRIBUTION_PORT = 7071;

    DistributedTaskManager(
        std::unique_ptr< ElectionProtocolBase > election,
        Ip6Addr& address,
        MessageDistributor& distributor,
        std::unique_ptr< udp_pcb > pcb,
        int blockingMessageTimeoutMs = 300 ) 
    : _address( address ),
      _functionRegistry( _loggingService ),
      _election( std::move( election ) ),
      _messaging( address, DISTRIBUTION_PORT, distributor,
        [ this ]( Ip6Addr sender, DistributionMessageType messageType, uint8_t* data, unsigned int size )
        {
            onMessage( sender, messageType, data, size );
        },
        std::move( pcb ) ),
      _memoryService( distributor, _messaging, address, _loggingService, blockingMessageTimeoutMs ),
      _customMessageQueueManager( *this, _messaging ),
      _workFlowService( _messaging.sender(), _functionRegistry, _memoryService, _loggingService, _customMessageQueueManager ),
      _messageDispatcher( address, *this, _functionRegistry, _messaging, _memoryService, _loggingService, _customMessageQueueManager, blockingMessageTimeoutMs ),
      _messageDistributor( distributor ),
      _blockingMessageTimeoutMs( blockingMessageTimeoutMs )
    {}

    template< std::derived_from< LoggerBase > Logger >
    void useLogger( const Logger& logger )
    {
        _loggingService.useLogger( logger );
    }

    /// @brief Registers a callback that is called when leader election fails.
    /// @param callback The callback function.
    /// @return True if the registration succeeded.
    bool registerLeaderFailureCallback( std::function< void() >&& callback ) 
    { 
        return _election.registerLeaderFailureCallback( std::forward< std::function< void() > >( callback ) ); 
    }

    /// @brief Unregisters a callback for leader election failure.
    /// @return True if the function was unregistered.
    bool unregisterLeaderFailureCallback() { return _election.unregisterLeaderFailureCallback(); }
    
    /// @brief Registers a callback that is called when a task request is received.
    /// @param callback Your custom callback. Returns true if the task request pipeline should not continue after this callback (e.g. to avoid double-scheduling of a task)
    void registerTaskRequestCallback( std::function< bool(DistributedTaskManager& manager, const rofi::net::Ip6Addr& requester ) >&& callback )
    { 
        _onTaskRequest = std::forward< std::function< bool(DistributedTaskManager& manager, const rofi::net::Ip6Addr& requester ) > >( callback );
    }

    /// @brief Unregisters a callback that is called when a task request is received.
    void unregisterTaskRequestCallback() { _onTaskRequest = nullptr; }

    void registerTaskFailedCallback( std::function< void(DistributedTaskManager& manager, const rofi::net::Ip6Addr& sender, const int functionId ) >&& callback ) 
    { 
        _onTaskFailure = std::forward< std::function< void(DistributedTaskManager& manager, const rofi::net::Ip6Addr& sender, const int functionId ) > >( callback );
    }

    void unregisterTaskFailedCallback() { _onTaskFailure = nullptr; }

    void registerNonBlockingCustomMessageCallback( std::function< void( DistributedTaskManager& manager, const rofi::net::Ip6Addr& sender, uint8_t* dataBuffer, const unsigned int bufferSize ) >&& callback )
    { 
        _onCustomMessage = std::forward< std::function< void( DistributedTaskManager& manager, const rofi::net::Ip6Addr& sender, uint8_t* dataBuffer, const unsigned int bufferSize ) > >( callback );
    }

    void unregisterNonBlockingCustomMessageCallback() { _onCustomMessage = nullptr; }

    void registerBlockingCustomMessageCallback( std::function< MessagingResult( DistributedTaskManager& manager, const rofi::net::Ip6Addr& sender, uint8_t* dataBuffer, const unsigned int bufferSize ) >&& callback )
    { 
        _onCustomMessageBlocking = std::forward< std::function< MessagingResult( DistributedTaskManager& manager, const rofi::net::Ip6Addr& sender, uint8_t* dataBuffer, const unsigned int bufferSize ) > >( callback );
    }

    void unregisterBlockingCustomMessageCallback() { _onCustomMessageBlocking = nullptr; }

    DistributedMemoryService& memoryService()
    {
        return _memoryService;
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
            _loggingService.logError("Election protocol not running.");
            return;
        }

        if ( !_election.isElectionComplete() )
        {
            return;
        }

        if ( _address == _election.getLeader() )
        {
            _workFlowService.doWorkLeader( METHOD_ID );
            return;
        }
        
        _workFlowService.doWorkFollower( _address, _election.getLeader() );
    }
    
    void start( int moduleId )
    {
        _messageDistributor.registerMethod( METHOD_ID, 
            [ this ] ( Ip6Addr sender, uint8_t* data, unsigned int size ) 
            {
                DistributionMessageType type = as< DistributionMessageType >( data );
                onMessage( sender, type, data, size ); 
            },
            [] () { return; } );

        _election.start( moduleId,
            [ this ]( const Ip6Addr& leader) {
                onElectionSuccesful( leader );
            });
    }

    void broadcastUnblockSignal()
    {
        _messaging.sender().broadcastMessage(DistributionMessageType::BlockingTaskRelease, METHOD_ID);
    }

    void sendUnblockSignal( Ip6Addr& receiver )
    {
        _messaging.sender().sendMessage(DistributionMessageType::BlockingTaskRelease, receiver );
    }

    std::optional< Ip6Addr > getLeader()
    {
        if ( _election.isElectionComplete() )
        {
            return _election.getLeader();
        }

        return std::nullopt;
    }

    void sendCustomMessage( uint8_t* data, unsigned int dataSize, std::optional< Ip6Addr > target )
    {
        if ( !target.has_value() )
        {
            return _messaging.sender().broadcastMessage( DistributionMessageType::CustomMessage, data, dataSize, METHOD_ID );
        }

        auto packet = PBuf::allocate( static_cast< int >( dataSize ) );
        std::memcpy( packet.payload(), data, dataSize );
        _messaging.sender().sendMessage( DistributionMessageType::CustomMessage, std::move( packet ), target.value() );
    }

    MessagingResult sendCustomMessageBlocking( uint8_t* data, unsigned int dataSize, Ip6Addr& target )
    {
        return _messaging.sendMessageBlocking( target, DistributionMessageType::CustomMessageBlocking, data, dataSize, _blockingMessageTimeoutMs );
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
    std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( const std::string& functionName )
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

    template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments,
              std::derived_from< DistributedFunction< Result, Arguments... > > Func >
    bool registerFunction( const Func& function )
    {
        return _functionRegistry.registerFunction< Result, Arguments... >( function );
    }

    /// @brief Removes all tasks from all schedulers on this module.
    void clearAllTasks()
    {
        _functionRegistry.clearTasks();
    }
};