#pragma once

#include <cstdint>
#include <lwip++.hpp>
#include <networking/networkManager.hpp>

/// @brief TODO: Consider making it possible to add custom data to memory items using a struct with a map <string, any>
class SharedMemoryBase {
    public:
        virtual ~SharedMemoryBase() = default;
        virtual void setLeader( Ip6Addr leader ) = 0;
        virtual void onStorageMessage( Ip6Addr sender, uint8_t* data, unsigned int size ) = 0;
        virtual bool store( uint8_t* data, int size, int address ) = 0;
        virtual std::vector< uint8_t > read( int address ) = 0;
        virtual bool remove( int address ) = 0;
        virtual std::vector< uint8_t > readMetadata( int address, std::string key ) = 0;
        virtual bool storeMetadata( int address, std::string key, uint8_t* metadata, int metadataSize ) = 0;
        virtual void clear() = 0;
};