#include "../../../include/memoryFacade.hpp"

MemoryFacade::MemoryFacade( DistributedMemoryService& memoryService ) : _memoryService( memoryService ) {}
    
bool MemoryFacade::isMemoryRegistered()
{
    return _memoryService.isMemoryRegistered();
}

bool MemoryFacade::deleteMemory()
{
    return _memoryService.deleteMemory();
}

bool MemoryFacade::isMemoryStable()
{
    return _memoryService.isMemoryStable();
}

MemoryReadResult MemoryFacade::readData( int address )
{
    return _memoryService.readData( address );
}

void MemoryFacade::removeData( int address )
{
    return _memoryService.removeData( address );
}

void MemoryFacade::clearLocalMemory()
{
    return _memoryService.clearLocalMemory();
}

void MemoryFacade::clearLocalQueue()
{
    return _memoryService.clearLocalQueue();
}

MemoryReadResult MemoryFacade::readMetadata( int address, const std::string& key )
{
    return _memoryService.readMetadata( address, key );
}

bool MemoryFacade::saveMetadata( int address, const std::string& key, uint8_t* metadata, std::size_t metadataSize )
{
    return _memoryService.saveMetadata( address, key, metadata, metadataSize );
}

void MemoryFacade::removeMetadata( int address, const std::string& key )
{
    return _memoryService.removeMetadata( address, key );
}
