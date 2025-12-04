#pragma once
#include "lwip++.hpp"
#include <vector>
#include <queue>
#include "../callbacks/userCallbackInvoker.hpp"
#include "../services/messagingService.hpp"
#include <atoms/concurrent_queue.hpp>

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
    MessageQueueManager() {}

    void pushMessage( const rofi::hal::Ip6Addr& sender, DistributionMessageType messageType, uint8_t* data, size_t size )
    {
        MessageEntry request { sender, messageType, std::vector< uint8_t >( size ) };
        if ( size > 0 )
        {
            std::memcpy( request.rawData.data(), data, size );
        }
        _messageQueue.push( request );
    }

    std::optional< MessageEntry > popMessage()
    {
        if ( _messageQueue.empty() )
        {
            return std::nullopt;
        }

        MessageEntry request = _messageQueue.pop();

        return request;
    }

    bool isEmpty()
    {
        return _messageQueue.empty();
    }
};