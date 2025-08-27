#include "../memory/sharedMemoryBase.hpp"

class MemoryService
{
    std::unique_ptr< SharedMemoryBase > _memory;
    
public:
    bool isMemoryRegistered()
    {
        return _memory != nullptr;
    }

    bool deleteMemory()
    {
        if ( _memory == nullptr )
        {
            return false;
        }

        _memory.reset();
        return true;
    }

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

    bool saveData( uint8_t* data, int size, int address )
    {
        return _memory->store( data, size, address );
    }

    template< typename T >
    bool readData( int address, T& out )
    {
        std::vector< uint8_t > data = _memory->read( address );

        if ( data.size() < sizeof( T ) )
        {
            return false;    
        }

        std::memcpy( &out, data.data(), sizeof( T ) );
        return true;
    }

    void onStorageMessage( Ip6Addr sender, uint8_t* data, unsigned int size )
    {
        _memory->onStorageMessage( sender, data, size );
    }

    void setLeader( Ip6Addr leader )
    {
        _memory->setLeader( leader );
    }
};