#pragma once

#include "../src/messaging/messagingService.hpp"
#include "../src/callbacks/userCallbackInvoker.hpp"
#include "../src/callbacks/callbackService.hpp"
#include "memoryFacade.hpp"
#include "functionFacade.hpp"
#include "../src/workflow/workflowService.hpp"
#include "../src/election/electionService.hpp"
#include "electionProtocolBase.hpp"
#include <boost/lockfree/queue.hpp>
#include "../src/logger/loggingService.hpp"
#include "../src/messaging/messageDispatcher.hpp"

class DistributedTaskManager : public UserCallbackInvoker
{
    rofi::net::Ip6Addr _address;
    
    LoggingService _loggingService;
    FunctionRegistry _functionRegistry;
    ElectionService _election;
    CallbackService _callbackService;
    MessagingService _messaging;
    DistributedMemoryService _memoryService;
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

    void cleanUp( bool cleanSchedulers = true, bool cleanMemory = false, bool cleanMessages = true );

    /// @brief Retrieves a facade look at the callback subsystem. Used to register custom callbacks.
    CallbackFacade& callbacks();

    /// @brief Retrieves a facade look at the memory subsystem. IMPORTANT - This facade must not outlive the task manager.
    MemoryFacade memory();

    /// @brief Retrieves a facade look at the function subsystem. IMPORTANT - This facade must not outlive the task manager.
    FunctionFacade functions();
    
    LoggingService& loggingService();

    /// @brief Performs a single iteration of the task manager workflow loop.
    /// @param messageProcessingBatch - The maximum number of incoming messages in the message queue that will be processed by the dispatcher during the loop.
    void doWork( unsigned int messageProcessingBatch = 10 );
    
    /// @brief Starts the task manager by initiating the election process.
    /// @param initialElectionDelay The initial delay of the election process in seconds.
    /// @param electionCyclesBeforeStabilization The number of election cycles before a leader is considered to be elected and the task manager can start work.
    void start( int initialElectionDelay = 1, int electionCyclesBeforeStabilization = 3 );

    std::optional< Ip6Addr > getLeader();

    /// @brief Sends a custom message. This function will end immediately and not wait for a response or a timeout.
    /// @param data The data for the custom message
    /// @param dataSize The size of the data
    /// @param target The recipient
    void sendCustomMessage( uint8_t* data, size_t dataSize, std::optional< Ip6Addr > target );

    /// @brief Sends a custom blocking message. This function will end once the target module responds or a timeout runs out
    /// @param data The data for the custom message
    /// @param dataSize The size of the data
    /// @param target The recipient
    /// @return A response
    MessagingResult sendCustomMessageBlocking( uint8_t* data, size_t dataSize, Ip6Addr& target );

    /// @brief  Used to manually send a function request to the leader. You should not need to use this in a typical workflow, but it may be useful for situations where you want the follower to request a task outside of the typical workflow.
    /// @return Returns false if the module is a leader and thus a request was not sent out.
    bool requestTask();

    /// @brief Broadcasts an unblock scheduler signal to all modules.
    void broadcastUnblockSignal();

    /// @brief Sends an unblock scheduler signal to a given module.
    /// @param receiver The module address
    void sendUnblockSignal( const Ip6Addr& receiver );
};