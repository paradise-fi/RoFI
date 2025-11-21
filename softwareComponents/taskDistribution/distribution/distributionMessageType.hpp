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
    DataReadResponseBlocking,

    CustomMessage,
    CustomMessageBlocking,
    CustomMessageResponseBlocking,
};

bool IsMessageTypeBlocking( DistributionMessageType messageType )
{
    return messageType != DistributionMessageType::CustomMessageBlocking &&
            messageType != DistributionMessageType::DataReadRequestBlocking &&
            messageType != DistributionMessageType::CustomMessageResponseBlocking &&
            messageType != DistributionMessageType::DataReadResponseBlocking;
}

const char* getMessageTypeName( DistributionMessageType messageType )
{
    switch (messageType)
    {
        case DistributionMessageType::BlockingTaskRelease:
            return "Blocking Task Release";
        case DistributionMessageType::CustomMessage:
            return "Custom Message";
        case DistributionMessageType::CustomMessageBlocking:
            return "Blocking Custom Message";
        case DistributionMessageType::CustomMessageResponseBlocking:
            return "Blocking Custom Message Response";
        case DistributionMessageType::DataReadRequestBlocking:
            return "Blocking Data Read Request";
        case DistributionMessageType::DataReadRequest:
            return "Forwarding Data Read Request";
        case DistributionMessageType::DataReadResponseBlocking:
            return "Blocking Data Read Response";
        case DistributionMessageType::DataRemovalRequest:
            return "Data Removal Request";
        case DistributionMessageType::DataStorageRequest:
            return "Data Storage Request";
        case DistributionMessageType::TaskAssignment:
            return "Task Assignment";
        case DistributionMessageType::TaskFailed:
            return "Task Failure";
        case DistributionMessageType::TaskRequest:
            return "Task Request";
        case DistributionMessageType::TaskResult:
            return "Task Result";
        default:
            return "Invalid Message Type";
    }
}