#pragma once
#include <vector>
#include "lwip++.hpp"
#include "memoryRequestType.hpp"

struct MemoryRequestQueueItem
{
    rofi::hal::Ip6Addr sender;
    int address;
    bool isMetadataOnly;
    MemoryRequestType requestType;
    std::vector< uint8_t > data;
    
    bool isDeleteRequest()
    {
        return requestType == MemoryRequestType::MemoryDelete;
    }

    bool isWriteRequest()
    {
        return requestType == MemoryRequestType::MemoryWrite;
    }
    
    bool isReadRequest()
    {
        return requestType == MemoryRequestType::MemoryRead;
    }

    MemoryRequestQueueItem( rofi::hal::Ip6Addr sender, uint8_t* buffer, size_t bufferSize, 
        int address, bool isMetadataOnly, MemoryRequestType requestType )
    : sender( sender ), address( address ), isMetadataOnly( isMetadataOnly ), requestType( requestType )
    {
        if ( bufferSize > 0 )
        {
            data.resize( bufferSize );
            std::memcpy( data.data(), buffer, bufferSize );
        }
    } 
};