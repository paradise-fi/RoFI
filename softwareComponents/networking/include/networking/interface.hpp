#pragma once

#include "lwip++.hpp"
#include "rofi_hal.hpp"

#include <lwip/mld6.h>
#include <lwip/raw.h>
#include <lwip/ip6_addr.h>
#include <lwip/tcpip.h>
#include <networking/logger.hpp>

#include <atoms/unreachable.hpp>

#include <optional>
#include <list>
#include <functional>
#include <map>


namespace rofi::net {

// TODO: Maybe don't expose it like this
using PBuf      = hal::PBuf;
using Ip6Addr   = hal::Ip6Addr;
using Netif     = hal::Netif;
using PhysAddr  = hal::PhysAddr;
using Connector = hal::Connector;

/**
 * \brief Class representing the interface of a Module. Supports both, physical (i.e., with
 * a physical connector) and virtual flavours.
*/
class Interface {
    Netif _netif;
    std::vector< std::pair< int, uint8_t > > _addrIndexWithMask;
    PhysAddr _paddr;
    std::map< Ip6Addr, raw_pcb* > _pcbs;
    std::optional< Connector > _connector;
    long long unsigned _sent = 0;
    long long unsigned _received = 0;
    Logger::LogFunction _logger;

    std::list< std::function< uint8_t (void*, raw_pcb*, pbuf*, const ip6_addr*) > > _onMessageStorage;

    static uint8_t uniqueID() {
        static uint8_t id = 0;
        return id++;
    }

    static err_t linkOutput( struct netif*, struct pbuf* ) {
        assert( false && "linkOutput should no be used" );
        return ERR_OK;
    }

    static err_t output( struct netif* n, struct pbuf* p, const ip6_addr_t* ip ) {
        auto* interface = reinterpret_cast< Interface* >( n->state );
        if ( !interface )
            return ERR_ARG;

        if ( interface->isVirtual() ) {
            auto [ addr, _ ] = interface->getAddress().front();
            auto* gateway = ip_find_route( ip, &addr );
            if ( gateway ) {
                interface->_sent += p ? p->tot_len : 0;
                return ip6_output_if_src( p, &addr, LWIP_IP_HDRINCL, 0, 0, 0, gateway );
            }
            return ERR_RTE;
        }
        // physical interface = interface with a connector
        interface->_sent += p ? p->tot_len : 0;
        interface->send( std::move( PBuf::reference( p ) ), 0 );

        return ERR_OK;
    }

    static err_t init( struct netif* n ) {
        auto* interface = reinterpret_cast< Interface* >( n->state );
        if ( interface == nullptr )
            throw std::runtime_error( "netif init function has null state" );
        n->hwaddr_len = 6;
        std::copy_n( interface->_paddr.addr, 6, n->hwaddr );
        n->mtu = 120; n->mtu6 = 120;
        n->name[ 0 ] = 'r'; n->name[ 1 ] = interface->isVirtual() ? 'l' : 'd';
        n->num = uniqueID(); // lwip will overwrite this if any netif with the same number exists
        n->output_ip6 = output;
        n->linkoutput = linkOutput;
        n->flags = NETIF_FLAG_IGMP | NETIF_FLAG_MLD6 | NETIF_FLAG_LINK_UP;
        interface->log( Logger::Level::Info, "Initialized successfully" );
        return ERR_OK;
    }

    static err_t my_input( struct pbuf* p, struct netif* n ) {
        if ( n ) {
            auto* interface = reinterpret_cast< Interface* >( n->state );
            if ( interface ) {
                interface->_received += p ? p->tot_len : 0;
            }
        }

        return tcpip_input( p, n ); 
    }

    static uint8_t onMessageDumb( void* arg, raw_pcb* pcb, pbuf* p, const ip_addr_t* addr ) {
        auto* f = reinterpret_cast< std::function< uint8_t( void*, raw_pcb*, pbuf*, const ip6_addr_t* ) >* >( arg );
        if ( f )
            return ( *f )( nullptr, pcb, p, addr );
        return 0;
    }

    template< typename OnMessage, typename = std::is_invocable_r< err_t, OnMessage, raw_pcb*, pbuf*, const ip_addr_t* > >
    void createListener( const Ip6Addr& listenerAddr, OnMessage& onMessage ) {
        raw_pcb* ptr = raw_new_ip_type( IPADDR_TYPE_V6, IP6_NEXTH_ICMP6 );
        assert( ptr && "raw_new_ip6 failed, obtained pcb ptr is null" );

        _pcbs[ listenerAddr ] = ptr;
        raw_bind_netif( ptr, &_netif );
        raw_recv( ptr, onMessageDumb, &onMessage /* <-- arg in onMessage */ );
        auto res = mld6_joingroup_netif( &_netif, &listenerAddr );
        if ( res != ERR_OK ) {
            log( Logger::Level::Warn, std::string( "Error - failed to joint mld6 group: " ) + lwip_strerr( res ) );
        } else {
            log( Logger::Level::Info
               , std::string( "Protocol listener set-up on " ) + Logger::toString( listenerAddr ) );
        }
        // TODO: return bool?
    }

