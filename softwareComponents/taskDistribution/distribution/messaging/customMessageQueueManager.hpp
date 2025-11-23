#pragma once
#include "lwip++.hpp"
#include <vector>
#include <queue>
#include "userCallbackInvoker.hpp"
#include "messagingService.hpp"

struct CustomMessageRequest
{
    const rofi::hal::Ip6Addr sender;
    std::vector< uint8_t > rawData;
};

class CustomMessageQueueManager
{
    std::queue< CustomMessageRequest > _customMessageBlockingQueue;
    UserCallbackInvoker& _callbackInvoker;
    MessagingService& _messagingService;

public:
    CustomMessageQueueManager( UserCallbackInvoker& callbackInvoker, MessagingService& messagingService )
        : _callbackInvoker( callbackInvoker ), _messagingService( messagingService ) 
        {}

    void emplaceRequest( const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size )
    {
        CustomMessageRequest request { sender, std::vector< uint8_t >( size ) };
        if ( size > 0 )
        {
            std::memcpy( request.rawData.data(), data, size );
        }
        _customMessageBlockingQueue.push( request );
    }

    void processQueue()
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
};