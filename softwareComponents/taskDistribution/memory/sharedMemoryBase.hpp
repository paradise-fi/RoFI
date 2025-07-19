#pragma once
#include <cstdint>
#include <lwip++.hpp>
#include <networking/networkManager.hpp>

class SharedMemoryBase {
    public:
        virtual ~SharedMemoryBase() = default;
        virtual void setLeader( Ip6Addr leader ) = 0;
        virtual void onStorageMessage( Ip6Addr sender, uint8_t* data, size_t size ) = 0;
        virtual bool store( uint8_t* data, int size, int address ) = 0;
        virtual std::vector<uint8_t> read( int address ) = 0;
};