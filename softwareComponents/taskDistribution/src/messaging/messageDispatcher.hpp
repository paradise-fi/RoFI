#pragma once
#include "lwip++.hpp"
#include "../functions/functionRegistry.hpp"
#include "../memory/services/memoryService.hpp"
#include "../callbacks/userCallbackInvoker.hpp"
#include "customMessageQueueManager.hpp"
#include "messageQueueManager.hpp"
#include <functional>
#include <queue>

class MessageDispatcher
{
    rofi::hal::Ip6Addr& _addr;
    UserCallbackInvoker& _callbackInvoker;
    FunctionRegistry& _functionRegistry;
    MessagingService& _messagingService;
    DistributedMemoryService& _memoryService;
    LoggingService& _loggingService;
    CustomMessageQueueManager& _customMessageQueueManager;
    MessageQueueManager& _messageQueueManager;
    int _blockingMessageTimeoutMs;

    void handleCustomMessage( const Ip6Addr& sender, uint8_t* data, size_t size );

    void handleTaskRequest( const rofi::hal::Ip6Addr& sender );

    void handleMemoryMessage( const Ip6Addr& sender, const DistributionMessageType type, uint8_t* data, size_t size );

    void handleTaskFailure( const Ip6Addr& sender, int functionId );

    void handleTaskAssignment( std::unique_ptr< TaskBase > task, std::reference_wrapper< FunctionConcept > fn );

    void handleTaskResult( const rofi::net::Ip6Addr& sender, 
        std::unique_ptr< TaskBase > task, std::reference_wrapper< FunctionConcept > fn );

    void handleTaskMessage( const Ip6Addr& sender, const DistributionMessageType type, uint8_t* data );

public:
    MessageDispatcher( rofi::hal::Ip6Addr& address, UserCallbackInvoker& callbackInvoker, FunctionRegistry& functionRegistry,
        MessagingService& messagingService, DistributedMemoryService& memoryService, LoggingService& loggingService, 
        CustomMessageQueueManager& customMessageQueueManager, MessageQueueManager& messageQueueManager, int blockingMessageTimeoutMS );

    bool dispatchMessageFromQueue();

    void dispatchMessage( const Ip6Addr& sender, DistributionMessageType messageType, uint8_t* data, size_t size );
};