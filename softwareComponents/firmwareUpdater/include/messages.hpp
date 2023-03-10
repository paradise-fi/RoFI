#pragma once

#include <cstdint>  // fixed size datatypes
#include <ostream>  // std::ostream
#include <utility>  // std::move
#include <variant>  // variant
#include <vector>  // std::vector


namespace rofi::updater::messages {
    using FWType = uint8_t;
    using FWVersion = uint16_t;
    using ChunkId = uint16_t;
    using Version = uint16_t;
    using ChunkSize = uint16_t;  // todo: check if datarange is sufficient
    using Chunks = uint16_t;  // todo: check whether datarange is sufficient
    using FWSize = uint32_t;
    using ConnectorId = int;


    constexpr ChunkSize CHUNK_SIZE = 1024;

    enum MessageType : uint8_t {
        ANNOUNCE = 0,
        REQUEST = 1,
        DATA = 2
    };


    struct ChunkDescriptor {
        FWType fwType;
        FWVersion fwVersion;
        ChunkId chunkId;

        bool operator==( const ChunkDescriptor& other ) const {
            return fwType == other.fwType && fwVersion == other.fwVersion && chunkId == other.chunkId;
        }
    };


    struct Proto {
        ConnectorId connectorId;  // id of connector that received the message; ignored during (de)serialization
        ChunkSize chunkSize;  // size in bytes of a typical chunk
        Chunks chunks;  // total number of chunks of firmware
        FWSize fwSize;  // total size in bytes of a firmware
    };


    struct AnnounceMessage {
        Proto proto;
        ChunkDescriptor dsc;

        AnnounceMessage() = default;
        AnnounceMessage( const AnnounceMessage& msg ) = default;
        AnnounceMessage( const Proto& proto, const ChunkDescriptor& dsc ) : proto( proto ), dsc( dsc ) {}
    };

    struct RequestMessage {
        Proto proto;
        ChunkDescriptor dsc;

        RequestMessage() = default;
        RequestMessage( const RequestMessage& msg ) = default;
        RequestMessage( const Proto& proto, const ChunkDescriptor& dsc ) : proto( proto ), dsc( dsc ) {}
    };


    struct DataMessage {
        Proto proto;
        ChunkDescriptor dsc;
        std::vector< unsigned char > data;  // todo: vector or array + occupancy??

        DataMessage() : proto(), dsc(), data() {}
        DataMessage( const DataMessage& msg ) = default;
        DataMessage( const Proto& proto, const ChunkDescriptor& dsc, std::vector< unsigned char > data )
                : proto( proto ), dsc( dsc ), data( std::move( data ) ) {}
    };

    using Message = std::variant< AnnounceMessage, RequestMessage, DataMessage >;

    const ChunkDescriptor& _dsc( const Message& message );
    ChunkDescriptor& _dsc( Message& message );
    const Proto& _proto( const Message& message );
    Proto& _proto( Message& message );

    std::ostream& operator<<( std::ostream& s, const Proto& p );

    std::ostream& operator<< ( std::ostream& s, const ChunkDescriptor& d );

    std::ostream& operator<<( std::ostream& s, const AnnounceMessage& m );
    std::ostream& operator<<( std::ostream& s, const RequestMessage& m );
    std::ostream& operator<<( std::ostream& s, const DataMessage& m );
    std::ostream& operator<<( std::ostream& s, const Message& m );
}

namespace std {
    template <>
    struct hash< rofi::updater::messages::ChunkDescriptor > {
        auto operator()( const rofi::updater::messages::ChunkDescriptor& dsc ) const -> size_t {
            using std::size_t;
            using std::hash;
            return ( ( hash< size_t >()( dsc.fwType )
                    ^ ( hash< size_t >()( dsc.fwVersion ) << 1 ) ) >> 1 )
                    ^ ( hash< size_t >()( dsc.chunkId ) << 1 );
        }
    };
}  // namespace std
