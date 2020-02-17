#pragma once

#include <lwip/def.h>
#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/dhcp.h>

#include <algorithm>
#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream>

#include "dock.hpp"

namespace _rofi {

// C++ wrapper for lwIP addr
struct Ip4Addr : ip4_addr {
    Ip4Addr( const char* str ) {
        addr = ipaddr_addr( str );
    }
    Ip4Addr( ip4_addr addr ): ip4_addr( addr ) {}
    Ip4Addr( uint8_t a, uint8_t b, uint8_t c, uint8_t d ) {
        addr = uint32_t( a ) << 24 |
                uint32_t( b ) << 16 |
                uint32_t( c ) << 8  |
                uint32_t( d )       ;
    }

    bool operator==( const Ip4Addr& o ) const { return addr == o.addr; }

    static int size() { return 4; }
};

struct PhysAddr {
    PhysAddr( uint8_t *a ) {
        std::copy_n( a, 6, addr );
    }
    PhysAddr( uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f ) {
        addr[ 0 ] = a; addr[ 1 ] = b; addr[ 2 ] = c;
        addr[ 3 ] = d; addr[ 4 ] = e; addr[ 5 ] = f;
    }
    PhysAddr( const PhysAddr& ) = default;
    PhysAddr& operator=( const PhysAddr& ) = default;

    static int size() { return 6; }

    uint8_t addr[ 6 ];
};

inline std::ostream& operator<<( std::ostream& o, Ip4Addr a ) {
    const char* sep = "";
    for ( int i = 0; i != 4; i++ ) {
        int byte = ( a.addr >> ( 8 * i ) ) & 0xFF;
        o << sep << byte;
        sep = ".";
    }
    return o;
}

class AddressMapping {
public:
    static const int ContentType = 1;
    enum class Command : uint8_t { Call = 0, Response = 1 };

    AddressMapping( const Ip4Addr& addr, const PhysAddr& guid )
        : _self{ addr, guid }
    {}

    struct Entry {
        Ip4Addr addr;
        PhysAddr guid;

        static int size() { return Ip4Addr::size() + PhysAddr::size(); }
    };

    void onPacket( Dock& dock, const PBuf& packet ) {
        if ( packet.size() < 1 )
            return;
        auto command = as< Command >( packet.payload() );
        if ( command == Command::Call )
            sendResponse( dock );
        else if ( command == Command::Response )
            _parseResponse( dock, packet );
    }

    void sendResponse( Dock& dock ) {
        auto p = PBuf::allocate( 4 +  Entry::size() );
        as< Command >( p.payload() + 0 ) = Command::Response;
        as< uint8_t >( p.payload() + 1 ) = Ip4Addr::size();
        as< uint8_t >( p.payload() + 2 ) = PhysAddr::size();
        as< uint8_t >( p.payload() + 3 ) = 1;
        auto dataField = p.payload() + 4;
        as< Ip4Addr >( dataField ) = _self.addr;
        as< PhysAddr >( dataField + Ip4Addr::size() ) = _self.guid;
        dataField += Ip4Addr::size() + PhysAddr::size();
        dock.sendBlob( ContentType, std::move( p ) );
    }

    void sendCall( Dock& dock ) {
        auto p = PBuf::allocate( 1 );
        as< Command >( p.payload() ) = Command::Call;
        dock.sendBlob( ContentType, std::move( p ) );
    }

    Dock *destination( Ip4Addr add ) {
        for ( const auto& e : _entries )
            if ( e.first.addr == add ) return e.second;
        return nullptr;
    }

    const Entry& self() const { return _self; }

private:
    void _parseResponse( Dock& d, const PBuf& packet ) {
        if ( packet.size() < 4 )
            return;
        if ( as< uint8_t >( packet.payload() + 1 ) != Ip4Addr::size() )
            return;
        if ( as< uint8_t >( packet.payload() + 2 ) != PhysAddr::size() )
            return;
        int count = as< uint8_t >( packet.payload() + 3 );
        if ( packet.size() != 4 + count * Entry::size() )
            return;
        _removeEntriesFor( d );
        for ( int i = 0; i != count; i++ ) {
            const uint8_t *entry = packet.payload() + 4 + i * Entry::size();
            Ip4Addr lAddr = as< Ip4Addr >( entry );
            PhysAddr pAddr = as< PhysAddr >( entry + Ip4Addr::size() );
            _entries.push_back( { { lAddr, pAddr }, &d } );
        }
    }

