#include "update_protocol.hpp"

#include <cmath>

namespace rofi::updater {
    std::size_t _actualChunkSize( const std::size_t totalSize, const ChunkId chunkId, const std::size_t chunkSize ) {
        return std::min( chunkSize, totalSize - chunkId * chunkSize );
    }

    ChunkId _sizeToChunks( const std::size_t size, const std::size_t chunkSize ) {
        return static_cast< ChunkId >( std::ceil( static_cast< float >( size ) / static_cast< float >( chunkSize ) ) );
    }

    bool _isValidChunkId( const std::size_t totalSize, const ChunkId &chunkId, const std::size_t chunkSize ) {
        return chunkId < _sizeToChunks( totalSize, chunkSize );
    }
}