    void log( Logger::Level l, const std::string& msg ) {
        if ( _logger ) {
            _logger( l, name(), msg );
        }
    }

public:
    /// Type alias for the Interface's name. Use this instead of string, it may change in the future.
    using Name = std::string;

    template< typename NetmgCB, typename = std::is_invocable_r< void, NetmgCB, const Interface*, hal::ConnectorEvent > >
    Interface( const PhysAddr& pAddr, Logger::LogFunction lf
             , std::optional< Connector > connector
             , const NetmgCB& networkManagerEventCB )
    : _paddr( pAddr ), _connector( connector ), _logger( lf )
    {
        netif_add_noaddr( &_netif, this, init, my_input );
        netif_create_ip6_linklocal_address( &_netif, isVirtual() ? 0 : 1 );
            
        if ( isVirtual() ) {
            netif_set_default( &_netif );
        } else {
            _connector->onPacket( [ this ]( Connector, uint16_t contentType, PBuf&& p ) {
                // TODO: Rework and support adding/removing callbacks based on contentType!
                if ( contentType == 0 ) {
                    _netif.input( p.release(), &_netif );
                } else {
                    log( Logger::Level::Debug, "message with a different content type than zero" );
                }
            } );

            _connector->onConnectorEvent( [ this, networkManagerEventCB ]( hal::Connector, hal::ConnectorEvent e ) {
                if ( e == hal::ConnectorEvent::Connected || e == hal::ConnectorEvent::Disconnected ) {
                    networkManagerEventCB( this, e );
                }
            } );
        }
    }

    ~Interface() { netif_remove( &_netif ); log( Logger::Level::Debug, "Shutting down..." ); }
    Interface( const Interface& ) = delete;
    Interface& operator=( const Interface& ) = delete;

    /**
     * \brief Set the interface up so it starts processing traffic.
     * 
     * \return Returns @true if the interface was set up correctly.
    */
    bool setUp() {
        if ( !isConnected() && !isVirtual() )
            return false;

        netif_set_up( &_netif );
        log( Logger::Level::Info, "Setting up..." );
        return true;
    }

    /**
     * \brief Set the interface down so it stops processing traffic.
    */
    void setDown() {
        log( Logger::Level::Info, "Setting down..." );
        netif_set_down( &_netif );
    }

    /**
     * \brief Query on the state of the interface.
     * 
     * \return Returns @true if the interface is up.
    */
    bool isUp() const {
        return _netif.flags | NETIF_FLAG_UP;
    }

    /**
     * \brief Query on the state of the interface.
     * 
     * \return Returns @true if the interface is down.
    */
    bool isDown() const {
        return !isUp();
    }

    /**
     * \brief Query on the state of the underlying connector.
     * 
     * \return Returns @true if the connector of the interface is connected. For virtual interface
     * this returns always @false.
    */
    bool isConnected() { return _connector.has_value() && _connector->getState().connected; }

    /**
     * \brief Returns @true if the interface is virtual (i.e., does not have a physical connector).
    */
    bool isVirtual() const { return !_connector.has_value(); }

    /**
     * \brief Set up a protocol listener on given address with given callback function.
     * 
     * \return Returns a handle type unique for every protocol.
    */
    template< typename OnMessage, typename = std::is_invocable_r< err_t, OnMessage, raw_pcb*, pbuf*, const ip_addr_t* > >
    void setProtocol( const Ip6Addr& listenerAddr, OnMessage&& onMessage ) {
        if ( _pcbs.contains( listenerAddr ) && _pcbs[ listenerAddr ] )
            throw std::runtime_error( "Protocol is already set on interface " + name() + " for address " + Logger::toString( listenerAddr ) );

        _onMessageStorage.push_back( std::forward< OnMessage >( onMessage ) );
        createListener( listenerAddr, _onMessageStorage.back() );
        // TODO: Maybe return bool
    }

    /**
     * \brief Remove protocol corresponding to the given handle. Stops corresponding listener.
    */
    void removeProtocol( const Ip6Addr& listenerAddr ) {
        if ( _pcbs.contains( listenerAddr ) ) {
            raw_remove( _pcbs[ listenerAddr ] );
            _pcbs.erase( listenerAddr );
        }
    }

    /**
     * \brief Sends a message onto the given address using the protocol corresponding to the handle.
     * 
     * \return Returns @true if the message was sent correctly.
    */
    bool sendProtocol( const Ip6Addr& listenerAddr, PBuf&& msg ) {
        if ( !_pcbs.contains( listenerAddr ) || !_pcbs[ listenerAddr ] )
            return false;

        auto [ netifIP, _ ] = getAddress().front();
        err_t res = raw_sendto_if_src( _pcbs[ listenerAddr ], msg.release(), &listenerAddr, &_netif, &netifIP );
        if ( res != ERR_OK ) {
            log( Logger::Level::Warn
               , std::string( "sendProtocol: raw send to returned an error: " ) + lwip_strerr( res ) );
        }
        return res == ERR_OK;
    }

