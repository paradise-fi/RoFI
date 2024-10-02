#include "serialization.hpp"

namespace rofi::updater::messages {

template<>
void write< decltype( DataMessage::data ) > ( PBuf::ChunkIt& currentChunkIt, const PBuf::ChunkIt& endChunkIt, void*& buffer, const decltype( DataMessage::data )& value ) {
    auto* b = static_cast< decltype( DataMessage::data )::pointer >( buffer );
    for ( auto& i : value ) {
        possiblyMoveChunkIt( currentChunkIt, endChunkIt, buffer, sizeof( decltype( DataMessage::data )::value_type ) );
        *b = i;
        b += 1;
    }
    buffer = b;
}

template<>
void* read< decltype( DataMessage::data ) > ( void* buffer, void* bufferEnd, decltype( DataMessage::data )& value ) {
    auto* b = static_cast< decltype( DataMessage::data )::pointer >( buffer );
    for ( auto& i : value ) {
        i = *b;
        b += 1;
    }

    return b;
}

template<>
void write< Proto > ( PBuf::ChunkIt& currentChunkIt, const PBuf::ChunkIt& endChunkIt, void*& buffer, const Proto& value ) {
    write( currentChunkIt, endChunkIt, buffer, value.chunkSize );
    write( currentChunkIt, endChunkIt, buffer, value.chunks );
    write( currentChunkIt, endChunkIt, buffer, value.fwSize );
}

template<>
void* read< Proto > ( void* buffer, void* bufferEnd, Proto& value ) {
    buffer = read( buffer, bufferEnd, value.chunkSize );
    buffer = read( buffer, bufferEnd, value.chunks );
    buffer = read( buffer, bufferEnd, value.fwSize );

    return buffer;
}

template<>
void write< ChunkDescriptor > ( PBuf::ChunkIt& currentChunkIt, const PBuf::ChunkIt& endChunkIt, void*& buffer, const ChunkDescriptor& value ) {
    write( currentChunkIt, endChunkIt, buffer, value.fwType );
    write( currentChunkIt, endChunkIt, buffer, value.fwVersion );
    write( currentChunkIt, endChunkIt, buffer, value.chunkId );
}

template<>
void* read< ChunkDescriptor > ( void* buffer, void* bufferEnd, ChunkDescriptor& value ) {
    buffer = read( buffer, bufferEnd, value.fwType );
    buffer = read( buffer, bufferEnd, value.fwVersion );
    buffer = read( buffer, bufferEnd, value.chunkId );

    return buffer;
}

PBuf serialize( const Message& message ) {
    PBuf b = PBuf::empty();
    std::visit(
        [ & ]( const auto& message ) { PBuf a = serialize( message ); b = a; },
        message
    );

    return b;
}

PBuf serialize( const AnnounceMessage& msg ) {
    constexpr std::size_t payloadSize = sizeof( MessageType ) + sizeof( msg );
    static_assert( payloadSize < UINT16_MAX, "Max PBuf size exceeded" );

    PBuf buffer = PBuf::allocate( payloadSize );

    auto currentChunkIt = buffer.chunksBegin();
    const auto endChunkIt = buffer.chunksEnd();
    void* ptr = currentChunkIt->mem();

    write( currentChunkIt, endChunkIt, ptr, MessageType::ANNOUNCE );
    write( currentChunkIt, endChunkIt, ptr, msg.proto );
    write( currentChunkIt, endChunkIt, ptr, msg.dsc );

    return buffer;
}

PBuf serialize( const RequestMessage& msg ) {
    constexpr std::size_t payloadSize = sizeof( MessageType ) + sizeof( msg );
    static_assert( payloadSize < UINT16_MAX, "Max PBuf size exceeded" );

    PBuf buffer = PBuf::allocate( payloadSize );

    auto currentChunkIt = buffer.chunksBegin();
    const auto endChunkIt = buffer.chunksEnd();
    void* ptr = currentChunkIt->mem();

    write( currentChunkIt, endChunkIt, ptr, MessageType::REQUEST );
    write( currentChunkIt, endChunkIt, ptr, msg.proto );
    write( currentChunkIt, endChunkIt, ptr, msg.dsc );

    return buffer;
}

void possiblyMoveChunkIt( PBuf::ChunkIt& currentChunkIt, const PBuf::ChunkIt& endChunkIt, void*& ptr, std::size_t need ) {
    auto* mem = currentChunkIt->mem();
    assert( ptr >= mem );
    assert( ptr <= mem + currentChunkIt->size() );

    if ( ptr == mem + currentChunkIt->size() ) {
        currentChunkIt++;
        assert( currentChunkIt != endChunkIt );
        ptr = currentChunkIt->mem();
    }

    assert( static_cast< uint8_t* >( ptr ) + need <= currentChunkIt->mem() + currentChunkIt->size() );
}


PBuf serialize( const DataMessage& msg ) {
    const std::size_t payloadSize = sizeof( MessageType ) + sizeof( msg.dsc ) + sizeof( msg.proto )
            + sizeof( ChunkSize ) + msg.data.size() * sizeof( std::decay_t< decltype( *msg.data.begin() ) > );
    assert( payloadSize < UINT16_MAX );

    PBuf buffer = PBuf::allocate( static_cast< int >( payloadSize ) );

    auto currentChunkIt = buffer.chunksBegin();
    const auto endChunkIt = buffer.chunksEnd();
    void* ptr = currentChunkIt->mem();

    write( currentChunkIt, endChunkIt, ptr, MessageType::DATA );
    write( currentChunkIt, endChunkIt, ptr, msg.proto );
    write( currentChunkIt, endChunkIt, ptr, msg.dsc );
    write( currentChunkIt, endChunkIt, ptr, static_cast< ChunkSize >( msg.data.size() ) );
    write( currentChunkIt, endChunkIt, ptr, msg.data );

    return buffer;
}

Message deserialize( PBuf& buffer ) {  // todo: we can't use const reference because chunksBegin is not const available
    auto it = buffer.chunksBegin();
    void* ptr = it->mem();
    assert( buffer.simple() );
    void* end = static_cast< uint8_t* >( ptr ) + it->size();

    MessageType messageType;
    Proto proto{};
    ChunkDescriptor dsc{};

    ptr = read( ptr, end, messageType );
    ptr = read( ptr, end, proto );
    ptr = read( ptr, end, dsc );

    Message m;

    switch ( messageType ) {
        case MessageType::ANNOUNCE: {
            m.emplace< 0 >( proto, dsc );
            break;
        }
        case MessageType::REQUEST: {
            m.emplace< 1 >( proto, dsc );
            break;
        }
        case MessageType::DATA: {
            std::vector< unsigned char > messageData;
            ChunkSize dataSize;

            m.emplace< 2 >(proto, dsc, messageData );

            auto& data = std::get< DataMessage >( m );

            ptr = read(ptr, end, dataSize );
            messageData.resize(dataSize );
            ptr = read(ptr, end, messageData );
            data.data = std::move( messageData );

            ptr = ptr;

            break;
        }
        default:
            throw std::runtime_error( "Unknown message messageType" );
    }

    return m;
}
}
