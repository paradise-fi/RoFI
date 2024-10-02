#pragma once

#include "messages.hpp"
#include "networking.hpp"  // PBuf

#include <cstddef>  // std::size_t
#include <cstdint>  // UINT16_MAX
#include <span>  // std::span
#include <stdexcept>  // std::out_of_range; std::runtime_error

namespace rofi::updater::messages {
    using rofi::hal::PBuf;
    using namespace rofi::updater;

    void possiblyMoveChunkIt( PBuf::ChunkIt& currentChunkIt, const PBuf::ChunkIt& endChunkIt, void*& ptr, std::size_t need );

    template< typename T >
    void write( PBuf::ChunkIt& currentChunkIt, const PBuf::ChunkIt& endChunkIt, void*& buffer, const T& value ) {
        possiblyMoveChunkIt( currentChunkIt, endChunkIt, buffer, sizeof( T ) );
        auto* b = static_cast< T* >( buffer );
        *b = value;
        buffer = b + 1;
    }

    template<>
    void write< decltype( DataMessage::data ) > ( PBuf::ChunkIt& currentChunkIt, const PBuf::ChunkIt& endChunkIt, void*& buffer, const decltype( DataMessage::data )& value );

    template<>
    void write< Proto > ( PBuf::ChunkIt& currentChunkIt, const PBuf::ChunkIt& endChunkIt, void*& buffer, const Proto& value );

    template<>
    void write< ChunkDescriptor > ( PBuf::ChunkIt& currentChunkIt, const PBuf::ChunkIt& endChunkIt, void*& buffer, const ChunkDescriptor& value );

    template< typename T >
    void* read( void* buffer, void* bufferEnd, T& value ) {
        auto* b = static_cast< T* >( buffer );

        if ( static_cast< void* >( b + 1 ) > bufferEnd ) {
            throw std::out_of_range( "Exhausted buffer" );
        }

        value = *b;

        return b + 1;
    }

    // for the sake of simplicity let's assume that the whole packet spans only one PBuf
    template<>
    void* read<decltype( DataMessage::data ) > ( void* buffer, void* bufferEnd, decltype( DataMessage::data )& value );

    template<>
    void* read< Proto > ( void* buffer, void* bufferEnd, Proto& value );

    template<>
    void* read< ChunkDescriptor > ( void* buffer, void* bufferEnd, ChunkDescriptor& value );

    PBuf serialize( const Message& message );

    PBuf serialize( const AnnounceMessage& msg );
    PBuf serialize( const RequestMessage& msg );
    PBuf serialize( const DataMessage& msg );

    Message deserialize( PBuf& buffer );
}
