#pragma once

#include <lwip++.hpp>
#include <networking/protocol.hpp>
#include <networking/interface.hpp>
#include <networking/routingTable.hpp>

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

    struct DistributorCallbacks
    {
        std::function< void( Ip6Addr sender_addr, unsigned int offset, uint8_t* data, unsigned int size ) > messageReceived;
        std::function< void( ) > connectionChanged;

        DistributorCallbacks(std::function< void( Ip6Addr sender_addr, unsigned int offset, uint8_t* data, unsigned int size ) > messageReceived,
            std::function< void( ) > connectionChanged) : messageReceived( messageReceived ), connectionChanged( connectionChanged ) {}
    };

    class MessageDistributor : public Protocol {
        using methodId = unsigned int;
        using timestamp = unsigned int;

        std::vector< std::reference_wrapper< const Interface > > _managedInterfaces;
        std::vector< std::pair< ConfigAction, ConfigChange > > _configChanges;
        
        std::vector< DistributorRecord > _history;
        std::map< methodId, DistributorCallbacks > _callbacks; 
        const Ip6Addr& _myAddr;
        NetworkManager& _manager;

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

        void _sendMessagesInner( uint8_t* data, int size, unsigned int method_id, const std::string& noSendInterfaceName )
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
        MessageDistributor( const Ip6Addr& myAddr, NetworkManager& manager ) 
        : _myAddr( myAddr ), _manager( manager ) {}

        void sendMessage( Ip6Addr sender, unsigned int method_id, unsigned int time_stamp, uint8_t* data, int data_size )
        {
            for ( const Interface& interface : _manager.interfaces() )
            {
                if ( interface.name() == "rl0" )
                {
                    continue;
                }

                PBuf packet = PBuf::allocate( Ip6Addr::size() + sizeof( unsigned int ) * 2 + data_size );
                as< Ip6Addr >( packet.payload() ) = sender;
                as< unsigned int >( packet.payload() + Ip6Addr::size() ) = method_id;
                as< unsigned int >( packet.payload() + Ip6Addr::size() + sizeof( unsigned int ) ) = time_stamp;
                std::memcpy( packet.payload() + Ip6Addr::size() + sizeof( unsigned int ) * 2,
                            data, data_size );

                if ( !const_cast< Interface& >( interface ).sendProtocol( address(), std::move( packet ) ) ) {
                    assert( false && "failed to send a message to the helper protocol. Something went wrong." );
                }           
            }
        }

        // PBuf&& wrapMessage( Ip6Addr sender, unsigned int method_id, unsigned int time_stamp, PBuf&& data )
        // {
        //     std::cout << "PBuf&& wrap" << std::endl;
        //     PBuf packet = PBuf::allocate( Ip6Addr::size() + sizeof( unsigned int ) * 2 + data.size() );
        //     as< Ip6Addr >( packet.payload() ) = sender;
        //     as< unsigned int >( packet.payload() + Ip6Addr::size() ) = method_id;
        //     as< unsigned int >( packet.payload() + Ip6Addr::size() + sizeof( unsigned int ) ) = time_stamp;
        //     std::memcpy( packet.payload() + Ip6Addr::size() + sizeof( unsigned int ) * 2,
        //                  data.payload(), data.size() );
        //     return std::move( packet );
        // }

        // PBuf&& wrapMessage( Ip6Addr sender, unsigned int method_id, unsigned int time_stamp, uint8_t* data, int data_size )
        // {
        //     std::cout << "uint8_t* wrap" << std::endl;
        //     PBuf packet = PBuf::allocate( Ip6Addr::size() + sizeof( unsigned int ) * 2 + data_size );
        //     as< Ip6Addr >( packet.payload() ) = sender;
        //     as< unsigned int >( packet.payload() + Ip6Addr::size() ) = method_id;
        //     as< unsigned int >( packet.payload() + Ip6Addr::size() + sizeof( unsigned int ) ) = time_stamp;
        //     std::memcpy( packet.payload() + Ip6Addr::size() + sizeof( unsigned int ) * 2,
        //                  data, data_size );
        //     return std::move( packet );
        // }
        
        bool registerMethod(unsigned int method_id, 
            std::function< void( Ip6Addr sender_addr, unsigned int offset, uint8_t* data, unsigned int size ) > onMessage,
            std::function< void( ) > connectionChanged )
        {
            if ( _callbacks.find( method_id ) != _callbacks.end() )
            {
                return false;
            }

            _callbacks.emplace( method_id, DistributorCallbacks( onMessage, connectionChanged ) );
            return true;
        }

        void unregisterMethod( unsigned int method_id )
        {
            _callbacks.erase( method_id );
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
            
            if ( res != _history.end() || senderAddr == _myAddr ) {
                return false;
            }

            const auto& callbacks = _callbacks.find( method_id );
            if ( callbacks == _callbacks.end() )
            {
                return false;
            }

            if ( stamp >= 2 ) {
                _eraseOldMethodHistory( senderAddr, method_id, stamp );
            }

            _history.emplace_back( DistributorRecord{ senderAddr, method_id, stamp } );
            callbacks->second.messageReceived( senderAddr, Ip6Addr::size()  + sizeof( unsigned int ) * 2, packet.payload(), packet.size() ); 
            _sendMessagesInner( packet.payload(), packet.size(), method_id, interfaceName );
            return true;
        }

        virtual bool afterMessage( const Interface& interface, std::function< void ( PBuf&& ) > fun, void* /* args */) {
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
            for ( const auto& callback_pair : _callbacks )
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