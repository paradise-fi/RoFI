#include "messages.hpp"

namespace rofi::updater::messages {

const ChunkDescriptor& _dsc( const Message& message ) {
    return *std::visit(
            []( const auto& m ) { return &m.dsc; },
            message
    );
}

ChunkDescriptor& _dsc( Message& message ) {
    return *std::visit(
            []( auto& m ) { return &m.dsc; },
            message
    );
}

const Proto& _proto( const Message& message ) {
    return *std::visit(
            []( const auto& m ) { return &m.proto; },
            message
    );
}

Proto& _proto( Message& message ) {
    return *std::visit(
            []( auto& m ) { return &m.proto; },
            message
    );
}

std::ostream& operator<<( std::ostream& s, const Proto& p ) {
    s << "Proto: " << std::endl;
    s << "  chunks: " << p.chunks << ", fwSize: " << p.fwSize << ", chunkSize: " << p.chunkSize << ", connectorId: " << p.connectorId;
    return s;
}

std::ostream& operator<< ( std::ostream& s, const ChunkDescriptor& d ) {
    s << "ChunkDescriptor: " << std::endl;
    s << "  fwType: " << static_cast<int>(d.fwType) << ", fwVersion: " << d.fwVersion << ", chunkId: " << d.chunkId;
    return s;
}

std::ostream& operator<<( std::ostream& s, const AnnounceMessage& m ) {
    s << "AnnounceMessage: " << std::endl;
    s << "proto: " << m.proto << std::endl;
    s << "dsc: " << m.dsc << std::endl;
    return s;
}

std::ostream& operator<<( std::ostream& s, const RequestMessage& m ) {
    s << "RequestMessage: " << std::endl;
    s << "proto: " << m.proto << std::endl;
    s << "dsc: " << m.dsc << std::endl;
    return s;
}

std::ostream& operator<<( std::ostream& s, const DataMessage& m ) {
    s << "DataMessage: " << std::endl;
    s << "proto: " << m.proto << std::endl;
    s << "dsc: " << m.dsc << std::endl;
    s << "data.size(): " << m.data.size() << std::endl;
    return s;
}

std::ostream& operator<<( std::ostream& s, const Message& m ) {
    std::visit(
            [&]( const auto& m ) { s << m; },
            m
    );
    return s;
}

}
