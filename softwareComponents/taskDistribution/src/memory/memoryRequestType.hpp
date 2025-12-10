#pragma once
#include "../distribution/distributionMessageType.hpp"

enum class MemoryRequestType
{
    MemoryDelete,
    MemoryWrite,
    MemoryRead,
    InvalidOperation,
};

MemoryRequestType mapMessageToMemoryRequest( const DistributionMessageType messageType );