    int _entriesFor( Dock& d ) {
        return std::count_if( _entries.begin(), _entries.end(), [&]( const auto& e ) {
            return e.second == &d;
        } );
    }

    template < typename Yield >
    void _forEachDockEntry( Dock& d, Yield yield ) {
        for ( const auto& e : _entries )
            if ( e.second == &d ) yield( e.first );
    }

    void _removeEntriesFor( Dock& d ) {
        auto end = std::remove_if( _entries.begin(), _entries.end(), [&]( const auto& e ){
            return e.second == &d;
        } );
        _entries.erase( end, _entries.end() );
    }

    std::vector< std::pair< Entry, Dock * > > _entries;
    Entry _self;
};


// RoFi Network Interface
class RoIF {
public:
    RoIF( PhysAddr pAddr, Ip4Addr ip, Ip4Addr mask, Ip4Addr gateway,
        std::vector< gpio_num_t > dockCs, spi_host_device_t spiHost = HSPI_HOST )
    : _mask( mask ), _gateway( gateway ), _mapping( ip, pAddr )
    {
        _docks.reserve( dockCs.size() );
        for ( auto cs : dockCs ) {
            _docks.emplace_back( spiHost, cs );
            _docks.back().onReceive( [this]( Dock& dock, int contentType, PBuf&& data ) {
                _onPacket( dock, contentType, std::move( data ) );
            } );
        }
        netif_add( &_netif, &_mapping.self().addr, &_mask, &_gateway, this, init,
            tcpip_input );
        dhcp_stop( &_netif );

        // Proactivelly send mapping responses
        _mappingTimer = rtos::Timer( 1000 / portTICK_PERIOD_MS, rtos::Timer::Type::Periodic,
            [this]() {
                for ( auto& d : _docks )
                    _mapping.sendResponse( d );
            } );
        _mappingTimer.start();
    }

    void setUp() {
        netif_set_default( &_netif );
        netif_set_up( &_netif );
    }

    static err_t init( struct netif* roif ) {
        auto self = reinterpret_cast< RoIF* >( roif->state );
        roif->hwaddr_len = 6;
        std::copy_n( self->_mapping.self().guid.addr, 6, roif->hwaddr );
        roif->mtu = 120;
        roif->name[ 0 ] = 'r'; roif->name[ 1 ] = 'o';
        roif->output = output;
        roif->linkoutput = linkOutput;
        roif->flags |= NETIF_FLAG_LINK_UP;
        roif->num = _seqNum();
        return ERR_OK;
    }

    static err_t output( struct netif* roif, struct pbuf *p,
        const ip4_addr_t *ipaddr )
    {
        auto self = reinterpret_cast< RoIF* >( roif->state );
        self->_send( Ip4Addr( *ipaddr ), PBuf::reference( p ) );
        return ERR_OK;
    }

    static err_t linkOutput( struct netif* roif, struct pbuf *p ) {
        assert( false && "Link output should not be directly used" );
        return ERR_OK;
    }
private:
    static int _seqNum() {
        static int num = 0;
        return num++;
    }

    void _onPacket( Dock& dock, int contentType, PBuf&& packet ) {
        if ( contentType == 0 ) {
            if ( !netif_is_up( &_netif ) )
                return;
            if ( _netif.input( packet.get(), &_netif ) == ERR_OK )
                packet.release();
        } else if ( contentType == AddressMapping::ContentType ) {
            _mapping.onPacket( dock, packet );
        }
    }

    void _send( Ip4Addr addr, PBuf&& packet ) {
        Dock* d = _mapping.destination( addr );
        if ( d )
            d->sendBlob( 0, std::move( packet ) );
    }

    Ip4Addr _mask, _gateway;
    netif _netif;
    std::vector< Dock > _docks;

    AddressMapping _mapping;
    rtos::Timer _mappingTimer;
};

} // namespace _rofi