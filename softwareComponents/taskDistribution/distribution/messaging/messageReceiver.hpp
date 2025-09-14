#pragma once

#include <functional>
#include <atoms/util.hpp>
#include "lwip++.hpp"
#include "distributionMessageType.hpp"

using namespace rofi::net;

/// @brief Low-level handler for messages received via network.
class MessageReceiver
{
    std::function< void( Ip6Addr, DistributionMessageType, uint8_t* data, unsigned int size ) > _onMessageHandler;

    static void recv_message( void* receiver, 
        struct udp_pcb* pcb,
        struct pbuf* p,
        const ip6_addr_t* addr,
        u16_t port )
    {
        MessageReceiver* self = static_cast< MessageReceiver* >( receiver );
        if ( self )
        {
            self->receiveMessage( nullptr, pcb, p, addr, port );
        }   
    }
    
public:
    MessageReceiver(
        u16_t port, 
        udp_pcb* pcb,
        std::function< void( Ip6Addr, DistributionMessageType, uint8_t* data, unsigned int size ) > onMessageHandler )
    : _onMessageHandler( onMessageHandler )
    {
        if ( !pcb )
        {
            std::cout << "PCB Null" << std::endl;
        }

        LOCK_TCPIP_CORE();
        udp_bind( pcb, IP6_ADDR_ANY, port );
        udp_recv( pcb, recv_message, this);
        UNLOCK_TCPIP_CORE();
    }

    void receiveMessage( void*,
        struct udp_pcb*,
        struct pbuf* p,
        const ip6_addr_t*, 
        u16_t )
    {
        if ( !p )
        {
            return;
        }
        
        auto packet = rofi::hal::PBuf::own( p );
        DistributionMessageType type = as< DistributionMessageType >( packet.payload() );
        Ip6Addr sender = as< Ip6Addr >( packet.payload() + sizeof( DistributionMessageType ) );

        // ToDo: Move packet payload past Type and Sender
        _onMessageHandler( sender, type, packet.payload(), packet.size() );
    }
};