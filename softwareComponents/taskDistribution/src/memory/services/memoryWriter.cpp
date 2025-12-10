#include "memoryWriter.hpp"

MemoryWriter::MemoryWriter( MemoryMessagingWrapper& memoryMessaging, rofi::hal::Ip6Addr currentModuleAddress, LoggingService& loggingService )
: _memoryMessaging( memoryMessaging ), _currentModuleAddress( currentModuleAddress ), _loggingService( loggingService ) {}

/// @brief Checks whether the memory, in this instant, is going to process any more write operations that are queued.
/// @return True if there are no more pending writes.
bool MemoryWriter::isMemoryStable()
{
    return _memoryStorageQueue.empty();
}

void MemoryWriter::emplaceIntoQueue( Ip6Addr& sender, uint8_t* data, size_t dataSize,
    int address, bool isMetadataOnly, MemoryRequestType requestType )
{
    _memoryStorageQueue.emplace( sender, data, dataSize, address, isMetadataOnly, requestType );
}

bool MemoryWriter::processQueue()
{
    if ( _memory == nullptr || _memoryStorageQueue.empty() )
    {
        return false;
    }

    auto memoryItem = _memoryStorageQueue.front();
    _memoryStorageQueue.pop();

    MemoryWriteResult result = memoryItem.isMetadataOnly 
        ? handleMetadataUpdate( memoryItem )
        : handleDataUpdate ( memoryItem );

    if ( result.success )
    {
        _memoryMessaging.propagateMemoryChange( result.propagationType, memoryItem.address, memoryItem.data.data(),
            memoryItem.data.size(), result.metadataOnly, result.propagationTarget, 
            memoryItem.isDeleteRequest() ? DistributionMessageType::DataRemovalRequest : DistributionMessageType::DataStorageRequest );
    }

    return true;
}

/// @brief Remove all entries in this module's storage queue.
void MemoryWriter::clearLocalQueue()
{
    while ( !_memoryStorageQueue.empty() )
    {
        _memoryStorageQueue.pop();
    }
}

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

// ================== PRIVATE

MemoryWriteResult MemoryWriter::handleMetadataUpdate( MemoryRequestQueueItem& memoryItem )
{
    MemoryWriteResult result;

    size_t keySize = as< size_t >( memoryItem.data.data() );
    std::string key;
    key.resize( keySize );
    std::memcpy( key.data(), memoryItem.data.data() + sizeof( size_t ), keySize );

    size_t keyDataSize = sizeof( size_t ) + keySize;

    if ( memoryItem.isDeleteRequest() )
    {
        return _memory->removeMetadata( memoryItem.address, key, _memoryMessaging.isLeaderMemory() );
    }

    return _memory->writeMetadata( memoryItem.address, key, 
        memoryItem.data.data() + keyDataSize,
        memoryItem.data.size() - keyDataSize,
        _memoryMessaging.isLeaderMemory() );
}

MemoryWriteResult MemoryWriter::handleDataUpdate( MemoryRequestQueueItem& memoryItem )
{
    if ( memoryItem.isDeleteRequest() )
    {
        return _memory->removeData( memoryItem.address, _memoryMessaging.isLeaderMemory() );
    }
    
    return _memory->writeData( memoryItem.data.data(), memoryItem.data.size(), memoryItem.address, _memoryMessaging.isLeaderMemory() );
}
