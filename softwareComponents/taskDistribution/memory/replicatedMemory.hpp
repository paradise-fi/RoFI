#include <shared_mutex>
#include "memoryEntry.hpp"
#include "../distribution/messageSender.hpp"
#include "sharedMemoryBase.hpp"
#include <map>

using namespace rofi::hal;
using namespace rofi::net;

class ReplicatedMemoryManager : public SharedMemoryBase {
    const unsigned int METHOD_ID = 2;
    unsigned int sender_stamp = 0;
    NetworkManager& _net_manager;
    MessageSender& _sender;
    const Ip6Addr& _my_address;
    Ip6Addr _leader;
    // TODO: MUST BE THREAD SAFE!!!!!
    std::map< int, MemoryEntry > _storage;
    mutable std::shared_mutex _mutex;

    void sendDataThroughHelper( PBuf&& data )
    {
        Protocol* proto = _net_manager.getProtocol( "message-distributor" );
        if ( proto == nullptr )
        {
            std::cout << "[ReplicatedMemory] Protocol message-distributor not found!" << std::endl;
            return;
        }

        auto distributor = reinterpret_cast< MessageDistributor* >( proto );
        sender_stamp++;
        std::vector< uint8_t > data_vector( data.size() );
        std::memcpy( data_vector.data(), data.payload(), data.size() );
        distributor->sendMessage( _my_address, METHOD_ID, sender_stamp, data_vector.data(), data_vector.size() );
    }

    void sendDataThroughHelper( int address, const uint8_t* data, int size, int stamp )
    {
        sender_stamp++;
        Protocol* proto = _net_manager.getProtocol( "message-distributor" );
        if ( proto == nullptr )
        {
            std::cout << "[ReplicatedMemory] Protocol message-distributor not found!" << std::endl;
            return;
        }

        auto distributor = reinterpret_cast< MessageDistributor* >( proto );

        std::vector< uint8_t > buffer;
        buffer.resize( size + 3 * sizeof ( int ) );
        as< int >( buffer.data() ) = address;
        as< int >( buffer.data() + sizeof( int ) ) = stamp;
        as< int >( buffer.data() + 2 * sizeof( int ) ) = size;
        std::memcpy( buffer.data() + 3 * sizeof( int ), data, size );

        distributor->sendMessage( _my_address, METHOD_ID, sender_stamp, buffer.data(), size + 3 * sizeof ( int ) );
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

    void onMemoryReplicationFollower( uint8_t* data, unsigned int offset, unsigned int size )
    {
        int address = as< int >( data + offset );
        int stamp = as< int >( data + offset + sizeof( int ) );
        int data_size = as< int >( data + offset + 2 * sizeof( int ) );
        saveData( address, stamp, data + offset + 3 * sizeof( int ), data_size );
        _sender.sendMessage( DistributionMessageType::DataStorageSuccess, _leader );
    }

    void onMemoryReplicationLeader( uint8_t* data, unsigned int offset, unsigned int size )
    {
        int address = as< int >( data + offset );
        int stamp = as< int >( data + offset + sizeof( int ) );
        int data_size = as< int >( data + offset + 2 * sizeof( int ) );
        auto current_value = _storage.find( address );
        if ( current_value != _storage.end() && current_value->second.stamp >= stamp )
        {
            return;
        }
        saveData( address, stamp, data + offset + 3 * sizeof( int ), data_size );
        // Distribute to followers
        sendDataThroughHelper( address, data + 3 * sizeof( int ) + offset, data_size, stamp + 1 );
        // Await responses from followers
    }

public:
    ReplicatedMemoryManager( NetworkManager& netManager, MessageDistributor* distributor, Ip6Addr& myAddress, MessageSender& sender )
    : _net_manager( netManager ), _my_address( myAddress ), _leader( myAddress ), _sender( sender )
    {
        distributor->registerMethod( METHOD_ID, 
            [ this ] ( Ip6Addr sender, unsigned int offset, uint8_t* data, unsigned int size ) 
            {
                onStorageMessage( sender, data, offset, size ); 
            },
            [] () { return; } );
    }

    virtual void setLeader( Ip6Addr leader ) override
    {
        _leader = leader;
    }

    void onStorageMessage( Ip6Addr sender, uint8_t* data, unsigned int offset, unsigned int size )
    { 
        if ( _leader == _my_address )
        {
            return onMemoryReplicationLeader( data, offset, size );
        }

        onMemoryReplicationFollower( data, offset, size );
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

        // Address, Stamp, Size, Data
        PBuf packet = PBuf::allocate( 3 * sizeof( int ) + size );
        as< int >( packet.payload() ) = address;
        as< int >( packet.payload() + sizeof( int ) ) = timestamp;
        as< int >( packet.payload() + 2 * sizeof( int ) ) = size;
        std::memcpy( packet.payload() + 3 * sizeof( int ), data, size );
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
};