#include "../distribution/distributionMessageType.hpp"

enum MemoryRequestType
{
    MemoryDelete,
    MemoryWrite,
    MemoryRead,
    InvalidOperation,
};

MemoryRequestType mapMessageToMemoryRequest( const DistributionMessageType messageType );