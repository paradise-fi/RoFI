#include "../../../memory/implementation/replicatedMemory.hpp"

MemoryStorageBehavior ReplicatedMemory::storageBehavior() const
{
    return MemoryStorageBehavior::LeaderFirstStorage;
}

MemoryWriteResult ReplicatedMemory::writeData( uint8_t* data, size_t size, int address, bool isLeader )
{
    MemoryWriteResult result;
    result.metadataOnly = false;
    unsigned int timestamp = as< unsigned int >( data );
    result.success = saveData( address, timestamp, data + sizeof( unsigned int ), size - sizeof( unsigned int ), isLeader );
    result.stored = result.success;

    result.propagationType = isLeader ? MemoryPropagationType::SEND_TO_ALL : MemoryPropagationType::NONE;
    
    return result;
}

MemoryReadResult ReplicatedMemory::readData( int address, bool ) const
{
    MemoryReadResult result;
    result.success = false;

    std::shared_lock lock( _mutex );
    
    auto entry = _storage.find( address );
    if (entry == _storage.end())
    {
        return result;
    }

    result.success = true;
    result.rawData = entry->second.storedData;

    return result;
}

MemoryWriteResult ReplicatedMemory::removeData( int address, bool isLeader )
{
    MemoryWriteResult result;
    result.success = false;

    std::shared_lock lock( _mutex );

    if ( _storage.erase( address ) > 0 )
    {
        result.success = true;
        result.stored = result.success;

        result.propagationType = isLeader ? MemoryPropagationType::SEND_TO_ALL : MemoryPropagationType::NONE;
    }

    return result;
}

void ReplicatedMemory::clear()
{
    std::shared_lock lock( _mutex );
    _storage.clear();
}

// This implementation only allows to read the timestamp with the stamp key.
MemoryReadResult ReplicatedMemory::readMetadata( int address, const std::string& key, bool ) const
{
    MemoryReadResult result;
    result.success = false;

    if ( key != std::string( "stamp" ) )
    {
        return result;
    }

    std::shared_lock lock( _mutex );
    auto entry = _storage.find( address );
    
    if ( entry == _storage.end() )
    {
        return result;
    }

    result.rawData.resize( sizeof( unsigned int ) );
    std::memcpy( result.rawData.data(), &(entry->second.stamp), sizeof( unsigned int ) );
    result.success = true;

    return result; 
}

// This implementation does not allow to store metadata.
MemoryWriteResult ReplicatedMemory::writeMetadata( int, const std::string&, uint8_t*, size_t, bool ) 
{
    MemoryWriteResult result;
    result.success = false;
    result.metadataOnly = true;
    return result;
}

MemoryWriteResult ReplicatedMemory::removeMetadata( int, const std::string&, bool )
{
    MemoryWriteResult result;
    result.success = false;
    result.metadataOnly = true;
    return result;
}

std::vector< uint8_t > ReplicatedMemory::serializeDataToMemoryFormat( uint8_t* data, size_t dataSize, int address, bool ) const
{
    unsigned int timestamp = 0;
    auto stored = _storage.find( address );

    if ( stored != _storage.end() )
    {
        timestamp = stored->second.stamp;
    }

    timestamp++;

    std::vector< uint8_t > buffer;
    buffer.resize( sizeof( unsigned int ) + dataSize );
    std::memcpy( buffer.data(), &timestamp, sizeof( unsigned int ) );
    std::memcpy( buffer.data() + sizeof( unsigned int ), data, dataSize );

    return buffer;
}

// =================== Private
bool ReplicatedMemory::saveData( int address, unsigned int stamp, uint8_t* data, size_t size, bool isLeader)
    {
        std::unique_lock lock( _mutex );
        
        auto entry = _storage.find( address );

        if ( entry != _storage.end() )
        {
            if ( isLeader && entry->second.stamp >= stamp )
            {
                return false;
            }
            
            entry->second.stamp = stamp;
            entry->second.storedData.resize( size );
            std::memcpy( entry->second.storedData.data(), data, size );
            
            return true;
        }

        auto res = _storage.emplace(address, MemoryEntry( stamp, size ) );
        if ( res.second )
        {
            std::memcpy( res.first->second.storedData.data(), data, size );
            
            return true;
        }

        return false;
    }