#pragma once

#include <any>
#include <cstdint>
#include <lwip++.hpp>
#include <networking/networkManager.hpp>

#include "memoryReadResult.hpp"
#include "memoryWriteResult.hpp"

class DistributedMemoryBase {
    public:
        virtual ~DistributedMemoryBase() = default;
        virtual void clear() = 0;
        
        virtual MemoryWriteResult removeData( int address, bool isLeader ) = 0;
        
        virtual MemoryWriteResult writeData( uint8_t* data, size_t size, int address, bool isLeader ) = 0;
        
        virtual MemoryWriteResult writeMetadata( int address, const std::string& key, uint8_t* metadata, size_t metadataSize, bool isLeader ) = 0;
        
        virtual MemoryWriteResult removeMetadata( int address, const std::string& key, bool isLeader ) = 0;

        virtual MemoryReadResult readData( int address ) const = 0;
        virtual MemoryReadResult readMetadata( int address, const std::string& key) const = 0;
        
        /// @brief Take data and add important details for your implementation. This will be sent to the other nodes.
        /// @param data The raw data.
        /// @param dataSize The size of the raw data.
        /// @param address The address this data is to be stored in - you do not need serialize the address. Can be used to fetch additional data about an alreadty existing entry.
        /// @param isMetadataOnly A flag that denotes whether the data being serialized is just metadata. You can use this for different formatting.
        /// @return A buffer with added data required for your own implementation.
        virtual std::vector< uint8_t > serializeDataToMemoryFormat( uint8_t* data, size_t dataSize, int address, bool isMetadataOnly = false ) const = 0;
};