#include "memoryService.hpp"

DistributedMemoryService::DistributedMemoryService( MessagingService& messaging,
    Ip6Addr& currentModuleAddress, LoggingService& loggingService, int blockingMessageTimeoutMs )
:   _loggingService( loggingService ),
    _memoryMessagingWrapper( MemoryMessagingWrapper( messaging, loggingService, METHOD_ID, currentModuleAddress ) ),
    _memoryWriter( MemoryWriter(  _memoryMessagingWrapper, currentModuleAddress, loggingService ) ),
    _memoryReader( MemoryReader( _memoryMessagingWrapper, currentModuleAddress, messaging, loggingService, blockingMessageTimeoutMs ) ) {}

bool DistributedMemoryService::isMemoryRegistered()
{
    return _memory != nullptr;
}

/// @brief Removes / deregisters the shared memory implementation from DistributedMemoryService.
/// @return True if the memory implementation was deregistered, otherwise false.
bool DistributedMemoryService::deleteMemory()
{
    if ( _memory == nullptr )
    {
        return false;
    }

    _memoryMessagingWrapper.unregisterMemory();
    _memoryWriter.unregisterMemory();
    _memoryReader.unregisterMemory();
    _memory.reset();
    return true;
}

void DistributedMemoryService::onMemoryMessage( const Ip6Addr& sender, uint8_t* data, size_t size, MemoryRequestType requestType )
{
    switch ( requestType )
    {
        case MemoryRequestType::MemoryDelete:
        case MemoryRequestType::MemoryWrite:
            _memoryWriter.processRemoteDataWriteRequest( sender, data, size, requestType );
            return;
        case MemoryRequestType::MemoryRead:
            _memoryReader.processRemoteDataReadRequest( data, size );
            return;
        default:
            _loggingService.logError( "Distributed Memory - Invalid Memory Operation Request detected. Likely caused by a non-memory message passed to memory.");
            return;
    }
}

MemoryReadResult DistributedMemoryService::readData( int address, bool isUserCall )
{
    return _memoryReader.readData( address, isUserCall );
}

void DistributedMemoryService::removeData( int address )
{
    _memoryWriter.removeData( address );
}

/// @brief Remove all data from this module's memory.
void DistributedMemoryService::clearLocalMemory()
{
    return _memory->clear();
}

MemoryReadResult DistributedMemoryService::readMetadata( int address, const std::string& key, bool isUserCall )
{
    return _memoryReader.readMetadata( address, key, isUserCall );
}

bool DistributedMemoryService::saveMetadata( int address, const std::string& key, uint8_t* metadata, std::size_t metadataSize )
{
    return _memoryWriter.saveMetadata( address, key, metadata, metadataSize );
}

void DistributedMemoryService::removeMetadata( int address, const std::string& key )
{
    return _memoryWriter.removeMetadata( address, key );
}

void DistributedMemoryService::setLeader( const Ip6Addr& leader )
{
    _memoryMessagingWrapper.setLeader( leader );
}

/// @brief Retrieves the internal implementation of memory for custom functionality.
/// @return NULLOPT if no memory has been registered. Otherwise a reference for the memory.
std::optional< std::reference_wrapper< DistributedMemoryBase > > DistributedMemoryService::memory()
{
    if ( _memory == nullptr )
    {
        return *_memory;
    }

    return std::nullopt;
}