#include "../../distribution/distributionMessageType.hpp"

bool IsMessageTypeBlocking( DistributionMessageType messageType )
{
    return messageType == DistributionMessageType::CustomMessageBlocking ||
            messageType == DistributionMessageType::DataReadRequestBlocking ||
            messageType == DistributionMessageType::BlockingMessageResponse;
}

bool IsMessageTypeTaskMessage( DistributionMessageType messageType )
{
    return messageType == DistributionMessageType::TaskRequest 
        || messageType == DistributionMessageType::BlockingTaskRelease
        || messageType == DistributionMessageType::TaskAssignment
        || messageType == DistributionMessageType::TaskFailed
        || messageType == DistributionMessageType::TaskResult;
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
        case DistributionMessageType::BlockingMessageResponse:
            return "Response to a Blocking Message.";
        case DistributionMessageType::DataReadRequestBlocking:
            return "Blocking Data Read Request";
        case DistributionMessageType::DataReadRequest:
            return "Forwarding Data Read Request";
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