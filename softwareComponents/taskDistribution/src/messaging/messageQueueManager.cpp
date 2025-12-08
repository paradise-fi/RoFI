#include "messageQueueManager.hpp"

void MessageQueueManager::pushMessage( const rofi::hal::Ip6Addr& sender, DistributionMessageType messageType, uint8_t* data, size_t size )
{
    MessageEntry request { sender, messageType, std::vector< uint8_t >( size ) };
    if ( size > 0 )
    {
        std::memcpy( request.rawData.data(), data, size );
    }
    _messageQueue.push( request );
}

std::optional< MessageEntry > MessageQueueManager::popMessage()
{
    if ( _messageQueue.empty() )
    {
        return std::nullopt;
    }

    MessageEntry request = _messageQueue.pop();

    return request;
}

bool MessageQueueManager::isEmpty()
{
    return _messageQueue.empty();
}