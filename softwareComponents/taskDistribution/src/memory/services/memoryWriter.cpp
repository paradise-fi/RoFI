#include "memoryWriter.hpp"

MemoryWriter::MemoryWriter( MemoryMessagingWrapper& memoryMessaging, rofi::hal::Ip6Addr currentModuleAddress, LoggingService& loggingService )
: _memoryMessaging( memoryMessaging ), _currentModuleAddress( currentModuleAddress ), _loggingService( loggingService ) {}

void MemoryWriter::unregisterMemory()
{
    _memory = nullptr;
}

void MemoryWriter::registerMemory( DistributedMemoryBase* memory )
{
    _memory = memory;
}

void MemoryWriter::removeMetadata( int address, const std::string& key )
{
    if ( _memory == nullptr )
    {
        return;
    }

    if ( _memory->storageBehavior() == MemoryStorageBehavior::LeaderFirstStorage )
    {
        if ( _memoryMessaging.isLeaderMemory() )
        {
            auto result = _memory->removeMetadata( address, key, true );

            if ( result.success )
            {
                _memoryMessaging.propagateMetadataChange( result.propagationType, address, key, nullptr, 
                    0, result.propagationTarget, DistributionMessageType::DataRemovalRequest );
            }
            return;
        }

        _memoryMessaging.propagateMetadataChange( MemoryPropagationType::ONE_TARGET, address, key, nullptr, 
                    0, _memoryMessaging.getLeader(), DistributionMessageType::DataRemovalRequest );
        return;
    }

    auto result = _memory->removeMetadata( address, key, false );

    if ( result.success )
    {
        _memoryMessaging.propagateMetadataChange( result.propagationType, address, key, nullptr, 
            0, result.propagationTarget, DistributionMessageType::DataRemovalRequest );
    }
}

bool MemoryWriter::saveMetadata( int address, const std::string& key, uint8_t* metadata, std::size_t metadataSize )
{
    if ( _memory == nullptr )
    {
        return false;
    }

    if ( _memory->storageBehavior() == MemoryStorageBehavior::LeaderFirstStorage )
    {
        if ( _memoryMessaging.isLeaderMemory() )
        {
            auto result = _memory->writeMetadata( address, key, metadata, metadataSize, true );

            if ( result.success )
            {
                _memoryMessaging.propagateMetadataChange(result.propagationType, address, key, metadata, 
                    metadataSize, result.propagationTarget, DistributionMessageType::DataStorageRequest );
            }
            
            return true;
        }

        // If not leader -> we simply request to save metadata
        _memoryMessaging.propagateMetadataChange( MemoryPropagationType::ONE_TARGET, address, key, metadata,
            metadataSize, _memoryMessaging.getLeader(), DistributionMessageType::DataStorageRequest );

        return true;
    }

    auto result = _memory->writeMetadata( address, key, metadata, metadataSize, false );

    if ( result.success )
    {
        _memoryMessaging.propagateMetadataChange(result.propagationType, address, key, metadata, 
            metadataSize, result.propagationTarget, DistributionMessageType::DataStorageRequest );
    }

    return result.success;
}

/// @brief Remove data at specified address
/// @param address The address of the data in memory
void MemoryWriter::removeData( int address )
{
    if ( _memory == nullptr )
    {
        return;
    }
    
    if ( _memory->storageBehavior() == MemoryStorageBehavior::LeaderFirstStorage )
    {
        if ( _memoryMessaging.isLeaderMemory() )
        {
            auto result = _memory->removeData( address, true );
            if ( result.success )
            {
                _memoryMessaging.propagateMemoryChange( result.propagationType, 
                    address, nullptr, 0, false, result.propagationTarget, DistributionMessageType::DataRemovalRequest );
            }
            return;
        }

        _memoryMessaging.propagateMemoryChange( MemoryPropagationType::ONE_TARGET, 
            address, nullptr, 0, false, _memoryMessaging.getLeader(), DistributionMessageType::DataRemovalRequest );
        return;
    }

    auto result = _memory->removeData( address, false );
    if ( result.success )
    {
        _memoryMessaging.propagateMemoryChange( result.propagationType, 
            address, nullptr, 0, false, result.propagationTarget, DistributionMessageType::DataRemovalRequest );
    }
}

void MemoryWriter::processRemoteDataWriteRequest( uint8_t* data, size_t size, MemoryRequestType requestType )
{
    size_t headerSize = _memoryMessaging.genericMemoryMessageHeaderSize();

    if ( size < headerSize )
    {
        _loggingService.logError( "Distributed Memory - Malformed message deceted. Data not saved." );
        return;
    }

    size_t offset = 0;

    int address = as< int >( data );
    offset += sizeof( int );

    bool isMetadataOnly = as< bool >( data + offset );
    offset += sizeof( bool );

    size_t dataSize = as< size_t >( data + offset );
    offset += sizeof( size_t );

    MemoryWriteResult result = isMetadataOnly 
        ? handleMetadataUpdate( address, data + offset, dataSize, requestType )
        : handleDataUpdate ( address, data + offset, dataSize, requestType );

    if ( result.success )
    {
        _memoryMessaging.propagateMemoryChange( result.propagationType, 
            address, data + offset, dataSize, result.metadataOnly, result.propagationTarget, 
            requestType == MemoryRequestType::MemoryDelete 
                ? DistributionMessageType::DataRemovalRequest 
                : DistributionMessageType::DataStorageRequest );
    }
}

// ================== PRIVATE

MemoryWriteResult MemoryWriter::handleMetadataUpdate( int address, uint8_t* data, size_t dataSize, MemoryRequestType requestType )
{
    MemoryWriteResult result;

    size_t keySize = as< size_t >( data );
    std::string key;
    if ( keySize > 0 )
    {
        key.resize( keySize );
        std::memcpy( key.data(), data + sizeof( size_t ), keySize );
    }
    
    size_t keyDataSize = sizeof( size_t ) + keySize;

    if ( requestType == MemoryRequestType::MemoryDelete )
    {
        return _memory->removeMetadata( address, key, _memoryMessaging.isLeaderMemory() );
    }

    return _memory->writeMetadata( address, key, 
        data + keyDataSize, dataSize - keyDataSize,
        _memoryMessaging.isLeaderMemory() );
}

MemoryWriteResult MemoryWriter::handleDataUpdate( int address, uint8_t* data, size_t dataSize, MemoryRequestType requestType )
{
    if ( requestType == MemoryRequestType::MemoryDelete )
    {
        return _memory->removeData(address, _memoryMessaging.isLeaderMemory() );
    }
    
    return _memory->writeData( data, dataSize, address, _memoryMessaging.isLeaderMemory() );
}