    /**
     * \brief Get an address on the given interface.
     * 
     * Number of addresses is determined by the underlying lwip library, namely by LWIP_IPV6_NUM_ADDRESSES macro.
     * 
     * \return Returns a pair – corresponding to IP address and the subnet mask.
    */
    std::pair< Ip6Addr, uint8_t > getAddress( int index ) const {
        for ( auto [ i, mask ] : _addrIndexWithMask ) {
            if ( index == i ) {
                return { *_netif.getAddress( index ), mask };
            }
        }

        throw std::runtime_error( "Address on given index does not exist" );
    }

    // TODO: Check the code, where getAddress().front() is and implement safety mechanisms where it makes sense
    std::vector< std::pair< Ip6Addr, uint8_t > > getAddress() const {
        std::vector< std::pair< Ip6Addr, uint8_t > > res;
        for ( auto [ index, mask ] : _addrIndexWithMask ) {
            res.push_back( { *_netif.getAddress( index ), mask } );
        }

        // push link-local to the end (it has to be always present)
        if ( _netif.getAddress( 0 ).has_value() && _netif.getAddress( 0 )->linkLocal() ) {
            res.push_back( { *_netif.getAddress( 0 ), 64 } );
        }

        return res;
    }

    // TODO
    /**
     * \brief Returns a name of the interface.
    */
    Name name() const { return _netif.getName(); }

    /**
     * \brief Sets the name of the interface. The name has to be in form of two characters followed
     * with up to 3 digits.
    */
    bool setName( const Name& name ) {
        if ( name.length() >= 5 )
            return false;
            
        return _netif.setName( name );
    }

    // TODO
    /**
     * \brief Add given address with the given mask to the interface.
     * 
     * \return Returns @true if the address was correctly added.
    */
    bool addAddress( const Ip6Addr& ip, uint8_t mask ) {
        auto index = _netif.addAddress( ip );
        if ( index == -1 )
            return false;

        _addrIndexWithMask.push_back( { index, mask } );
        log( Logger::Level::Info
           , std::string( "Address " ) + Logger::toString( ip ) + "/"
                                       + Logger::toString( static_cast< int >( mask ) ) + " added" );
        return true;
    }

    /**
     * \brief Removes given address with the given mask to the interface.
     * 
     * \return Returns @true if the address was correctly removed.
    */
    bool removeAddress( const Ip6Addr& ip, uint8_t mask ) {
        int i = 0;
        for ( auto& [ index, addrMask ] : _addrIndexWithMask ) {
            if ( mask == addrMask && _netif.getAddress( index ) == ip ) {
                _netif.removeAddress( index );
                log( Logger::Level::Info
                   , std::string( "Address " ) + Logger::toString( ip ) + "/"
                                               + Logger::toString( static_cast< int >( mask ) ) + " removed" );

                std::swap( _addrIndexWithMask[ i ], _addrIndexWithMask.back() );
                _addrIndexWithMask.pop_back();
                return true;
            }
            i++;
        }

        return false;
    }

    /**
     * \brief Send given packet with the content type via the interface.
    */
    void send( PBuf&& packet, uint16_t contentType ) {
        if ( _connector )
            _connector->send( contentType, std::move( packet ) );
        else
            throw std::runtime_error( "Interface::send called on virtual interface" );
    }

    /**
     * \brief Function that returns the amount of data sent (in bytes).
    */
    long long unsigned dataSent() const { return _sent; }
    /**
     * \brief Function that returns the amount of data received (in bytes).
    */
    long long unsigned dataReceived() const { return _received; }

    friend bool operator==( const Interface& i1, const Interface& i2 );
    friend std::ostream& operator<<( std::ostream& o, const Interface& i );
};

inline bool operator==( const Interface& i1, const Interface& i2 ) {
    return &i1._netif == &i2._netif && i1.name() == i2.name();
}

inline bool operator!=( const Interface& i1, const Interface& i2 ) {
    return !( i1 == i2 );
}

/**
 * \brief Outputs info about the interface.
*/
inline std::ostream& operator<<( std::ostream& o, const Interface& i ) {
    o << "interface " << i.name() << "\n";
    if ( !i.isVirtual() )
        o << "sent: " << i.dataSent() << " bytes / received: " << i.dataReceived() << " bytes\n";
    // link-local address
    if ( i.getAddress( 0 ).first.linkLocal() )
        o  << "\t address " << 0 << ": " << i.getAddress( 0 ).first << " [link-local]\n";
    for ( auto& [ index, addrMask ] : i._addrIndexWithMask ) {
        o << "\t address " << index << ": " << *i._netif.getAddress( index )
                                    << "/" << static_cast< int >( addrMask ) << "\n";
    }

    return o;
}

} // namespace rofi::net
