#pragma once

#include <functional>
#include <atoms/util.hpp>
#include "lwip++.hpp"
#include "lwip/udp.h"
#include "../distribution/distributionMessageType.hpp"
#include "messageQueueManager.hpp"
#include "networking/protocols/messageDistributor.hpp"
#include "blockingMessageDataService.hpp"

using namespace rofi::net;

/// @brief Low-level handler for messages received via network.
class MessageReceiver
{
    MessageQueueManager& _messageQueueManager;
    BlockingMessageDataService& _blockingMessageDataService;
    MessageDistributor& _messageDistributor;
    int _receiveMethodId;

public:
    MessageReceiver(
        u16_t port, 
        udp_pcb* pcb,
        MessageQueueManager& messageQueueManager,
        MessageDistributor& messageDistributor,
        int receiveMethodId,
        BlockingMessageDataService& blockingMessageDataService );

    ~MessageReceiver();

    void receiveMessage( void*,
        struct udp_pcb*,
        struct pbuf* p,
        const ip6_addr_t*, 
        u16_t );
};