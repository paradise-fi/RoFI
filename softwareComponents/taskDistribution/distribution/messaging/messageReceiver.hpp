#pragma once

#include <functional>
#include <atoms/util.hpp>
#include "lwip++.hpp"
#include "lwip/udp.h"
#include "distributionMessageType.hpp"
#include "messageQueueManager.hpp"
#include "networking/protocols/messageDistributor.hpp"
#include "blockingMessageDataService.hpp"

using namespace rofi::net;

/// @brief Low-level handler for messages received via network.
class MessageReceiver
{
    MessageQueueManager& _messageQueueManager;
    BlockingMessageDataService& _blockingMessageDataService;

    // static void recv_message( void* receiver, 
    //     struct udp_pcb* pcb,
    //     struct pbuf* p,
    //     const ip6_addr_t* addr,
    //     u16_t port )
    // {
    //     MessageReceiver* self = static_cast< MessageReceiver* >( receiver );
    //     if ( self )
    //     {
    //         self->receiveMessage( nullptr, pcb, p, addr, port );
    //     }   
    // }
    
public:
    MessageReceiver(
        u16_t port, 
        udp_pcb* pcb,
        MessageQueueManager& messageQueueManager,
        MessageDistributor& messageDistributor,
        int receiveMethodId,
        BlockingMessageDataService& blockingMessageDataService );

    void receiveMessage( void*,
        struct udp_pcb*,
        struct pbuf* p,
        const ip6_addr_t*, 
        u16_t );
};