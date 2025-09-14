#pragma once

#include <shared_mutex>
#include "memoryEntry.hpp"
#include "../distribution/messaging/messageSender.hpp"
#include "sharedMemoryBase.hpp"
#include <map>

using namespace rofi::hal;
using namespace rofi::net;

class ReplicatedMemoryManager : public SharedMemoryBase {
    const unsigned int METHOD_ID = 2;
    const Ip6Addr& _my_address;
    Ip6Addr _leader;
    MessageSender& _sender;

    std::map< int, MemoryEntry > _storage;
    mutable std::shared_mutex _mutex;

    void sendDataThroughHelper( int address, const uint8_t* data, int size, int stamp )
    {
        std::vector< uint8_t > buffer;
        buffer.resize( size + 3 * sizeof ( int ) );
        as< int >( buffer.data() ) = address;
        as< int >( buffer.data() + sizeof( int ) ) = stamp;
        as< int >( buffer.data() + 2 * sizeof( int ) ) = size;
        std::memcpy( buffer.data() + 3 * sizeof( int ), data, size );

        _sender.broadcastMessage( DistributionMessageType::DataStorageRequest, buffer.data(), buffer.size(), METHOD_ID );
    }

    void saveData( int address, unsigned int stamp, uint8_t* data, int size)
    {
        std::unique_lock lock( _mutex );
        
        auto entry = _storage.find( address );
        if ( entry != _storage.end() )
        {
            if ( _my_address == _leader && entry->second.stamp >= stamp )
            {
                return;
            }
            entry->second.stamp = stamp;
            entry->second.stored_data.resize( size );
            std::memcpy( entry->second.stored_data.data(), data, size );
            return;
        }

        auto res = _storage.emplace(address, MemoryEntry( stamp, size ) );
        if ( res.second )
        {
            std::memcpy( res.first->second.stored_data.data(), data, size );
        }
    }

    void onMemoryReplicationFollower( uint8_t* data )
    {
        int address = as< int >( data );
        unsigned int stamp = as< unsigned int >( data + sizeof( int ) );
        int data_size = as< int >( data + sizeof( int ) + sizeof( unsigned int ) );
        
        saveData( address, stamp, data + 2 * sizeof( int ) + sizeof( unsigned int ), data_size );
        _sender.sendMessage( DistributionMessageType::DataStorageSuccess, _leader );
    }

    void onMemoryReplicationLeader( uint8_t* data )
    {
        int address = as< int >( data );
        unsigned int stamp = as< unsigned int >( data + sizeof( int ) );
        int data_size = as< int >( data + sizeof( int ) + sizeof( unsigned int ) );
        auto current_value = _storage.find( address );
        
        if ( current_value != _storage.end() && current_value->second.stamp >= stamp )
        {
            return;
        }
        
        saveData( address, stamp, data + 2 * sizeof( int ) + sizeof( unsigned int ), data_size );
        // Distribute to followers
        sendDataThroughHelper( address, data + 2 * sizeof( int ) + sizeof( unsigned int ), data_size, stamp + 1 );
        // Await responses from followers?
    }

public:
    ReplicatedMemoryManager( MessageDistributor* distributor, Ip6Addr& myAddress, MessageSender& sender )
    : _my_address( myAddress ), _leader( myAddress ), _sender( sender )
    {
        distributor->registerMethod( METHOD_ID, 
            [ this ] ( Ip6Addr sender, uint8_t* data, unsigned int size ) 
            {
                std::cout << "Received storage message." << std::endl;
                onStorageMessage( sender, data + _sender.headerSize(), size ); 
            },
            [] () { return; } );
    }

    virtual void setLeader( Ip6Addr leader ) override
    {
        _leader = leader;
    }

    void onStorageMessage( Ip6Addr, uint8_t* data, unsigned int )
    { 
        if ( _leader == _my_address )
        {
            return onMemoryReplicationLeader( data );
        }

        onMemoryReplicationFollower( data );
    }

    virtual bool store( uint8_t* data, int size, int address ) override
    {
        unsigned int timestamp = 0;
        auto stored = _storage.find( address );
        
        if ( stored != _storage.end() )
        {
            timestamp = stored->second.stamp;
        }

        timestamp++;
        
        if ( _leader == _my_address )
        {
            saveData( address, timestamp, data, size );
            sendDataThroughHelper( address, data, size, timestamp );
            return true;
        }

        PBuf packet = PBuf::allocate( 3 * sizeof( int ) + size );
        as< int >( packet.payload() ) = address;
        as< unsigned int >( packet.payload() + sizeof( int ) ) = timestamp;
        as< int >( packet.payload() + sizeof( int ) + sizeof( unsigned int ) ) = size;
        std::memcpy( packet.payload() + 2 * sizeof( int ) + sizeof( unsigned int ), data, size );
        _sender.sendMessage( DistributionMessageType::DataStorageRequest, std::move( packet ), _leader );
        return true;
    }

    /// @brief Reads data from an address in the shared memory.
    /// @param address - An address of the requested data.
    /// @return Data in uint8_t* format. Nullptr if no data found under the address.
    virtual std::vector<uint8_t> read( int address ) override
    {
        std::shared_lock lock( _mutex );
        auto entry = _storage.find( address );
        
        if (entry == _storage.end())
        {
            return std::vector< uint8_t >();
        }

        return entry->second.stored_data;   
    }

    virtual bool remove( int address ) override
    {
        std::shared_lock lock( _mutex );
        return _storage.erase( address ) == 1;
    }
    
    virtual void clear() override
    {
        std::shared_lock lock( _mutex );
        _storage.clear();
    }

    // This implementation only allows to read the timestamp with the stamp key.
    virtual std::vector< uint8_t > readMetadata( int address, std::string key ) override
    {
        auto result = std::vector< uint8_t >();

        if ( key != std::string( "stamp" ) )
        {
            return result;
        }

        std::shared_lock lock( _mutex );
        auto entry = _storage.find( address );
        
        if (entry == _storage.end())
        {
            return result;
        }

        result.resize( sizeof( unsigned int ) );
        std::memcpy( result.data(), &(entry->second.stamp), sizeof( unsigned int ) );
        return result; 
    }

    // This implementation does not allow to store metadata.
    virtual bool storeMetadata( int, std::string, uint8_t*, int ) override 
    {
        return false;
    }
};