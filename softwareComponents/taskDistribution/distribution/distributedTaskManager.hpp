#pragma once

#include "services/messagingService.hpp"
#include "services/memoryFacade.hpp"
#include "services/functionFacade.hpp"
#include "services/workflowService.hpp"
#include "services/electionService.hpp"
#include "electionProtocolBase.hpp"
#include <boost/lockfree/queue.hpp>
#include "services/loggingService.hpp"
#include "callbacks/userCallbackInvoker.hpp"
#include "callbacks/callbackService.hpp"
#include "messaging/messageDispatcher.hpp"
#include "messaging/customMessageQueueManager.hpp"

class DistributedTaskManager : public UserCallbackInvoker
{
    rofi::net::Ip6Addr _address;
    
    LoggingService _loggingService;
    FunctionRegistry _functionRegistry;
    ElectionService _election;
    CallbackService _callbackService;
    MessagingService _messaging;
    DistributedMemoryService _memoryService;
    CustomMessageQueueManager _customMessageQueueManager;
    MessageQueueManager _messageQueueManager;
    MessageDispatcher _messageDispatcher;
    WorkFlowService _workFlowService;

    int _blockingMessageTimeoutMs;

    void onElectionSuccesful( const Ip6Addr& leader )
    {
        if ( _memoryService.isMemoryRegistered() )
        {
            _memoryService.setLeader( _election.getLeader() ); 
        }

        if ( _address == leader )
        {
            _loggingService.logInfo( "I have been elected as the leader." );
            return;
        }

        _functionRegistry.clearTasks();
        std::ostringstream stream;
        stream << "I am a follower of " << leader;
        _loggingService.logInfo( stream.str() );
        requestTask();
    }

    virtual MessagingResult invokeUserCallback( CallbackType cbType, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size ) override
    {
        int taskId;
        bool stopRequestPipeline;
        switch ( cbType )
        {
            case CallbackType::CompleteBlockingMessageCb:
                _messaging.completeBlockingMessage( data, size );
                return MessagingResult( true );
            case CallbackType::CustomMessageCb:
                _callbackService.invokeOnCustomMessage( *this, sender, data, size );
                return MessagingResult( true );
            case CallbackType::CustomMessageBlockingCb:
                return _callbackService.invokeOnCustomMessageBlocking( *this, sender, data, size );
            case CallbackType::TaskFailureCb:
                taskId = 0;
                if ( size == sizeof( int ) )
                {
                    taskId = as< int >( data );
                }
                _callbackService.invokeOnTaskFailure( *this, sender, taskId );
                return MessagingResult( true );

            case CallbackType::TaskRequestCb:
                // Semantics -> true means that the task request pipeline ends with this call. False means we still need to queue the task request.
                stopRequestPipeline = _callbackService.invokeOnTaskRequest( *this, sender );
                return MessagingResult( stopRequestPipeline );

            default:
                std::ostringstream errorMessageStream;
                errorMessageStream << "Callback type not registered. " << std::endl;
                _loggingService.logError( errorMessageStream.str() );
                return MessagingResult( false );
        }

        return MessagingResult( false );
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
      _callbackService( _election, _loggingService ),
      _messaging( address, DISTRIBUTION_PORT, distributor, std::move( pcb ), _messageQueueManager, METHOD_ID ),
      _memoryService( distributor, _messaging, address, _loggingService, _callbackService, blockingMessageTimeoutMs ),
      _customMessageQueueManager( *this, _messaging ),
      _messageDispatcher( address, *this, _functionRegistry, _messaging, _memoryService, _loggingService, _customMessageQueueManager, _messageQueueManager, blockingMessageTimeoutMs ),
      _workFlowService( _messaging.sender(), _functionRegistry, _memoryService, _loggingService, _customMessageQueueManager, _messageDispatcher ),
      _blockingMessageTimeoutMs( blockingMessageTimeoutMs )
    {}

    template< std::derived_from< LoggerBase > Logger >
    void useLogger( const Logger& logger )
    {
        _loggingService.useLogger( logger );
    }

    CallbackFacade& callbacks()
    {
        return _callbackService;
    }

    [[nodiscard]] MemoryFacade memory()
    {
        return MemoryFacade( _memoryService );
    }

    [[nodiscard]] FunctionFacade functions()
    {
        return FunctionFacade( _functionRegistry );
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
    
    void start( int initialElectionDelay, int electionCyclesBeforeStabilization = 3 )
    {
        _election.start( initialElectionDelay,
            [ this ]( const Ip6Addr& leader) {
                onElectionSuccesful( leader );
            }, electionCyclesBeforeStabilization);
    }

    std::optional< Ip6Addr > getLeader()
    {
        if ( _election.isElectionComplete() )
        {
            return _election.getLeader();
        }

        return std::nullopt;
    }

    void sendCustomMessage( uint8_t* data, size_t dataSize, std::optional< Ip6Addr > target )
    {
        if ( !target.has_value() )
        {
            return _messaging.sender().broadcastMessage( DistributionMessageType::CustomMessage, data, dataSize, METHOD_ID );
        }

        auto packet = PBuf::allocate( static_cast< int >( dataSize ) );
        std::memcpy( packet.payload(), data, dataSize );
        _messaging.sender().sendMessage( DistributionMessageType::CustomMessage, std::move( packet ), target.value() );
    }

    MessagingResult sendCustomMessageBlocking( uint8_t* data, size_t dataSize, Ip6Addr& target )
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

    void broadcastUnblockSignal()
    {
        _messaging.sender().broadcastMessage(DistributionMessageType::BlockingTaskRelease, METHOD_ID);
    }

    void sendUnblockSignal( Ip6Addr& receiver )
    {
        _messaging.sender().sendMessage(DistributionMessageType::BlockingTaskRelease, receiver );
    }
};