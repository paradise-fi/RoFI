#pragma once

#include <system/memory.hpp>
#include <util.hpp>

using Blob = memory::Pool::Block;

static const int CRC_SIZE = 4;
static const int BLOB_HEADER_SIZE = 4;

inline uint16_t blobLen( const Blob& b ) {
    assert( b.get() );
    return viewAs< uint16_t >( b.get() + 2 );
}

inline AsProxy< uint32_t > crcField( const Blob& b ) {
    assert( b.get() );
    return viewAs< uint32_t >( b.get() + BLOB_HEADER_SIZE + blobLen( b ) );
}

inline AsProxy< uint32_t > crcField( Blob& b ) {
    assert( b.get() );
    return viewAs< uint32_t >( b.get() + BLOB_HEADER_SIZE + blobLen( b ) );
}

inline uint8_t *blobBody( Blob& b ) {
    assert( b.get() );
    return b.get() + BLOB_HEADER_SIZE;
}

inline const uint8_t *blobBody( const Blob& b ) {
    assert( b.get() );
    return b.get() + BLOB_HEADER_SIZE;
}
