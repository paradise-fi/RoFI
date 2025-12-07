#pragma once

#include "services/messagingService.hpp"
#include "callbacks/userCallbackInvoker.hpp"
#include "callbacks/callbackService.hpp"
#include "services/memoryFacade.hpp"
#include "services/functionFacade.hpp"
#include "services/workflowService.hpp"
#include "services/electionService.hpp"
#include "electionProtocolBase.hpp"
#include <boost/lockfree/queue.hpp>
#include "services/loggingService.hpp"
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

    void onElectionSuccesful( const Ip6Addr& leader );

    virtual MessagingResult invokeUserCallback( CallbackType cbType, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size ) override;

public:
    static constexpr unsigned int METHOD_ID = 3;
    static constexpr int DISTRIBUTION_PORT = 7071;

    DistributedTaskManager(
        std::unique_ptr< ElectionProtocolBase > election,
        Ip6Addr& address,
        MessageDistributor& distributor,
        std::unique_ptr< udp_pcb > pcb,
        int blockingMessageTimeoutMs = 300 ); 

    CallbackFacade& callbacks();

    [[nodiscard]] MemoryFacade memory();

    [[nodiscard]] FunctionFacade functions();
    
    LoggingService& loggingService();

    void doWork();
    
    void start( int initialElectionDelay, int electionCyclesBeforeStabilization = 3 );

    std::optional< Ip6Addr > getLeader();

    void sendCustomMessage( uint8_t* data, size_t dataSize, std::optional< Ip6Addr > target );

    MessagingResult sendCustomMessageBlocking( uint8_t* data, size_t dataSize, Ip6Addr& target );

    /// @brief  Used to manually send a function request to the leader. You should not need to use this in a typical workflow, but it may be useful for situations where you want the follower to request a task outside of the typical workflow.
    /// @return Returns false if the module is a leader and thus a request was not sent out.
    bool requestTask();

    void broadcastUnblockSignal();

    void sendUnblockSignal( Ip6Addr& receiver );

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

};