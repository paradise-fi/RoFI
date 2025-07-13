#pragma once
#include "lwip++.hpp"
#include "task.hpp"
#include "LRElect.hpp"

enum DistributionMessageType {
    TaskRequest,
    TaskAssignment,
    TaskResult,
    TaskFailed,
    MalformedMessage,
    FollowerBusy,
    DataStorageRequest,
    DataStorageSuccess,
};

class MessageSender {
    udp_pcb* _pcb;
    Ip6Addr& _address;
    MessageDistributor* _messageDistributor;
    u16_t _distribution_port;
public:
    MessageSender(Ip6Addr& address, u16_t port, udp_pcb* pcb, MessageDistributor* messageDistributor)
    : _address( address ), _distribution_port( port ), _messageDistributor( messageDistributor ) {
        _pcb = pcb;
    }

    void sendMessage( DistributionMessageType type, TaskBase& task, const Ip6Addr& target)
    {
        auto buffer = rofi::hal::PBuf::allocate( task.size() 
                                                 + sizeof( DistributionMessageType ) 
                                                 + sizeof( Ip6Addr ) );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;
        task.copyToBuffer( buffer.payload() + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) );

        auto result = udp_sendto( _pcb, buffer.release(), &target, _distribution_port );

        if ( result != ERR_OK )
        {
            std::cout << "Error while sending message: " << lwip_strerr( result ) << std::endl;
        }
    }

    void sendMessage( DistributionMessageType type, PBuf&& data, const Ip6Addr& target )
    {
        auto buffer = rofi::hal::PBuf::allocate( sizeof( DistributionMessageType )
                                                + Ip6Addr::size()
                                                + data.size() );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;
        std::memcpy(buffer.payload() + sizeof( DistributionMessageType ) + Ip6Addr::size(), data.payload(), data.size() );
        auto result = udp_sendto( _pcb, buffer.release(), &target, _distribution_port );

        if ( result != ERR_OK )
        {
            std::cout << "Error while sending message: " << lwip_strerr( result ) << std::endl;
        }
    }
    
    void sendMessage( DistributionMessageType type, const Ip6Addr& target )
    {
        auto buffer = rofi::hal::PBuf::allocate( static_cast< std::size_t >( sizeof( DistributionMessageType ) 
                                               + static_cast< std::size_t >( sizeof( Ip6Addr ) ) ) );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;

        auto result = udp_sendto( _pcb, buffer.release(), &target, _distribution_port );

        if ( result != ERR_OK )
        {
            std::cout << "Error while sending message: " << lwip_strerr( result ) << std::endl;
        }
    }

    // void broadcastMessage()
};