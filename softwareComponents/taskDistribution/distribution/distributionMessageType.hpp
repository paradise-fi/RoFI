#pragma once

enum DistributionMessageType {
    TaskRequest,
    TaskAssignment,
    TaskResult,
    TaskFailed,
    BlockingTaskRelease,

    DataStorageRequest,
    DataRemovalRequest,
    DataReadRequest,
    DataReadRequestBlocking,

    CustomMessage,
    CustomMessageBlocking,
    
    BlockingMessageResponse,
};

bool IsMessageTypeBlocking( DistributionMessageType messageType );

bool IsMessageTypeTaskMessage( DistributionMessageType messageType );

const char* getMessageTypeName( DistributionMessageType messageType );