#include "../../distribution/messaging/messageReceiver.hpp"

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

MessageReceiver::MessageReceiver(
    u16_t port, 
    udp_pcb* pcb,
    MessageQueueManager& messageQueueManager,
    MessageDistributor& messageDistributor,
    int receiveMethodId,
    BlockingMessageDataService& blockingMessageDataService )
: _messageQueueManager( messageQueueManager ),
    _blockingMessageDataService( blockingMessageDataService )
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

void MessageReceiver::receiveMessage( void*,
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

    if ( type == DistributionMessageType::BlockingMessageResponse )
    {
        _blockingMessageDataService.completeBlockingMessage( 
            packet.payload() + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ),
            packet.size() - ( sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) ) );
    }
    else
    {
        _messageQueueManager.pushMessage( sender, type,
            packet.payload() + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ),
            packet.size() - ( sizeof( DistributionMessageType) + sizeof( Ip6Addr ) ) );
    }
}