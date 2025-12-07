#pragma once
#include "lwip++.hpp"
#include "task.hpp"
#include "networking/protocols/messageDistributor.hpp"
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
    MessageSender(Ip6Addr& address, u16_t port, udp_pcb* pcb, MessageDistributor& messageDistributor);
    
    MessageSendResult sendMessage( DistributionMessageType type, TaskBase& task, const Ip6Addr& target);

    MessageSendResult sendMessage( DistributionMessageType type, PBuf&& data, const Ip6Addr& target );

    MessageSendResult sendMessage( DistributionMessageType type, uint8_t* data, size_t dataSize, const Ip6Addr& target );
    
    MessageSendResult sendMessage( DistributionMessageType type, const Ip6Addr& target );

    void broadcastMessage( DistributionMessageType type, unsigned int methodId );

    void broadcastMessage( DistributionMessageType type, PBuf&& data, unsigned int methodId );

    void broadcastMessage( DistributionMessageType type, uint8_t* data, size_t size, unsigned int methodId );

    void broadcastMessage( DistributionMessageType type, TaskBase& task, unsigned int methodId );

    size_t headerSize();
};