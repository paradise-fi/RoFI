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
    class LRHelper : public Protocol {
        using BroadcastRecord = std::tuple< Ip6Addr, unsigned int, unsigned int >;
        
        std::vector< std::reference_wrapper< const Interface > > _managedInterfaces;
        std::vector< std::pair< ConfigAction, ConfigChange > > _configChanges;
        
        std::vector< BroadcastRecord > _history;
        
        const Ip6Addr& _myAddr;
        std::function< void( Ip6Addr, unsigned int ) > _messageReceived;
        std::function< void( ) > _connectionChanged;
        Interface::Name _sender;

        void _eraseOldHistory( Ip6Addr& addr, unsigned int seqNum ) {
            for ( auto i = _history.begin(); i != _history.end(); ) {
                if ( std::get< 0 >( *i ) == addr && std::get< 2 > ( *i ) <= seqNum - 1 ) {
                    i = _history.erase( i );
                } else {
                    ++i;
                }
            }
        }

    public:
        LRHelper( const Ip6Addr& myAddr, std::function< void( Ip6Addr, unsigned int ) > msgCb,
                  std::function< void() > connCb ) : _myAddr( myAddr ), _messageReceived( msgCb ), _connectionChanged( connCb ){}

        virtual bool onMessage( const std::string& interfaceName,
                                rofi::hal::PBuf packetWithHeader ) {
            auto packet = PBuf::own( pbuf_free_header( packetWithHeader.release(), IP6_HLEN ) );

            Ip6Addr senderAddr = as< Ip6Addr >( packet.payload() );
            unsigned int logTime = as< unsigned int >( packet.payload() + Ip6Addr::size() );
            unsigned int seqNum = as< unsigned int >( packet.payload() + Ip6Addr::size()  + sizeof( unsigned int ) );
            auto res = std::find_if( _history.begin(), _history.end(),
                                    [ senderAddr, logTime, seqNum ]( BroadcastRecord record ) {
                                        return senderAddr == std::get< 0 >( record ) 
                                               && logTime == std::get< 1 >( record )
                                               &&  seqNum == std::get< 2 >( record );
                                    } );

            if ( res != _history.end() || senderAddr == _myAddr ) {
                return false;
            }

            _sender = interfaceName;
            if ( seqNum >= 2 ) {
                _eraseOldHistory( senderAddr, seqNum );
            }

            _history.emplace_back( BroadcastRecord{ senderAddr, logTime, seqNum } );
            _messageReceived( senderAddr, logTime );
            _configChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
            return true;
        }

        virtual bool afterMessage( const Interface& interface, std::function< void ( PBuf&& ) > fun, void* /* args */) {
            // Nothing to send.
            if ( _history.size() == 0 ) {
                return false;
            }

            if ( interface.name() == _sender ) {
                _sender = "rl0";
                return false;
            }

            if ( ! const_cast< Interface& >( interface ).isConnected() ) {
                return false;
            }

            BroadcastRecord message = _history.back();
            PBuf packet = PBuf::allocate( Ip6Addr::size() + 2 * sizeof( unsigned int ) );
            as< Ip6Addr >( packet.payload() ) = std::get< 0 >( message );
            as< unsigned int >( packet.payload() + Ip6Addr::size() ) = std::get< 1 >( message );
            as< unsigned int >( packet.payload() + Ip6Addr::size() + sizeof( unsigned int ) ) = std::get< 2 >( message );
            fun( std::move( packet ) );
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
            _connectionChanged();
            return false;
        }

        virtual bool manages( const Interface& interface ) const override {
            return std::ranges::any_of( _managedInterfaces, [ &interface ]( const Interface& i ) {
                return interface == i;
            } );
        }

        virtual Ip6Addr address() const override { return Ip6Addr( "ff02::ea:ea" ); }

        virtual std::string name() const override { return "lr-helper"; }

        virtual std::string info() const override {
            std::string str = Protocol::info();
            std::stringstream ss;
            ss << "; module address: " << _myAddr;
            ss << "; This protocol is used for leader election functionality provided by LRElection.";
            return str  + ss.str();
        }
    };
}