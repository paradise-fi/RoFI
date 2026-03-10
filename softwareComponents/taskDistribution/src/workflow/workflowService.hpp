#include "../memory/services/memoryService.hpp"
#include "../functions/functionRegistry.hpp"
#include "../logger/loggingService.hpp"
#include "../messaging/messagingService.hpp"
#include "../messaging/messageDispatcher.hpp"

class WorkFlowService
{
    MessagingService& _messagingService;
    FunctionRegistry& _functionRegistry;
    DistributedMemoryService& _memoryService;
    LoggingService& _loggingService;
    MessageDispatcher& _messageDispatcher;

    void tryDistributeNewTask( int methodId );

    void distributeTask( FunctionDistributionType distributionType, TaskBase& task, 
        int methodId, const Ip6Addr& requester );

public:
    WorkFlowService(MessagingService& messagingService, FunctionRegistry& functionRegistry, DistributedMemoryService& memoryService,
        LoggingService& loggingService, MessageDispatcher& messageDispatcher );

    void doWorkLeader( int methodId, unsigned int messageProcessingBatch = 5 );

    void doWorkFollower( const Ip6Addr& address, const Ip6Addr& leader, unsigned int messageProcessingBatch = 5 );
};