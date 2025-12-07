#pragma once

#include <atomic>
#include <lwip++.hpp>
#include <networking/protocol.hpp>
#include <networking/interface.hpp>
#include <networking/routingTable.hpp>
#include <networking/networkManager.hpp>

#include <atoms/util.hpp>

#include <vector>
#include <map>
#include <set>
#include <utility>

namespace rofi::net {
    struct DistributorRecord 
    {
        Ip6Addr sender_address;
        unsigned int method_id;
        unsigned int stamp;

        bool operator==(const DistributorRecord& other)
        {
            return sender_address == other.sender_address
                && method_id == other.method_id
                && stamp == other.stamp;
        }
    };

    struct DistributorMethodRegistry
    {
        std::function< void( Ip6Addr sender_addr, uint8_t* data, unsigned int size ) > messageReceived;
        std::function< void( ) > connectionChanged;
        std::atomic< unsigned int > timestamp = 0;

        DistributorMethodRegistry(std::function< void( Ip6Addr sender_addr, uint8_t* data, unsigned int size ) > messageReceived,
            std::function< void( ) > connectionChanged) 
            : messageReceived( messageReceived ), connectionChanged( connectionChanged ) {}

        DistributorMethodRegistry( const DistributorMethodRegistry& other )
            : messageReceived( other.messageReceived ), connectionChanged( other.connectionChanged ), timestamp( other.timestamp.load() ) {}
        DistributorMethodRegistry& operator=( const DistributorMethodRegistry& other ) 
        {
            if ( this == &other )
            {
                return *this;
            }

            messageReceived = other.messageReceived;
            connectionChanged = other.connectionChanged;
            timestamp.store( other.timestamp.load() );
        }

        DistributorMethodRegistry( DistributorMethodRegistry&& ) = default;
        DistributorMethodRegistry& operator=( DistributorMethodRegistry&& ) = default;
    };

    class MessageDistributor : public Protocol {
        using methodId = unsigned int;
        using timestamp = unsigned int;

        std::vector< std::reference_wrapper< const Interface > > _managedInterfaces;
        std::vector< std::pair< ConfigAction, ConfigChange > > _configChanges;
        
        std::vector< DistributorRecord > _history;
        std::map< methodId, DistributorMethodRegistry > _registry; 
        const Ip6Addr& _myAddr;
        NetworkManager& _manager;

        unsigned int _stampClearTreshold = 5;

        void _eraseOldMethodHistory( Ip6Addr& addr, unsigned int method_id, unsigned int stamp ) {
            for ( auto record_it = _history.begin(); record_it != _history.end(); ) {
                if ( record_it->sender_address == addr
                  && record_it->stamp <= stamp - 1
                  && record_it->method_id == method_id ) {
                    record_it = _history.erase( record_it );
                } else {
                    ++record_it;
                }
            }
        }

        void _sendMessagesInner( uint8_t* data, int size, const std::string& noSendInterfaceName )
        {
            for ( const Interface& interface : _manager.interfaces() )
            {
                if ( interface.name() == "rl0" || interface.name() == noSendInterfaceName )
                {
                    continue;
                }
                PBuf packet = PBuf::allocate( size );
                std::memcpy( packet.payload(), data, size );
                if ( !const_cast< Interface& >( interface ).sendProtocol( address(), std::move( packet ) ) ) {
                    assert( false && "failed to send a message to the helper protocol. Something went wrong." );
                }
            }
        }


    public:
        MessageDistributor( const Ip6Addr& myAddr, NetworkManager& manager, unsigned int stampClearTreshold = 5 ) 
        : _myAddr( myAddr ), _manager( manager ), _stampClearTreshold( stampClearTreshold ) {}

        bool sendMessage( Ip6Addr sender, unsigned int method_id, uint8_t* data, size_t data_size )
        {
            const auto& entry = _registry.find( method_id );

            if ( entry == _registry.end() )
            {
                return false;
            }

            unsigned int timestamp = entry->second.timestamp.fetch_add( 1 );

            for ( const Interface& interface : _manager.interfaces() )
            {
                if ( interface.name() == "rl0" )
                {
                    continue;
                }

                PBuf packet = PBuf::allocate( Ip6Addr::size() + static_cast< int >( sizeof( unsigned int ) ) * 2 + static_cast< int >( data_size ) );
                as< Ip6Addr >( packet.payload() ) = sender;
                as< unsigned int >( packet.payload() + Ip6Addr::size() ) = method_id;
                as< unsigned int >( packet.payload() + Ip6Addr::size() + sizeof( unsigned int ) ) = timestamp;
                std::memcpy( packet.payload() + Ip6Addr::size() + sizeof( unsigned int ) * 2,
                             data, data_size );

                if ( !const_cast< Interface& >( interface ).sendProtocol( address(), std::move( packet ) ) ) 
                {
                    assert( false && "failed to send a message to the helper protocol. Something went wrong." );
                }           
            }

            return true;
        }

