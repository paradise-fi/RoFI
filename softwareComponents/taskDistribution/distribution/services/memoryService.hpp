#pragma once
#include "../memory/sharedMemoryBase.hpp"
#include <queue>
#include "lwip++.hpp"
#include "atoms/util.hpp"
#include "../messaging/messageSender.hpp"

using namespace rofi::hal;
using namespace rofi::net;

struct MemoryStorageQueueItem
{
    Ip6Addr sender;
    int address;
    bool isMetadataOnly;
    bool isDelete;
    std::vector< uint8_t > data;
    
    MemoryStorageQueueItem(Ip6Addr sender, uint8_t* buffer, size_t bufferSize, int address, bool isMetadataOnly, bool isDelete )
    : sender( sender ), address( address ), isMetadataOnly( isMetadataOnly ), isDelete( isDelete )
    {
        data.resize( bufferSize );
        std::memcpy( data.data(), buffer, bufferSize );
    } 
};

class DistributedMemoryService
{
    const unsigned int METHOD_ID = 2;

    MessageSender& _sender;
    Ip6Addr _leader;
    const Ip6Addr& _currentModuleAddress;
    std::unique_ptr< SharedMemoryBase > _memory;

    std::queue<MemoryStorageQueueItem> _memoryStorageQueue;
    std::optional< std::function< void( int memoryAddress, bool isLeaderMemory, DistributedMemoryService& memoryService ) > > _onMemoryStoredCb;
    unsigned int _queueThroughput;

    bool isLeaderMemory()
    {
        return _leader == _currentModuleAddress;
    }

    void onMemoryStored( int memoryAddress, bool isLeaderMemory )
    {
        if ( _onMemoryStoredCb == std::nullopt )
        {
            return;
        }

        _onMemoryStoredCb.value()( memoryAddress, isLeaderMemory, *this );
    }

    void sendDataBroadcast( int address, const uint8_t* data, size_t size, bool isMetadataOnly, DistributionMessageType messageType )
    {
        // ADDRESS - IS METADATA ONLY - DATA SIZE - DATA
        std::vector< uint8_t > buffer;

        buffer.resize( size + sizeof ( int ) + sizeof( size_t ) + sizeof( bool ) );
        as< int >( buffer.data() ) = address;
        as< bool >( buffer.data() + sizeof( int ) ) = isMetadataOnly;
        as< size_t >( buffer.data() + sizeof( int ) + sizeof( bool ) ) = size;

        if ( size != 0 )
        {
            std::memcpy( buffer.data() + sizeof ( int ) + sizeof( size_t ) + sizeof( bool ), data, size );
        }

        _sender.broadcastMessage( messageType, buffer.data(), buffer.size(), METHOD_ID );
    }

    void sendDataUnicast( int address, const uint8_t* data, size_t size, bool isMetadataOnly, Ip6Addr& target, DistributionMessageType messageType )
    {
        PBuf packet = PBuf::allocate( static_cast< int >( sizeof( int ) + sizeof( bool ) + sizeof( size_t ) + size ) );
        as< int >( packet.payload() ) = address;
        as< bool >( packet.payload() + sizeof( int ) ) = isMetadataOnly;
        as< size_t >( packet.payload() + sizeof( int ) + sizeof( bool ) ) = size;

        if ( size != 0 )
        {
            std::memcpy( packet.payload() + sizeof( int ) + sizeof( bool ) + sizeof( size_t ), data, size );
        }

        _sender.sendMessage( messageType, std::move( packet ), target );
    }

    void propagateMemoryChange( MemoryPropagationType type, int address, uint8_t* data, size_t size,
        bool isMetadataOnly, std::optional< Ip6Addr > target, DistributionMessageType messageType )
    {
        switch ( type )
        {
            case MemoryPropagationType::ONE_TARGET:
                sendDataUnicast( address, data, size, isMetadataOnly, target.value(), messageType );
                break;
            case MemoryPropagationType::SEND_TO_ALL:
                sendDataBroadcast( address, data, size, isMetadataOnly, messageType );
                break;
            default:
                return;
        }
    }

