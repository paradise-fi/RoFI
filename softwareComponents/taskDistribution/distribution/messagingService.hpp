#pragma once

#include <memory>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include "lwip++.hpp"
#include "messaging/messageReceiver.hpp"
#include "messaging/messageSender.hpp"
#include "../tasks/serializable/serializable.hpp"
#include "messaging/messagingResult.hpp"

using namespace rofi::net;

/// @brief Wrapper for pcb ownership.
class MessagingService
{
    std::unique_ptr< udp_pcb > _pcb;

    MessageReceiver _receiver;
    MessageSender _sender;

    std::condition_variable _blockingMessageCV;
    std::mutex _lock;
    std::vector< uint8_t > _blockingDataBuffer;

public:
    MessagingService(
        Ip6Addr& address,
        u16_t distributionPort,
        MessageDistributor* distributor,
        std::function< void( Ip6Addr sender, DistributionMessageType messageType, uint8_t* data, unsigned int size ) > onMessageHandler,
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

    
    void completeBlockingMessage( uint8_t* message, size_t messageSize )
    {
        {
            std::lock_guard lk( _lock );
            _blockingDataBuffer.clear();
            _blockingDataBuffer.resize( messageSize );
            std::memcpy(_blockingDataBuffer.data(), message, messageSize );
        }
        _blockingMessageCV.notify_all();
    }


    MessagingResult sendMessageBlocking( Ip6Addr& receiver, DistributionMessageType messageType, uint8_t* message, size_t messageSize, int timeout = 600 )
    {
        MessagingResult result;
        if ( !IsMessageTypeBlocking( messageType ) )
        {
            result.success = false;
            result.statusMessage = std::string("Non-blocking message type provided. Aborting.");
            return result;
        }

        _sender.sendMessage( messageType, message, messageSize, receiver );

        std::unique_lock lk ( _lock );
        auto cvResult = _blockingMessageCV.wait_for( lk, std::chrono::milliseconds( timeout ) );
        if ( cvResult == std::cv_status::timeout )
        {
            result.statusMessage = std::string("Message sending timed out.");
            result.success = false;
        }
        else
        {
            result.success = true;
            if ( !_blockingDataBuffer.empty() )
            {
                result.rawData.resize( _blockingDataBuffer.size() );
                std::memcpy( result.rawData.data(), _blockingDataBuffer.data(), _blockingDataBuffer.size() );
                _blockingDataBuffer.clear();
            }
        }
        
        return result;
    }

    MessagingResult sendMessageBlocking( Ip6Addr& receiver, DistributionMessageType messageType, int timeout = 100 )
    {
        MessagingResult result;
        result.success = false;

        if ( !IsMessageTypeBlocking( messageType ) )
        {
            result.statusMessage = std::string("Non-blocking message type provided. Aborting.");
            return result;
        }

        _sender.sendMessage( messageType, receiver );

        std::unique_lock lk ( _lock );
        auto cvResult = _blockingMessageCV.wait_for( lk, std::chrono::microseconds( timeout ) );


        if ( cvResult == std::cv_status::timeout )
        {
            result.statusMessage = std::string("Message sending timed out.");
        }
        else
        {
            result.success = true;
            if ( !_blockingDataBuffer.empty() )
            {
                result.rawData.resize( _blockingDataBuffer.size() );
                std::memcpy( result.rawData.data(), _blockingDataBuffer.data(), _blockingDataBuffer.size() );
                _blockingDataBuffer.clear();
            }
        }
        
        return result;
    }
};