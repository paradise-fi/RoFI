#pragma once
#include "lwip++.hpp"
#include <vector>
#include <queue>
#include "../callbacks/userCallbackInvoker.hpp"
#include <atoms/concurrent_queue.hpp>
#include "../distribution/distributionMessageType.hpp"

struct MessageEntry
{
    const rofi::hal::Ip6Addr sender;
    DistributionMessageType messageType;
    std::vector< uint8_t > rawData;
};

class MessageQueueManager
{
    atoms::ConcurrentQueue< MessageEntry > _messageQueue;

public:
    void pushMessage( const rofi::hal::Ip6Addr& sender, DistributionMessageType messageType, uint8_t* data, size_t size );

    std::optional< MessageEntry > popMessage();

    bool isEmpty();

    void clearQueue();
};