    void propagateMetadataChange( MemoryPropagationType type, int address, std::string key, uint8_t* metadata, 
        size_t metadataSize, std::optional< Ip6Addr > target, DistributionMessageType messageType)
    {
        std::size_t keySize = key.size();

        auto dataBuffer = _memory->serializeDataToMemoryFormat( metadata, metadataSize, address, true );
        dataBuffer.resize( key.size() + dataBuffer.size() + sizeof( size_t ) );

        std::memmove( dataBuffer.data() + key.size(), dataBuffer.data(), dataBuffer.size() - key.size() );
        std::memcpy( dataBuffer.data(), &keySize, sizeof( size_t ) );
        std::memcpy( dataBuffer.data() + sizeof( size_t ), key.data(), key.size() );

        propagateMemoryChange( type, address, dataBuffer.data(), dataBuffer.size(), true, target, messageType );
    }

    MemoryWriteResult handleMetadataUpdate( MemoryStorageQueueItem& memoryItem )
    {
        MemoryWriteResult result;

        size_t keySize = as< size_t >( memoryItem.data.data() );
        std::string key;
        key.resize( keySize );
        std::memcpy( key.data(), memoryItem.data.data() + sizeof( size_t ), keySize );

        size_t keyDataSize = sizeof( size_t ) + keySize;

        if ( memoryItem.isDelete )
        {
            return _memory->removeMetadata( memoryItem.address, key, isLeaderMemory() );
        }

        return _memory->writeMetadata( memoryItem.address, key, 
            memoryItem.data.data() + keyDataSize,
            memoryItem.data.size() - keyDataSize,
            isLeaderMemory() );
    }

    MemoryWriteResult handleDataUpdate( MemoryStorageQueueItem& memoryItem )
    {
        if ( memoryItem.isDelete )
        {
            return _memory->removeData( memoryItem.address, isLeaderMemory() );
        }
        
        return _memory->writeData( memoryItem.data.data(), memoryItem.data.size(), memoryItem.address, isLeaderMemory() );
    }

public:
    DistributedMemoryService( MessageDistributor* distributor, MessageSender& sender, Ip6Addr& currentModuleAddress )
    : _sender( sender ), _currentModuleAddress( currentModuleAddress )
    {
        distributor->registerMethod( METHOD_ID, 
            [ this ] ( Ip6Addr sender, uint8_t* data, unsigned int size ) 
            {
                auto messageType = as< DistributionMessageType >( data );
                onStorageMessage( sender, data + _sender.headerSize(), size, messageType == DistributionMessageType::DataRemovalRequest ); 
            },
            [] () { return; } );
    }
    
    void registerOnMemoryStored( std::function< void( int memoryAddress, bool isLeaderMemory, DistributedMemoryService& memoryService ) > onMemoryStoredCb )
    {
        _onMemoryStoredCb = onMemoryStoredCb;
    }

    bool isMemoryRegistered()
    {
        return _memory != nullptr;
    }

    /// @brief Removes / deregisters the shared memory implementation from DistributedMemoryService.
    /// @return True if the memory implementation was deregistered, otherwise false.
    bool deleteMemory()
    {
        if ( _memory == nullptr )
        {
            return false;
        }

        _memory.reset();
        return true;
    }
    
    void onStorageMessage( Ip6Addr sender, uint8_t* data, size_t size, bool isDeleteMessage )
    {
        size_t headerSize = sizeof( int ) + sizeof( bool ) + sizeof( size_t );
        if ( size < headerSize )
        {
            std::cout << "[DistributionMemoryService.onStorageMessage] Malformed message detected. Not saving data." << std::endl;
            return;
        }

        int address = as< int >( data );
        bool isMetadataOnly = as< bool >( data + sizeof( int ) );
        size_t dataSize = as< size_t >( data + sizeof( int ) + sizeof ( bool ) );
        
        if ( size < headerSize + dataSize )
        {
            std::cout << "[DistributionMemoryService.onStorageMessage] The total data size " << headerSize + dataSize << " doesa not match expected size " << size << std::endl;
            return;
        }
        
        _memoryStorageQueue.emplace( sender, data + sizeof( int ) + sizeof( bool ) + sizeof( size_t ),
            dataSize, address, isMetadataOnly, isDeleteMessage );
    }

