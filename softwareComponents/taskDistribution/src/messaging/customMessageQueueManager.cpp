#include "customMessageQueueManager.hpp"

CustomMessageQueueManager::CustomMessageQueueManager( UserCallbackInvoker& callbackInvoker, MessagingService& messagingService )
: _callbackInvoker( callbackInvoker ), _messagingService( messagingService ) {}

void CustomMessageQueueManager::emplaceRequest( const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size )
{
    CustomMessageRequest request { sender, std::vector< uint8_t >( size ) };
    if ( size > 0 )
    {
        std::memcpy( request.rawData.data(), data, size );
    }
    _customMessageBlockingQueue.push( request );
}

void CustomMessageQueueManager::processQueue()
{
    if ( _customMessageBlockingQueue.empty() )
    {
        return;
    }

    CustomMessageRequest request = _customMessageBlockingQueue.front();
    _customMessageBlockingQueue.pop();

    MessagingResult result = _callbackInvoker.invokeUserCallback( CallbackType::CustomMessageBlockingCb, 
        request.sender, request.rawData.data(), request.rawData.size() );
    
    _messagingService.sender().sendMessage( DistributionMessageType::BlockingMessageResponse, result.rawData.data(), result.rawData.size(), request.sender );
}