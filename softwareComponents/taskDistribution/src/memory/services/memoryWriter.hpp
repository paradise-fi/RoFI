#pragma once

#include "../../../include/distributedMemoryBase.hpp"
#include "memoryMessagingWrapper.hpp"
#include "../memoryRequestQueueItem.hpp"

class MemoryWriter
{
    DistributedMemoryBase* _memory = nullptr;
    MemoryMessagingWrapper& _memoryMessaging;
    rofi::hal::Ip6Addr _currentModuleAddress;
    LoggingService& _loggingService;

    std::queue<MemoryRequestQueueItem> _memoryStorageQueue;

    MemoryWriteResult handleMetadataUpdate( int address, uint8_t* data, size_t dataSize, MemoryRequestType requestType );

    MemoryWriteResult handleDataUpdate( int address, uint8_t* data, size_t dataSize, MemoryRequestType requestType );

public:
    MemoryWriter( MemoryMessagingWrapper& memoryMessaging, rofi::hal::Ip6Addr currentModuleAddress, LoggingService& loggingService );

    void unregisterMemory();

    void registerMemory( DistributedMemoryBase* memory );

    void removeMetadata( int address, const std::string& key );

    bool saveMetadata( int address, const std::string& key, uint8_t* metadata, std::size_t metadataSize );

    /// @brief Remove data at specified address
    /// @param address The address of the data in memory
    void removeData( int address );

    void handleRemoteDataWriteRequest( Ip6Addr& sender, uint8_t* data, size_t dataSize, int address, bool isMetadataOnly, MemoryRequestType requestType );

    /// @brief Save data in memory.
    /// @param data The data to be stored.
    /// @param size The size of the data.
    /// @param address The address to store the data at
    template < SerializableOrTrivial T >
    bool saveData( T&& data, int address )
    {
        if ( _memory == nullptr )
        {
            return false;
        }
        
        // Convert the data to the memory format required by the implementation
        std::vector< uint8_t > dataBuffer;

        if constexpr ( std::is_base_of_v< Serializable, T > )
        {
            std::vector< uint8_t > buffer;
            buffer.resize( data.size() );
            data.serialize( buffer );
            dataBuffer = _memory->serializeDataToMemoryFormat( buffer.data(), buffer.size(), address, false );
        }
        else
        {
            dataBuffer = _memory->serializeDataToMemoryFormat( reinterpret_cast< uint8_t* >( &data ), sizeof( T ), address );
        }

        if ( _memory->storageBehavior() == MemoryStorageBehavior::LeaderFirstStorage )
        {
            if ( _memoryMessaging.isLeaderMemory() )
            {
                auto result = _memory->writeData( dataBuffer.data(), dataBuffer.size(), address, true );

                if ( result.success )
                {
                    _memoryMessaging.propagateMemoryChange( result.propagationType, address, dataBuffer.data(), 
                        dataBuffer.size(), false, result.propagationTarget, DistributionMessageType::DataStorageRequest );
                }

                return result.success;
            }

            _memoryMessaging.propagateMemoryChange( MemoryPropagationType::ONE_TARGET, address, 
                dataBuffer.data(), dataBuffer.size(), false, _memoryMessaging.getLeader(), 
                DistributionMessageType::DataStorageRequest );

            return true;
        }

        auto result = _memory->writeData( dataBuffer.data(), dataBuffer.size(), address, false );
        
        if ( result.success )
        {
            _memoryMessaging.propagateMemoryChange( result.propagationType, address, dataBuffer.data(), 
                dataBuffer.size(), false, result.propagationTarget, DistributionMessageType::DataStorageRequest );
        }

        return result.success;
    }
};