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
        int receiverMethodId )
    : _pcb( std::move( pcb ) ),
      _receiver( distributionPort, _pcb.get(), messageQueueManager, distributor, receiverMethodId, _blockingMessageDataService ),
      _sender( address, distributionPort, _pcb.get(), distributor )
    {}

    MessageSender& sender()
    {
        return _sender;
    }

    MessageReceiver& receiver()
    {
        return _receiver;
    }

    udp_pcb& pcb()
    {
        return *( _pcb.get() );
    }


    MessagingResult sendMessageBlocking( Ip6Addr& receiver, DistributionMessageType messageType, uint8_t* message, size_t messageSize, int timeout = 600 )
    {
        if ( !IsMessageTypeBlocking( messageType ) )
        {
            return MessagingResult( std::string("Non-blocking message type provided. Aborting."), false );
        }

        auto senderResult = _sender.sendMessage( messageType, message, messageSize, receiver );

        if ( !senderResult.success )
        {
            return MessagingResult( senderResult.messsage, false );
        }

        return _blockingMessageDataService.awaitBlockingMessage( timeout );
    }

    MessagingResult sendMessageBlocking( Ip6Addr& receiver, DistributionMessageType messageType, int timeout = 100 )
    {
        if ( !IsMessageTypeBlocking( messageType ) )
        {
            return MessagingResult( std::string("Non-blocking message type provided. Aborting."), false );
        }

        auto senderResult = _sender.sendMessage( messageType, receiver );
        if ( !senderResult.success )
        {
            return MessagingResult( senderResult.messsage, false );
        }

        return _blockingMessageDataService.awaitBlockingMessage( timeout );
    }

    void completeBlockingMessage( uint8_t* data, size_t size )
    {
        _blockingMessageDataService.completeBlockingMessage( data, size );
    }
};