        bool registerMethod(methodId method_id, 
            std::function< void( Ip6Addr sender_addr, uint8_t* data, unsigned int size ) > onMessage,
            std::function< void( ) > connectionChanged )
        {
            if ( _registry.find( method_id ) != _registry.end() )
            {
                return false;
            }

            _registry.emplace( method_id, DistributorMethodRegistry( onMessage, connectionChanged ) );
            return true;
        }

        void unregisterMethod( unsigned int method_id )
        {
            _registry.erase( method_id );
        }

        virtual bool onMessage( const std::string& interfaceName,
                                rofi::hal::PBuf packetWithHeader ) {
            auto packet = PBuf::own( pbuf_free_header( packetWithHeader.release(), IP6_HLEN ) );
            Ip6Addr senderAddr = as< Ip6Addr >( packet.payload() );
            unsigned int method_id = as< unsigned int >( packet.payload() + Ip6Addr::size() );
            unsigned int stamp = as< unsigned int >( packet.payload() + Ip6Addr::size()  + sizeof( unsigned int ) );
            
            auto res = std::find_if( _history.begin(), _history.end(),
                                    [ senderAddr, method_id, stamp ]( DistributorRecord& record ) {
                                        return senderAddr == record.sender_address
                                            && method_id == record.method_id
                                            && stamp == record.stamp;
                                    } );
            
            if ( res != _history.end() || senderAddr == _myAddr )
            {
                return false;
            }

            const auto& methodRegistryEntry = _registry.find( method_id );

            if ( methodRegistryEntry == _registry.end() )
            {
                return false;
            }
            
            if ( stamp >= _stampClearTreshold )
            {
                _eraseOldMethodHistory( senderAddr, method_id, stamp );
                // Reset the timestamp.
                methodRegistryEntry->second.timestamp.store( 0 );
            }
            else
            {
                unsigned int timestamp = methodRegistryEntry->second.timestamp.load();

                if (stamp > timestamp )
                {
                    methodRegistryEntry->second.timestamp.fetch_add(stamp + 1);
                }
            }
            
            _history.emplace_back( DistributorRecord{ senderAddr, method_id, stamp } );
            
            unsigned int header_size = Ip6Addr::size()  + sizeof( unsigned int ) * 2; 
            methodRegistryEntry->second.messageReceived( senderAddr, packet.payload() + header_size, packet.size() - header_size ); 
            
            _sendMessagesInner( packet.payload(), packet.size(), interfaceName );
            
            return true;
        }

        virtual bool afterMessage( const Interface&, std::function< void ( PBuf&& ) >, void* /* args */) {
            return false;
        }

        virtual bool hasConfigUpdates() const override { return !_configChanges.empty(); }

        virtual std::vector< std::pair< ConfigAction, ConfigChange > > getConfigUpdates() const {
            return _configChanges;
        }

        virtual void clearUpdates() { 
            _configChanges.clear(); 
        }

        virtual bool addInterface( const Interface& interface ) {
            if ( manages( interface ) ) {
                return false;
            }

            _managedInterfaces.push_back( std::reference_wrapper( interface ) );
            return false;
        }

        virtual bool removeInterface( const Interface& interface ) {
            auto it = std::find_if( _managedInterfaces.begin(), _managedInterfaces.end(), 
                                    [ &interface ]( const auto& i ) { 
                                        return interface == i; 
                                    } );
            if ( it == _managedInterfaces.end() ) {
                return false;
            }

            std::swap( *it, _managedInterfaces.back() );
            _managedInterfaces.pop_back();
            return true;
        }

        virtual bool onInterfaceEvent( const Interface&, bool ) override {
            for ( const auto& callback_pair : _registry )
            {
                callback_pair.second.connectionChanged();
            }
            return false;
        }

        virtual bool manages( const Interface& interface ) const override {
            return std::ranges::any_of( _managedInterfaces, [ &interface ]( const Interface& i ) {
                return interface == i;
            } );
        }

        virtual Ip6Addr address() const override { return Ip6Addr( "ff02::ea:ea" ); }

        virtual std::string name() const override { return "message-distributor"; }

        virtual std::string info() const override {
            std::string str = Protocol::info();
            std::stringstream ss;
            ss << "; module address: " << _myAddr;
            ss << "; This protocol is used for distributing messages regardless of address, a sort of multicast-broadcast.";
            return str  + ss.str();
        }
    };
}