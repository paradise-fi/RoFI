#pragma once
#include "lwip++.hpp"
#include <vector>
#include <queue>
#include "../callbacks/userCallbackInvoker.hpp"
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
    CustomMessageQueueManager( UserCallbackInvoker& callbackInvoker, MessagingService& messagingService );

    void emplaceRequest( const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size );

    void processQueue();
};