    void processQueue()
    {
        if ( _memory == nullptr || _memoryStorageQueue.empty() )
        {
            return;
        }

        auto memory = _memoryStorageQueue.front();
        _memoryStorageQueue.pop();
        
        MemoryWriteResult result = memory.isMetadataOnly 
            ? handleMetadataUpdate( memory )
            : handleDataUpdate ( memory );

        if ( result.success )
        {
            onMemoryStored( memory.address, isLeaderMemory() );

            if ( isLeaderMemory() )
            {
                propagateMemoryChange( result.propagationType, memory.address, memory.data.data(), memory.data.size(),
                    result.metadataOnly, result.propagationTarget, DistributionMessageType::DataStorageRequest );
            }
        }
    }
    
    /// @brief Save data in memory.
    /// @param data The data to be stored.
    /// @param size The size of the data.
    /// @param address The address to store the data at
    template < SerializableOrTrivial T >
    bool saveData( T data, int address )
    {
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

        if ( isLeaderMemory() )
        {
            auto result = _memory->writeData( dataBuffer.data(), dataBuffer.size(), address, true );
            if ( result.success )
            {
                propagateMemoryChange( result.propagationType, address, dataBuffer.data(), dataBuffer.size(), 
                    false, result.propagationTarget, DistributionMessageType::DataStorageRequest );
            }
            return result.success;
        }

        sendDataUnicast( address, dataBuffer.data(), dataBuffer.size(), false, 
            _leader, DistributionMessageType::DataStorageRequest );

        return true;
    }

    /// @brief Reads data from specified address in memory.
    /// @param address The memory address to be read
    /// @return MemoryReadResult struct containing information about read success and a method for extracting the data read.
    MemoryReadResult readData( int address )
    {
        return _memory->readData( address );
    }
    
    /// @brief Remove data at specified address
    /// @param address The address of the data in memory
    void removeData( int address )
    {
        if ( isLeaderMemory() )
        {
            auto result = _memory->removeData( address, true );
            if ( result.success )
            {
                propagateMemoryChange( result.propagationType, address, nullptr, 0, false, result.propagationTarget, DistributionMessageType::DataRemovalRequest );
            }
            return;
        }

        sendDataUnicast( address, nullptr, 0, false, _leader, DistributionMessageType::DataRemovalRequest );
        
    }
    
    /// @brief Remove all data from this module's memory.
    void clearLocalMemory()
    {
        return _memory->clear();
    }

    /// @brief Remove all entries in this module's storage queue.
    void clearLocalQueue()
    {
        while ( !_memoryStorageQueue.empty() )
        {
            _memoryStorageQueue.pop();
        }
    }
    
    MemoryReadResult readMetadata( int address, std::string key )
    {
        return _memory->readMetadata( address, key );
    }

    void saveMetadata( int address, std::string key, uint8_t* metadata, std::size_t metadataSize )
    {
        if (isLeaderMemory() )
        {
            auto result = _memory->writeMetadata( address, key, metadata, metadataSize, true );
            if ( result.success )
            {
                propagateMetadataChange(result.propagationType, address, key, metadata, 
                    metadataSize, result.propagationTarget, DistributionMessageType::DataStorageRequest );
            }
            return;
        }

        // If not leader -> we simply request to save metadata
        propagateMetadataChange( MemoryPropagationType::ONE_TARGET, address, key, metadata,
            metadataSize, _leader, DistributionMessageType::DataStorageRequest );
    }

    void removeMetadata( int address, std::string key )
    {
        if ( isLeaderMemory() )
        {
            auto result = _memory->removeMetadata( address, key, true );

            if ( result.success )
            {
                propagateMetadataChange(result.propagationType, address, key, nullptr, 
                    0, result.propagationTarget, DistributionMessageType::DataRemovalRequest );
            }
            return;
        }

        propagateMetadataChange(MemoryPropagationType::ONE_TARGET, address, key, nullptr, 
                    0, _leader, DistributionMessageType::DataRemovalRequest );
    }
    
    void setLeader( Ip6Addr leader )
    {
        _leader = leader;
    }
    
    /// @brief Registers a shared memory implementation in the memory service. Only one implementation may be registered at a time.
    /// @tparam Memory
    /// @param memory The SharedMemoryBase implementation to be registered.
    /// @return True if memory was succesfully registered, otherwise false.
    template< std::derived_from< SharedMemoryBase > Memory >
    bool useMemory( std::unique_ptr< Memory > memory )
    {
        if (_memory != nullptr )
        {
            return false;
        }

        _memory = std::move( memory );
        return true;
    } 
};