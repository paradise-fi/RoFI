#pragma once
#include "../memory/distributedMemoryBase.hpp"
#include <queue>
#include "lwip++.hpp"
#include "atoms/util.hpp"
#include "../messagingService.hpp"
#include "../memory/memoryRequestType.hpp"
#include "loggingService.hpp"
#include "callbacks/callbackService.hpp"

using namespace rofi::hal;
using namespace rofi::net;

struct MemoryRequestQueueItem
{
    Ip6Addr sender;
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

    MemoryRequestQueueItem(Ip6Addr sender, uint8_t* buffer, size_t bufferSize, int address, bool isMetadataOnly, MemoryRequestType requestType )
    : sender( sender ), address( address ), isMetadataOnly( isMetadataOnly ), requestType( requestType )
    {
        data.resize( bufferSize );
        std::memcpy( data.data(), buffer, bufferSize );
    } 
};

class DistributedMemoryService
{
    const unsigned int METHOD_ID = 2;
    const unsigned int READ_BATCH_SIZE = 5;

    MessagingService& _messaging;
    Ip6Addr _leader;
    const Ip6Addr& _currentModuleAddress;
    std::unique_ptr< DistributedMemoryBase > _memory;
    
    // For external read requests that need to be sent over network.
    std::queue<MemoryRequestQueueItem> _memoryReadQueue;
    
    // For external write requests.
    std::queue<MemoryRequestQueueItem> _memoryStorageQueue;
    LoggingService& _loggingService;
    CallbackService& _callbackService;
    int _blockingMessageTimeoutMs = 300;

    bool isLeaderMemory()
    {
        return _leader == _currentModuleAddress;
    }

    size_t genericMemoryMessageHeaderSize()
    {
        return sizeof( int ) + sizeof( bool ) + sizeof( size_t );
    }
    
    void prepareMemoryMessageData( uint8_t* buffer, int address, const uint8_t* data, size_t dataSize, bool isMetadataOnly )
    {
        as< int >( buffer ) = address;
        as< bool >( buffer + sizeof( int ) ) = isMetadataOnly;
        as< size_t >( buffer + sizeof( int ) + sizeof( bool ) ) = dataSize;

        if ( dataSize != 0 && data != nullptr )
        {
            std::memcpy( buffer + genericMemoryMessageHeaderSize(), data, dataSize );
        }
    }

    void sendDataBroadcast( int address, const uint8_t* data, size_t size, bool isMetadataOnly, DistributionMessageType messageType )
    {
        // ADDRESS - IS METADATA ONLY - DATA SIZE - DATA
        std::vector< uint8_t > buffer;

        buffer.resize( size + genericMemoryMessageHeaderSize() );
        prepareMemoryMessageData( buffer.data(), address, data, size, isMetadataOnly );

        _messaging.sender().broadcastMessage( messageType, buffer.data(), buffer.size(), METHOD_ID );
    }

    void sendDataUnicast( int address, const uint8_t* data, size_t size, bool isMetadataOnly, const Ip6Addr& target, DistributionMessageType messageType )
    {
        PBuf packet = PBuf::allocate( static_cast< int >( genericMemoryMessageHeaderSize() + size ) );
        
        prepareMemoryMessageData( packet.payload(), address, data, size, isMetadataOnly );

        auto sendResult = _messaging.sender().sendMessage( messageType, std::move( packet ), target );

        if ( !sendResult.success )
        {
            std::ostringstream stream;
            stream << " Memory Messaging Failure to " << target << ", reason: " << sendResult.messsage;
            _loggingService.logError( stream.str() );
        }
    }

