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
    BlockingTaskRelease
};

class MessageSender {
    Ip6Addr& _address;
    u16_t _distribution_port;
    MessageDistributor* _messageDistributor;
    udp_pcb* _pcb;

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
        std::cout << "Copying task to buffer of size " << buffer.size() - ( sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) ) << std::endl;
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

    void broadcastMessage( DistributionMessageType type, unsigned int methodId )
    {
        auto buffer = rofi::hal::PBuf::allocate( static_cast< std::size_t >( sizeof( DistributionMessageType ) 
                                               + static_cast< std::size_t >( sizeof( Ip6Addr ) ) ) );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;

        _messageDistributor->sendMessage( _address, methodId, buffer.payload(), buffer.size() );
    }

    void broadcastMessage( DistributionMessageType type, PBuf&& data, unsigned int methodId )
    {
        auto buffer = rofi::hal::PBuf::allocate( sizeof( DistributionMessageType )
                                                + Ip6Addr::size()
                                                + data.size() );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;
        std::memcpy(buffer.payload() + sizeof( DistributionMessageType ) + Ip6Addr::size(), data.payload(), data.size() );

        _messageDistributor->sendMessage( _address, methodId, buffer.payload(), buffer.size() );
    }

    void broadcastMessage( DistributionMessageType type, uint8_t* data, size_t size, unsigned int methodId )
    {
        std::vector< uint8_t > buffer;
        buffer.resize( size + sizeof( DistributionMessageType ) + Ip6Addr::size() );
        as< DistributionMessageType >( buffer.data() ) = type;
        as< Ip6Addr >( buffer.data() + sizeof( DistributionMessageType ) ) = _address;
        std::memcpy( buffer.data() + sizeof( DistributionMessageType ) + Ip6Addr::size(), data, size );

        _messageDistributor->sendMessage( _address, methodId, buffer.data(), buffer.size() );
    }

    void broadcastMessage( DistributionMessageType type, TaskBase& task, unsigned int methodId )
    {
        auto buffer = rofi::hal::PBuf::allocate( task.size() 
                                                 + sizeof( DistributionMessageType ) 
                                                 + sizeof( Ip6Addr ) );
        as< DistributionMessageType >( buffer.payload() ) = type;
        as< Ip6Addr >( buffer.payload() + sizeof( DistributionMessageType ) ) = _address;
        task.copyToBuffer( buffer.payload() + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) );

        _messageDistributor->sendMessage( _address, methodId, buffer.payload(), buffer.size() );
    }

    unsigned long int headerSize()
    {
        return sizeof( DistributionMessageType ) + Ip6Addr::size();
    }
};