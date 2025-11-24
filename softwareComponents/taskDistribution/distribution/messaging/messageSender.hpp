#pragma once
#include "lwip++.hpp"
#include "task.hpp"
#include "LRElect.hpp"
#include "distributionMessageType.hpp"
#include "../../tasks/serializable/serializable.hpp"

struct MessageSendResult
{
    bool success;
    std::string messsage;
};

/// @brief Low level handler for sending messages via network.
class MessageSender {
    Ip6Addr& _address;
    u16_t _distributionPort;
    MessageDistributor& _messageDistributor;
    udp_pcb* _pcb;

public:
    MessageSender(Ip6Addr& address, u16_t port, udp_pcb* pcb, MessageDistributor& messageDistributor)
    : _address( address ), _distributionPort( port ), _messageDistributor( messageDistributor ) {
        if ( !pcb )
        {
            std::cout << "PCB Null" << std::endl;
        }
        _pcb = pcb;
    }
    
    MessageSendResult sendMessage( DistributionMessageType type, TaskBase& task, const Ip6Addr& target)
    {
        auto buffer = rofi::hal::PBuf::allocate( headerSize() + static_cast< int >( task.size() ) );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;
        task.copyToBuffer( buffer.payload() + headerSize() );

        MessageSendResult sendResult;
        sendResult.success = true;
        auto result = udp_sendto( _pcb, buffer.get(), &target, _distributionPort );

        if ( result != ERR_OK )
        {
            sendResult.success = false;
            sendResult.messsage = lwip_strerr( result );
        }

        return sendResult;
    }

    MessageSendResult sendMessage( DistributionMessageType type, PBuf&& data, const Ip6Addr& target )
    {
        auto buffer = rofi::hal::PBuf::allocate( headerSize() + data.size() );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;

        if ( data.size() > 0 )
        {
            std::memcpy(buffer.payload() + headerSize(), data.payload(), data.size() );
        }

        MessageSendResult sendResult;
        sendResult.success = true;

        auto result = udp_sendto( _pcb, buffer.get(), &target, _distributionPort );

        if ( result != ERR_OK )
        {
            sendResult.success = false;
            sendResult.messsage = lwip_strerr( result );
        }

        return sendResult;
    }

    MessageSendResult sendMessage( DistributionMessageType type, uint8_t* data, size_t dataSize, const Ip6Addr& target )
    {
        auto buffer = rofi::hal::PBuf::allocate( headerSize() + dataSize );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;

        if ( dataSize > 0 )
        {
            std::memcpy( buffer.payload() + headerSize(), data, dataSize );
        }

        MessageSendResult sendResult;
        sendResult.success = true;

        auto result = udp_sendto( _pcb, buffer.get(), &target, _distributionPort );

        if ( result != ERR_OK )
        {
            sendResult.success = false;
            sendResult.messsage = lwip_strerr( result );
        }

        return sendResult;
    }
    
    MessageSendResult sendMessage( DistributionMessageType type, const Ip6Addr& target )
    {
        auto buffer = rofi::hal::PBuf::allocate( headerSize() );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;

        MessageSendResult sendResult;
        sendResult.success = true;
        auto result = udp_sendto( _pcb, buffer.get(), &target, _distributionPort );

        if ( result != ERR_OK )
        {
            sendResult.success = false;
            sendResult.messsage = lwip_strerr( result );
        }

        return sendResult;
    }

    void broadcastMessage( DistributionMessageType type, unsigned int methodId )
    {
        auto buffer = rofi::hal::PBuf::allocate( headerSize() );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;

        _messageDistributor.sendMessage( _address, methodId, buffer.payload(), buffer.size() );
    }

    void broadcastMessage( DistributionMessageType type, PBuf&& data, unsigned int methodId )
    {
        auto buffer = rofi::hal::PBuf::allocate( headerSize() + data.size() );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;

        if ( data.size() > 0 )
        {
            std::memcpy(buffer.payload() + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ), data.payload(), data.size() );
        }

        _messageDistributor.sendMessage( _address, methodId, buffer.payload(), buffer.size() );
    }

    void broadcastMessage( DistributionMessageType type, uint8_t* data, size_t size, unsigned int methodId )
    {
        std::vector< uint8_t > buffer;
        buffer.resize( size + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) );
        as< DistributionMessageType >( buffer.data() ) = type;
        as< Ip6Addr >( buffer.data() + sizeof( DistributionMessageType ) ) = _address;

        if ( size > 0 )
        {
            std::memcpy( buffer.data() + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ), data, size );
        }

        _messageDistributor.sendMessage( _address, methodId, buffer.data(), buffer.size() );
    }

    void broadcastMessage( DistributionMessageType type, TaskBase& task, unsigned int methodId )
    {
        auto buffer = rofi::hal::PBuf::allocate( static_cast< int >( task.size() )
                                                 + headerSize() );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;
        task.copyToBuffer( buffer.payload() + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) );

        _messageDistributor.sendMessage( _address, methodId, buffer.payload(), buffer.size() );
    }

    unsigned int headerSize()
    {
        return static_cast< int >( sizeof( DistributionMessageType ) ) + sizeof( Ip6Addr );
    }
};