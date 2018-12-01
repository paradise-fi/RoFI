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

// RoFi Network Interface
class RoIF {
public:
    RoIF( Ip4Addr ip, Ip4Addr mask, Ip4Addr gateway,
        std::vector< gpio_num_t > dockCs, spi_host_device_t spiHost = HSPI_HOST )
    : _ip( ip ), _mask( mask ), _gateway( gateway )
    {
        _docks.reserve( dockCs.size() );
        for ( auto cs : dockCs ) {
            _docks.emplace_back( spiHost, cs );
            _docks.back().onReceive( [this]( Dock& dock, int contentType, PBuf&& data ) {
                _onPacket( dock, contentType, std::move( data ) );
            } );
        }
        netif_add( &_netif, &_ip, &_mask, &_gateway, this, init,
            tcpip_input );
        dhcp_stop( &_netif );
    }

    void setUp() {
        netif_set_default( &_netif );
        netif_set_up( &_netif );
    }

    static err_t init( struct netif* roif ) {
        auto self = reinterpret_cast< RoIF* >( roif->state );
        roif->hwaddr_len = 6;
        std::copy_n( self->_hwAddr.begin(), 6, roif->hwaddr );
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
        assert( false && "Link output should not be directly use" );
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
        }
    }

    void _send( Ip4Addr addr, PBuf&& packet ) {
        _docks[ 0 ].sendBlob( 0, std::move( packet ) );
    }

    std::array< uint8_t, 6 > _hwAddr;
    Ip4Addr _ip, _mask, _gateway;
    netif _netif;
    std::vector< Dock > _docks;
};

} // namespace _rofi