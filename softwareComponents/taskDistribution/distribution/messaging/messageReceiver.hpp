#pragma once

#include <functional>
#include <atoms/util.hpp>
#include "lwip++.hpp"
#include "distributionMessageType.hpp"
#include "messageQueueManager.hpp"
#include "networking/protocols/messageDistributor.hpp"

using namespace rofi::net;

/// @brief Low-level handler for messages received via network.
class MessageReceiver
{
    MessageQueueManager& _messageQueueManager;

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
        MessageQueueManager& messageQueueManager,
        MessageDistributor& messageDistributor,
        int receiveMethodId )
    : _messageQueueManager( messageQueueManager )
    {
        if ( !pcb )
        {
            std::cout << "PCB Null" << std::endl;
        }

        LOCK_TCPIP_CORE();
        udp_bind( pcb, IP6_ADDR_ANY, port );
        udp_recv( pcb, recv_message, this);
        UNLOCK_TCPIP_CORE();

        messageDistributor.registerMethod( receiveMethodId, [&]( rofi::net::Ip6Addr, uint8_t* data, unsigned int size ){
            DistributionMessageType type = as< DistributionMessageType >( data );
            Ip6Addr sender = as< Ip6Addr >( data + sizeof( DistributionMessageType ) );
            _messageQueueManager.pushMessage( sender, type, 
                data + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ), 
                size - ( sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) ) );
        }, [](){});
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
        _messageQueueManager.pushMessage( sender, type,
            packet.payload() + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ),
            packet.size() - ( sizeof( DistributionMessageType) + sizeof( Ip6Addr ) ) );
    }
};