#pragma once

#include <memory>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include "lwip++.hpp"
#include "../messaging/messageReceiver.hpp"
#include "../messaging/messageSender.hpp"
#include "../../tasks/serializable/serializable.hpp"
#include "../messaging/messagingResult.hpp"
#include "../messaging/blockingMessageDataService.hpp"

using namespace rofi::net;

/// @brief Wrapper for pcb ownership.
class MessagingService
{
    std::unique_ptr< udp_pcb > _pcb;

    MessageReceiver _receiver;
    MessageSender _sender;
    BlockingMessageDataService _blockingMessageDataService;

public:
    MessagingService(
        Ip6Addr& address,
        u16_t distributionPort,
        MessageDistributor& distributor,
        std::unique_ptr< udp_pcb > pcb,
        MessageQueueManager& messageQueueManager,
        int receiverMethodId );

    MessageSender& sender();

    MessageReceiver& receiver();

    udp_pcb& pcb();

    MessagingResult sendMessageBlocking( Ip6Addr& receiver, DistributionMessageType messageType, uint8_t* message, size_t messageSize, int timeout = 600 );

    MessagingResult sendMessageBlocking( Ip6Addr& receiver, DistributionMessageType messageType, int timeout = 100 );

    void completeBlockingMessage( uint8_t* data, size_t size );
};