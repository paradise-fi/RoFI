#pragma once

#include <chrono>
#include <functional>
#include <vector>

#include "delayed_data_handler.hpp"
#include "inner_state.hpp"

#include <rofiResp.pb.h>


namespace rofi::simplesim
{

class PacketFilter {
public:
    struct SendPacketData {
        Connector sender;
        Connector receiver;
        rofi::messages::Packet packet;

        rofi::messages::RofiResp getRofiResp() const
        {
            auto rofiResp = this->receiver.getRofiResp( rofi::messages::ConnectorCmd::PACKET );
            *rofiResp.mutable_connectorresp()->mutable_packet() = this->packet;
            return rofiResp;
        }
    };
    struct DelayedPacket {
        rofi::messages::Packet packet;
        /// packet if lost if `delay < 0`
        std::chrono::milliseconds delay = std::chrono::milliseconds::zero();
    };

    using FilterFunction = std::function< DelayedPacket( SendPacketData ) >;

    PacketFilter( FilterFunction filter = {} ) : _filter( std::move( filter ) ) {}

    std::vector< SendPacketData > advanceTime( std::chrono::milliseconds duration )
    {
        return _handler.advanceTime( duration );
    }

    void registerPacket( SendPacketData packetData )
    {
        auto delay = std::chrono::milliseconds::zero();

        if ( _filter ) {
            auto delayedPacket = _filter( { .sender = packetData.sender,
                                            .receiver = packetData.receiver,
                                            .packet = std::move( packetData.packet ) } );
            delay = delayedPacket.delay;
            packetData.packet = std::move( delayedPacket.packet );
        }

        if ( delay < std::chrono::milliseconds::zero() ) {
            return; // Throw away packet
        }
        _handler.registerDelayedData( std::move( packetData ), delay );
    }

private:
    FilterFunction _filter;
    DelayedDataHandler< SendPacketData > _handler;
};

} // namespace rofi::simplesim
