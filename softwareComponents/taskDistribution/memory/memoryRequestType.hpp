#include "../distribution/distributionMessageType.hpp"

enum MemoryRequestType
{
    MemoryDelete,
    MemoryWrite,
    MemoryRead,
    InvalidOperation,
};

MemoryRequestType mapMessageToMemoryRequest( DistributionMessageType messageType )
{
    switch (messageType)
    {
        case DistributionMessageType::DataReadRequest:
        case DistributionMessageType::DataReadRequestBlocking:
            return MemoryRequestType::MemoryRead;
        case DistributionMessageType::DataStorageRequest:
            return MemoryRequestType::MemoryWrite;
        case DistributionMessageType::DataRemovalRequest:
            return MemoryRequestType::MemoryDelete;
        default:
            return MemoryRequestType::InvalidOperation;
    }
}