    void propagateMemoryChange( MemoryPropagationType type, int address, uint8_t* data, size_t size,
        bool isMetadataOnly, std::optional< Ip6Addr > target, DistributionMessageType messageType )
    {
        switch ( type )
        {
            case MemoryPropagationType::ONE_TARGET:
                sendDataUnicast( address, data, size, isMetadataOnly, target.has_value() ? target.value() : _leader, messageType );
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

    MemoryWriteResult handleMetadataUpdate( MemoryRequestQueueItem& memoryItem )
    {
        MemoryWriteResult result;

        size_t keySize = as< size_t >( memoryItem.data.data() );
        std::string key;
        key.resize( keySize );
        std::memcpy( key.data(), memoryItem.data.data() + sizeof( size_t ), keySize );

        size_t keyDataSize = sizeof( size_t ) + keySize;

        if ( memoryItem.isDeleteRequest() )
        {
            return _memory->removeMetadata( memoryItem.address, key, isLeaderMemory() );
        }

        return _memory->writeMetadata( memoryItem.address, key, 
            memoryItem.data.data() + keyDataSize,
            memoryItem.data.size() - keyDataSize,
            isLeaderMemory() );
    }

    MemoryWriteResult handleDataUpdate( MemoryRequestQueueItem& memoryItem )
    {
        if ( memoryItem.isDeleteRequest() )
        {
            return _memory->removeData( memoryItem.address, isLeaderMemory() );
        }
        
        return _memory->writeData( memoryItem.data.data(), memoryItem.data.size(), memoryItem.address, isLeaderMemory() );
    }

    MemoryReadResult readDataBlocking( Ip6Addr& target, uint8_t* data, size_t dataSize )
    {
        MemoryReadResult result;
        result.success = false;
        auto remoteReadResult = _messaging.sendMessageBlocking( target, DistributionMessageType::DataReadRequestBlocking, data, dataSize, _blockingMessageTimeoutMs );
        if ( remoteReadResult.success )
        {
            auto success = as< bool >( remoteReadResult.rawData.data() + genericMemoryMessageHeaderSize() );
            size_t readSize = as< size_t >( remoteReadResult.rawData.data() + sizeof( bool ) + genericMemoryMessageHeaderSize() );
            
            if ( success )
            {
                result.rawData.resize( readSize );
                std::memcpy( result.rawData.data(), remoteReadResult.rawData.data() + sizeof( bool ) + sizeof( size_t ) + genericMemoryMessageHeaderSize(), readSize );
                result.success = true;
            }
        }
        else
        {
            std::ostringstream stream;
            stream << "MemoryService - Remote Data Retrieval at " << target << " failed, reason: " << remoteReadResult.statusMessage;
            _loggingService.logError( stream.str() );
        }

        return result;
    }

    MemoryReadResult forwardReadRequest( Ip6Addr& target, uint8_t* data, size_t dataSize )
    {
        MemoryReadResult result;
        result.success = true;
        result.requestRemoteRead = true;
        auto remoteReadRequestResult = _messaging.sender().sendMessage( DistributionMessageType::DataReadRequest, data, dataSize, target );
        
        if ( !remoteReadRequestResult.success )
        {
            result.success = false;
            std::ostringstream stream;
            stream << "MemoryService - Remote Data Retrieval relay to " << target << " failed, reason: " << remoteReadRequestResult.messsage;
            _loggingService.logError( stream.str() );
        }

        return result;
    }

    MemoryReadResult readDataInternal( int address, const Ip6Addr& origin, bool isUserCall )
    {
        auto result = _memory->readData( address, _leader == _currentModuleAddress );

        std::vector< uint8_t > data;
        if ( !result.requestRemoteRead && isUserCall )
        {
            return result;
        }

        if ( !result.requestRemoteRead && !isUserCall )
        {
            data.resize( result.rawData.size() + sizeof( size_t ) + sizeof( bool ) );
            as< bool >( data.data() ) = result.success;
            as< size_t >( data.data() + sizeof( bool ) ) = result.success ? result.rawData.size() : 0;
            if ( result.success )
            {
                std::memcpy( data.data() + sizeof( bool ) + sizeof( size_t ), result.rawData.data(), result.rawData.size() );
            }

            // Send a message to the origin.
            sendDataUnicast( address, data.data(), data.size(), false, origin, DistributionMessageType::BlockingMessageResponse );
            return result;
        }

        result.success = false;
        Ip6Addr& target = result.readTarget.has_value() ? result.readTarget.value() : _leader;
        data.resize( sizeof( int ) + sizeof( bool ) + sizeof( size_t ) + sizeof( Ip6Addr ) + sizeof(u8_t));
        as< Ip6Addr >( data.data() ) = origin;
        as< u8_t >( data.data() + sizeof( Ip6Addr ) ) = origin.zone;
        as< int >( data.data() + sizeof( Ip6Addr ) + sizeof(u8_t) ) = address;
        as< bool >( data.data() + sizeof( int ) + sizeof( Ip6Addr ) + sizeof(u8_t)  ) = false;
        as< size_t >( data.data() + sizeof( int ) + sizeof( bool ) + sizeof( Ip6Addr ) + sizeof(u8_t)  ) = 0;

        if ( isUserCall ) 
        {
            return readDataBlocking( target, data.data(), data.size() );
        }

        auto forwardResult = forwardReadRequest( target, data.data(), data.size() );

        if ( !forwardResult.success )
        {
            std::vector< uint8_t > errorMessage;
            errorMessage.resize( sizeof( bool ) + sizeof( size_t ) );
            as< bool >( errorMessage.data() ) = false;
            as< size_t >( errorMessage.data() + sizeof( bool ) ) = 0;
            auto messageResult = _messaging.sender().sendMessage( DistributionMessageType::BlockingMessageResponse, errorMessage.data(), errorMessage.size(), origin );
            if ( !messageResult.success )
            {
                std::ostringstream stream;
                stream << "MemoyService - Error while fetching data for blocking read for " << origin << ", reason: " << messageResult.messsage;
                _loggingService.logError( stream.str() );
            }
        }

        return forwardResult;
    }

    MemoryReadResult readMetadataInternal( int address, const Ip6Addr& origin, const std::string key, bool isUserCall )
    {
        auto result = _memory->readMetadata( address, key, _leader == _currentModuleAddress );

        std::vector< uint8_t > data;
        if ( !result.requestRemoteRead && isUserCall )
        {
            return result;
        }

        if ( !result.requestRemoteRead && !isUserCall )
        {
            data.resize( result.rawData.size() + sizeof( size_t ) + sizeof( bool ) );
            as< bool >( data.data() ) = result.success;
            as< size_t >( data.data() + sizeof( bool ) ) = result.success ? result.rawData.size() : 0;
            if ( result.success )
            {
                std::memcpy( data.data() + sizeof( bool ) + sizeof( size_t ), result.rawData.data(), result.rawData.size() );
            }

            // Send a message to the origin.
            sendDataUnicast( address, data.data(), data.size(), true, origin, DistributionMessageType::BlockingMessageResponse );
            return result;
        }

        result.success = false;
        Ip6Addr& target = result.readTarget.has_value() ? result.readTarget.value() : _leader;
        
        data.resize( sizeof( int ) + sizeof( bool ) + sizeof( size_t ) + sizeof( Ip6Addr ) + key.size() + sizeof( u8_t ) );
        as< Ip6Addr >( data.data() ) = origin;
        as< u8_t >( data.data() + sizeof( Ip6Addr ) ) = origin.zone;
        as< int >( data.data() + sizeof( Ip6Addr ) + sizeof( u8_t ) ) = address;
        as< bool >( data.data() + sizeof( int ) + sizeof( Ip6Addr ) + sizeof( u8_t ) ) = false;
        as< size_t >( data.data() + sizeof( int ) + sizeof( bool ) + sizeof( Ip6Addr ) + sizeof( u8_t )  ) = key.size();
        std::memcpy( data.data() + sizeof( int ) + sizeof( bool ) + sizeof( Ip6Addr ) + sizeof( size_t ) + sizeof( u8_t ),
                     key.data(), key.size() );

        return isUserCall 
            ? readDataBlocking( target, data.data(), data.size() )
            : forwardReadRequest( target, data.data(), data.size() );
    }

    void handleReadRequest( MemoryRequestQueueItem& memoryItem )
    {
        MemoryReadResult readResult;
        if ( memoryItem.isMetadataOnly )
        {
            size_t keySize = as< size_t >( memoryItem.data.data() );
            std::string key;
            key.resize( keySize );
            std::memcpy( key.data(), memoryItem.data.data() + sizeof( size_t ), keySize );

            readResult = readMetadataInternal( memoryItem.address, memoryItem.sender, key, false ); 
        }
        else
        {
            readResult = readDataInternal( memoryItem.address, memoryItem.sender, false );
        }
    }

    bool processMemoryQueue( std::queue< MemoryRequestQueueItem >& queue )
    {
        if ( _memory == nullptr || queue.empty() )
        {
            return false;
        }

        auto memoryItem = queue.front();
        queue.pop();

        if ( memoryItem.isReadRequest() )
        {
            handleReadRequest( memoryItem );
            return true;
        }

        MemoryWriteResult result = memoryItem.isMetadataOnly 
            ? handleMetadataUpdate( memoryItem )
            : handleDataUpdate ( memoryItem );

        if ( result.success )
        {
            if ( result.stored )
            {
                _callbackService.invokeOnMemoryStored( memoryItem.address, isLeaderMemory(), *this );
            }
            
            propagateMemoryChange( result.propagationType, memoryItem.address, memoryItem.data.data(), memoryItem.data.size(),
                result.metadataOnly, result.propagationTarget, DistributionMessageType::DataStorageRequest );
        }

        return true;
    }

public:
    DistributedMemoryService( MessageDistributor& distributor, MessagingService& messaging,
        Ip6Addr& currentModuleAddress, LoggingService& loggingService, 
        CallbackService& callbackService, int blockingMessageTimeoutMs = 300 )
    : _messaging( messaging ), _currentModuleAddress( currentModuleAddress ),
      _loggingService( loggingService ), _callbackService( callbackService ), 
      _blockingMessageTimeoutMs( blockingMessageTimeoutMs )
    {
        distributor.registerMethod( METHOD_ID, 
            [ this ] ( Ip6Addr sender, uint8_t* data, unsigned int size ) 
            {
                auto messageType = as< DistributionMessageType >( data );
                onMemoryMessage( sender, data + _messaging.sender().headerSize(), size, mapMessageToMemoryRequest(messageType) ); 
            },
            [] () { return; } );
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
    
    void onMemoryMessage( Ip6Addr sender, uint8_t* data, size_t size, MemoryRequestType requestType )
    {
        if ( requestType == MemoryRequestType::InvalidOperation )
        {
            _loggingService.logError( "Distributed Memory - Invalid Memory Operation Request detected. Likely caused by a non-memory message passed to memory.");
        }

        size_t headerSize = sizeof( int ) + sizeof( bool ) + sizeof( size_t );
        
        if ( requestType == MemoryRequestType::MemoryRead )
        {
            headerSize += sizeof( Ip6Addr );
        }
        
        if ( size < headerSize )
        {
            _loggingService.logError( "Distributed Memory - Malformed message deceted. Data not saved." );
            return;
        }
                
        size_t offset = 0;
        
        if ( requestType == MemoryRequestType::MemoryRead )
        {
            sender = as< Ip6Addr >( data );
            sender.zone = as< u8_t >( data + sizeof( Ip6Addr ) );
            offset += sizeof( Ip6Addr ) + sizeof( u8_t );
        }
        
        int address = as< int >( data + offset );
        offset += sizeof( int );
        bool isMetadataOnly = as< bool >( data + offset );
        offset += sizeof( bool );
        size_t dataSize = as< size_t >( data + offset );
        offset += sizeof( size_t );
        
        if ( size < headerSize + dataSize )
        {
            std::ostringstream stream;
            stream << "Distributed Memory -  The total data size " << headerSize + dataSize << " does not match expected size " << size;
            _loggingService.logError( stream.str() );
            return;
        }

        if ( requestType == MemoryRequestType::MemoryRead )
        {
            _memoryReadQueue.emplace( sender, data + offset,
            dataSize, address, isMetadataOnly, requestType );   
        }
        else
        {
            _memoryStorageQueue.emplace( sender, data + offset,
                dataSize, address, isMetadataOnly, requestType );
        }
    }

    /// @brief Checks whether the memory, in this instant, is going to process any more write operations that are queued.
    /// @return True if there are no more pending writes.
    bool isMemoryStable()
    {
        return _memoryStorageQueue.empty();
    }

    void processQueue()
    {
        processMemoryQueue( _memoryStorageQueue );

        for ( unsigned int i = 0; i < READ_BATCH_SIZE; ++i )
        {
            if ( !processMemoryQueue( _memoryReadQueue ) )
            {
                break;
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
        if ( !isMemoryRegistered() )
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

            propagateMemoryChange( MemoryPropagationType::ONE_TARGET, address, 
                dataBuffer.data(), dataBuffer.size(), false, _leader, 
                DistributionMessageType::DataStorageRequest );

            return true;
        }

        auto result = _memory->writeData( dataBuffer.data(), dataBuffer.size(), address, false );
        if ( result.success )
        {
            propagateMemoryChange( result.propagationType, address, dataBuffer.data(), dataBuffer.size(), 
                false, result.propagationTarget, DistributionMessageType::DataStorageRequest );
        }
        return result.success;
    }

    /// @brief Reads data from specified address in memory.
    /// @param address The memory address to be read
    /// @return MemoryReadResult struct containing information about read success and a method for extracting the data read.
    MemoryReadResult readData( int address )
    {
        if ( !isMemoryRegistered() )
        {
            return MemoryReadResult{ false, false, std::nullopt, std::vector< uint8_t >() };
        }
        return readDataInternal( address, _currentModuleAddress, true );
    }
    
    /// @brief Remove data at specified address
    /// @param address The address of the data in memory
    void removeData( int address )
    {
        if ( !isMemoryRegistered() )
        {
            return;
        }
        
        if ( _memory->storageBehavior() == MemoryStorageBehavior::LeaderFirstStorage )
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

            propagateMemoryChange( MemoryPropagationType::ONE_TARGET, address, nullptr, 0, false, _leader, DistributionMessageType::DataRemovalRequest );
            return;
        }

        auto result = _memory->removeData( address, false );
        if ( result.success )
        {
            propagateMemoryChange( result.propagationType, address, nullptr, 0, false, result.propagationTarget, DistributionMessageType::DataRemovalRequest );
        }
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

        while ( !_memoryReadQueue.empty() )
        {
            _memoryReadQueue.pop();
        }
    }
    
    MemoryReadResult readMetadata( int address, const std::string& key )
    {
        if ( !isMemoryRegistered() )
        {
            return MemoryReadResult{ false, false, std::nullopt, std::vector< uint8_t >() };
        }

        return readMetadataInternal( address, _currentModuleAddress, key,  true );
    }

    bool saveMetadata( int address, const std::string& key, uint8_t* metadata, std::size_t metadataSize )
    {
        if ( !isMemoryRegistered() )
        {
            return false;
        }

        if ( _memory->storageBehavior() == MemoryStorageBehavior::LeaderFirstStorage )
        {
            if ( isLeaderMemory() )
            {
                auto result = _memory->writeMetadata( address, key, metadata, metadataSize, true );
                if ( result.success )
                {
                    propagateMetadataChange(result.propagationType, address, key, metadata, 
                        metadataSize, result.propagationTarget, DistributionMessageType::DataStorageRequest );
                }
                return true;
            }

            // If not leader -> we simply request to save metadata
            propagateMetadataChange( MemoryPropagationType::ONE_TARGET, address, key, metadata,
                metadataSize, _leader, DistributionMessageType::DataStorageRequest );

            return true;
        }

        auto result = _memory->writeMetadata( address, key, metadata, metadataSize, false );
        if ( result.success )
        {
            propagateMetadataChange(result.propagationType, address, key, metadata, 
                metadataSize, result.propagationTarget, DistributionMessageType::DataStorageRequest );
        }
        return result.success;
    }

    void removeMetadata( int address, const std::string& key )
    {
        if ( !isMemoryRegistered() )
        {
            return;
        }

        if ( _memory->storageBehavior() == MemoryStorageBehavior::LeaderFirstStorage )
        {
            if ( isLeaderMemory() )
            {
                auto result = _memory->removeMetadata( address, key, true );

                if ( result.success )
                {
                    propagateMetadataChange( result.propagationType, address, key, nullptr, 
                        0, result.propagationTarget, DistributionMessageType::DataRemovalRequest );
                }
                return;
            }

            propagateMetadataChange( MemoryPropagationType::ONE_TARGET, address, key, nullptr, 
                        0, _leader, DistributionMessageType::DataRemovalRequest );
            return;
        }

        auto result = _memory->removeMetadata( address, key, false );

        if ( result.success )
        {
            propagateMetadataChange( result.propagationType, address, key, nullptr, 
                0, result.propagationTarget, DistributionMessageType::DataRemovalRequest );
        }
    }
    
    void setLeader( Ip6Addr leader )
    {
        _leader = leader;
    }
    
    /// @brief Retrieves the internal implementation of memory for custom functionality.
    /// @return NULLOPT if no memory has been registered. Otherwise a reference for the memory.
    std::optional< std::reference_wrapper< DistributedMemoryBase > > memory()
    {
        if ( _memory == nullptr )
        {
            return *_memory;
        }

        return std::nullopt;
    }

    /// @brief Registers a shared memory implementation in the memory service. Only one implementation may be registered at a time.
    /// @tparam Memory
    /// @param memory The DistributedMemoryBase implementation to be registered.
    /// @return True if memory was succesfully registered, otherwise false.
    template< std::derived_from< DistributedMemoryBase > Memory >
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