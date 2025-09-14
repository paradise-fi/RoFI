#pragma once

#include <memory>
#include "lwip++.hpp"
#include "messaging/messageReceiver.hpp"
#include "messaging/messageSender.hpp"

using namespace rofi::net;

/// @brief Wrapper for pcb ownership.
class MessagingWrapper
{
    std::unique_ptr< udp_pcb > _pcb;

    MessageReceiver _receiver;
    MessageSender _sender;

public:
    MessagingWrapper(
        Ip6Addr& address,
        u16_t distributionPort,
        MessageDistributor* distributor,
        std::function< void( Ip6Addr, DistributionMessageType, uint8_t* data, unsigned int size ) > onMessageHandler,
        std::unique_ptr< udp_pcb > pcb )
    : _pcb( std::move( pcb ) ),
      _receiver( distributionPort, _pcb.get(), onMessageHandler ),
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
};