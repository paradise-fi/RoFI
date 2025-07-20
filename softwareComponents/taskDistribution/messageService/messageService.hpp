#pragma once
#include <functional>
#include <memory>
#include <map>
#include "distributionMessageType.hpp"
#include "LRElect.hpp"

class MessageService {
    using onMessageCallback = std::function< void( Ip6Addr sender, DistributionMessageType messageType, uint8_t* data, size_t size ) >;

    Ip6Addr _address;
    u16_t _distributionPort;
    MessageDistributor* _messageDistributor;
    std::unique_ptr< udp_pcb > _pcb;
    std::map< unsigned int, onMessageCallback > _callbacks;

    static void recv_message( void* messageService, 
        struct udp_pcb* pcb,
        struct pbuf * p,
        const ip6_addr_t *,
        u16_t )
    {
        if ( !pcb )
        {
            std::cout << "[MessageService] Message Received with null pcb." << std::endl;
            return;
        }

        MessageService* self = static_cast< MessageService* >( messageService );
        if ( self )
        {
            auto packet = rofi::hal::PBuf::own( p );
            self->onMessage( packet.payload(), packet.size() );
        }   
    }

    // Adds header data to the message. Make sure you use headerSize() to move past the header later
    void addHeader( unsigned int methodId, DistributionMessageType messageType, Ip6Addr address, uint8_t* buffer )
    {
        as< unsigned int >( buffer ) = methodId;
        as< DistributionMessageType >( buffer + sizeof( unsigned int ) ) = messageType;
        as< Ip6Addr >( buffer + sizeof( unsigned int ) + sizeof( DistributionMessageType ) ) = address;
    }

public:
    MessageService( Ip6Addr address, u16_t distributionPort, MessageDistributor* messageDistributor, std::unique_ptr< udp_pcb > pcb )
    : _address( address ), _distributionPort( distributionPort), _messageDistributor( messageDistributor )
    {
        LOCK_TCPIP_CORE();
        _pcb = std::move( pcb );
        UNLOCK_TCPIP_CORE();

        if ( _pcb.get() == nullptr )
        {
            std::cout << "[MessageService] PCB null" << std::endl;
        }

        LOCK_TCPIP_CORE();
        auto err = udp_bind( _pcb.get(), IP6_ADDR_ANY, distributionPort );
        if ( err != ERR_OK)
        {
            std::cout << "[MessageService] UDP_BIND: " << lwip_strerr(err) << std::endl;
        }
        udp_recv( _pcb.get(), recv_message, this);
        UNLOCK_TCPIP_CORE();
    }

    bool registerOnMessageCallback( unsigned int methodId, onMessageCallback callback )
    {
        std::cout << "Registering callback for method " << methodId << std::endl;
        auto existingCallback = _callbacks.find( methodId );
        if ( existingCallback != _callbacks.end() )
        {
            return false;
        }

        if ( !_messageDistributor->registerMethod(
            methodId,
            [&](Ip6Addr, uint8_t* data, unsigned int size){ onMessage( data, size ); },
            [](){} ) )
        {
            return false;
        }

        _callbacks.emplace( methodId, callback );
        return true;
    }

    void onMessage( uint8_t* data, unsigned int size )
    {
        unsigned int methodId = as< unsigned int >( data );
        DistributionMessageType messageType = as< DistributionMessageType >( data + sizeof( unsigned int ) );
        Ip6Addr address = as< Ip6Addr >( data + sizeof( unsigned int ) + sizeof( DistributionMessageType ) );

        std::cout << "[MessageService] onMessage method " << methodId << " messageType " << messageType << " address " << address << std::endl;
        auto callback = _callbacks.find( methodId );
        if ( callback == _callbacks.end() )
        {
            std::cout << "[MessageService] Callback for method " << methodId << " not registered." << std::endl;
            return;
        }

        callback->second( address, messageType, data + headerSize(), size - headerSize() );
    }

    void sendMessage( unsigned int methodId, DistributionMessageType type, TaskBase& task, const Ip6Addr& target)
    {
        auto buffer = rofi::hal::PBuf::allocate( task.size() + headerSize() );
        addHeader( methodId, type, _address, buffer.payload() );
        task.copyToBuffer( buffer.payload() + headerSize() );

        auto result = udp_sendto( _pcb.get(), buffer.release(), &target, _distributionPort );

        if ( result != ERR_OK )
        {
            std::cout << "Error while sending message: " << lwip_strerr( result ) << std::endl;
        }
    }

    void sendMessage( unsigned int methodId, DistributionMessageType type, PBuf&& data, const Ip6Addr& target )
    {
        auto buffer = rofi::hal::PBuf::allocate( headerSize() + data.size() );

        addHeader( methodId, type, _address, buffer.payload() );
        std::memcpy(buffer.payload() + headerSize(), data.payload(), data.size() );

        auto result = udp_sendto( _pcb.get(), buffer.release(), &target, _distributionPort );

        if ( result != ERR_OK )
        {
            std::cout << "Error while sending message: " << lwip_strerr( result ) << std::endl;
        }
    }
    
    void sendMessage( unsigned int methodId, DistributionMessageType type, const Ip6Addr& target )
    {
        auto buffer = rofi::hal::PBuf::allocate( headerSize() );
        
        addHeader( methodId, type, _address, buffer.payload() );

        auto result = udp_sendto( _pcb.get(), buffer.release(), &target, _distributionPort );

        if ( result != ERR_OK )
        {
            std::cout << "Error while sending message: " << lwip_strerr( result ) << std::endl;
        }
    }

    void broadcastMessage( DistributionMessageType type, unsigned int methodId )
    {
        auto buffer = rofi::hal::PBuf::allocate( headerSize() );

        addHeader( methodId, type, _address, buffer.payload() );

        _messageDistributor->sendMessage( _address, methodId, buffer.payload(), buffer.size() );
    }

    void broadcastMessage( DistributionMessageType type, PBuf&& data, unsigned int methodId )
    {
        auto buffer = rofi::hal::PBuf::allocate( headerSize() + data.size() );

        addHeader( methodId, type, _address, buffer.payload() );

        std::memcpy(buffer.payload() + headerSize(), data.payload(), data.size() );

        _messageDistributor->sendMessage( _address, methodId, buffer.payload(), buffer.size() );
    }

    void broadcastMessage( DistributionMessageType type, uint8_t* data, size_t size, unsigned int methodId )
    {
        std::vector< uint8_t > buffer;
        buffer.resize( size + headerSize() );
        addHeader( methodId, type, _address, buffer.data() );
        unsigned int method = as< unsigned int >( buffer.data() );
        std::memcpy( buffer.data() + headerSize(), data, size );
        _messageDistributor->sendMessage( _address, methodId, buffer.data(), size + headerSize() );
    }

    void broadcastMessage( DistributionMessageType type, TaskBase& task, unsigned int methodId )
    {
        auto buffer = rofi::hal::PBuf::allocate( task.size() + headerSize() );

        addHeader( methodId, type, _address, buffer.payload() );
        task.copyToBuffer( buffer.payload() + headerSize() );

        _messageDistributor->sendMessage( _address, methodId, buffer.payload(), buffer.size() );
    }

    unsigned int headerSize()
    {
        return sizeof( unsigned int ) + sizeof( DistributionMessageType ) + Ip6Addr::size();
    }
};