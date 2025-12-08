#include "messagingService.hpp"

MessagingService::MessagingService(
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

MessageSender& MessagingService::sender()
{
    return _sender;
}

MessageReceiver& MessagingService::receiver()
{
    return _receiver;
}

udp_pcb& MessagingService::pcb()
{
    return *( _pcb.get() );
}


MessagingResult MessagingService::sendMessageBlocking( Ip6Addr& receiver, 
    DistributionMessageType messageType, uint8_t* message, size_t messageSize, int timeout )
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

MessagingResult MessagingService::sendMessageBlocking( Ip6Addr& receiver, DistributionMessageType messageType, int timeout )
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

void MessagingService::completeBlockingMessage( uint8_t* data, size_t size )
{
    _blockingMessageDataService.completeBlockingMessage( data, size );
}