#pragma once

#include <lwip/def.h>
#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/dhcp.h>
#include <lwip/ethip6.h>
#include <lwip/nd6.h>
#include <lwip/mld6.h>

#include <lwip/raw.h>

#include <algorithm>
#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

#include "dock.hpp"
#include "rtable.hpp"


namespace _rofi {
	class RoIF6;

static void ncb(netif* n, u8_t s) {}

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

static int _seqNum() {
    static int num = 0;
    return num++;
}

class Netif {
public:
	Netif( PhysAddr pAddr, gpio_num_t dockCs, spi_host_device_t spiHost, RoutingTable& rt )
	: _pAddr( pAddr ), _dock( spiHost, dockCs ), _rtable( rt ) {
		_dock.onReceive( [ this ]( Dock& dock, int contentType, PBuf&& data ) {
			_onPacket( dock, contentType, std::move( data ) );
		} );

		netif_add( &_netif, NULL, NULL, NULL, this, init, tcpip_input );

		ip6_addr_t ip;
		ip6addr_aton( "ff02::1f", &ip );
		auto res = mld6_joingroup_netif( &_netif , &ip );
		if ( res != ERR_OK )
			std::cout << "Error - failed to join mld6 group: " << lwip_strerr( res ) << std::endl;
		create_rrp_listener();
	}

	void addAddress( Ip6Addr addr, s8_t i ) {
		netif_add_ip6_address( &_netif, &addr, &i );
	}

	void setUp() {
		netif_create_ip6_linklocal_address( &_netif, 0 );
		netif_set_up( &_netif );
	}

	static err_t init( struct netif* n ) {
		auto self = reinterpret_cast< Netif* >( n->state );
		n->hwaddr_len = 6;
		std::copy_n( self->_pAddr.addr, 6, n->hwaddr );
		n->mtu = 120;
		n->name[ 0 ] = 'r'; n->name[ 1 ] = 'd';
		n->num = _seqNum();
		n->output_ip6 = output;
		n->input = netif_input;
		n->linkoutput = linkOutput;
		n->flags |= NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP | NETIF_FLAG_UP | NETIF_FLAG_MLD6;
		nd6_set_cb( n, ncb );
		return ERR_OK;
	}

	static err_t output( struct netif* n, struct pbuf *p, const ip6_addr_t *ipaddr) {
		auto self = reinterpret_cast< Netif* >( n->state );
		self->_send( PBuf::reference( p ) );

		return ERR_OK;
	}

	static err_t linkOutput( struct netif* n, struct pbuf *p ) {
		assert( false && "Link output should not be directly used" );
		return ERR_OK;
	}

	void printAddresses() const {
		std::cout << "Addresses are \n";
		for ( int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++ ) {
			std::cout << "\t" << ipaddr_ntoa( &_netif.ip6_addr[ i ] ) << std::endl;
		}
	}

private:
    void _onPacket( Dock& dock, int contentType, PBuf&& packet ) {
        if ( contentType == 0 ) {
            if ( netif_is_up( &_netif ) ) {
                _netif.input( packet.get(), &_netif );
				packet.release();
			}			
        }
    }

    void _send( PBuf&& packet, int contentType = 0 ) {
		_dock.sendBlob( contentType, std::move( packet ) );
    }

	u8_t static onRRP(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr) {
		auto self = reinterpret_cast< Netif* >( arg );
		if ( self ) {
			p = pbuf_free_header( p, IP6_HLEN );
			self->_rtable.update( PBuf::own( p ), &self->_netif );
			return 1;
		}
		return 0;
	}

	void sendRRP() {
		ip_addr_t ip;
	    ipaddr_aton( "ff02::1f", &ip );
		auto rrp = _rtable.toSendWithoutIf( &_netif );
		raw_sendto_if_src( pcb, rrp.get(), &ip, &_netif, &_netif.ip6_addr[0] ); // use link-local address as source
	}

	void create_rrp_listener() {
		pcb = raw_new_ip6( IP6_NEXTH_ICMP6 );
		if ( !pcb ) 
			assert( false && "raw_new_ip6 failed, pcb is null!");
		raw_bind_netif( pcb, &_netif );
		raw_recv( pcb, onRRP, this );
	}

	struct raw_pcb* pcb;
	PhysAddr _pAddr;
	Dock _dock;
	struct netif _netif;
	RoutingTable& _rtable;

	friend class RoIF6;
};


// RoFi Network Interface for IPv6
class RoIF6 {
public:
	RoIF6( int id, PhysAddr pAddr, std::vector< gpio_num_t > dockCs, spi_host_device_t spiHost = HSPI_HOST )
	: RoIF6( Ip6Addr( _createAddress( id ) ), 128, pAddr, dockCs, spiHost ) {}

	RoIF6( Ip6Addr addr, uint8_t mask, PhysAddr pAddr, std::vector< gpio_num_t > dockCs, spi_host_device_t spiHost = HSPI_HOST )
	{
		netif_set_default( &_netif );
		netif_add( &_netif, NULL, NULL, NULL, this, init, tcpip_input );
		dhcp_stop( &_netif );
		addAddress( addr, mask );

		_netifs.reserve( dockCs.size() );
		for ( auto cs : dockCs ) {
			_netifs.emplace_back( pAddr, cs, spiHost, _rtable );
		}

	}

	bool addAddress( const Ip6Addr& addr, uint8_t mask ) {
		s8_t index = -1;
		netif_add_ip6_address( &_netif, &addr, &index );
		if ( index >= 0 ) {
			_rtable.addRecord( addr, mask, 0, &_netif ); // Cost is zero - it is always loopback.
			return true;
		}
		return false;
	}

	void printAddresses() const {
		int counter = 0;
		std::cout << "Roif6 address is\n";
		for ( int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++ ) {
			std::cout << "\t" << ipaddr_ntoa( &_netif.ip6_addr[ i ] ) << std::endl;
		}
		for ( const auto& n : _netifs ) {
			std::cout << "netif number " << counter++ << std::endl;
			n.printAddresses();
		}
	}

	void setUp() {
		for ( auto& n : _netifs ) {
			n.setUp();
		}

		_mappingTimer = rtos::Timer( 5000 / portTICK_PERIOD_MS, rtos::Timer::Type::Periodic,
            [ this ]() {
                _broadcastRTable();
            } );
        _mappingTimer.start();
	}

	static err_t init( struct netif* roif ) {
        roif->mtu = 120;
        roif->name[ 0 ] = 'r'; roif->name[ 1 ] = 'l';
        roif->num = _seqNum();
        roif->output_ip6 = output;
        roif->linkoutput = linkOutput;
        roif->flags |= NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP | NETIF_FLAG_UP | NETIF_FLAG_MLD6;
		nd6_set_cb( roif, ncb );
        return ERR_OK;
    }

    static err_t output( struct netif* roif, struct pbuf *p, const ip6_addr_t *ipaddr ) {
        auto self = reinterpret_cast< RoIF6* >( roif->state );
        self->_send( Ip6Addr( *ipaddr ), PBuf::reference( p ) );
        return ERR_OK;
    }

    static err_t linkOutput( struct netif* roif, struct pbuf *p ) {
        assert( false && "Link output should not be directly used" );
        return ERR_OK;
    }

private:
	void _send( Ip6Addr addr, PBuf&& packet ) {
		auto gw = ip_find_route( &addr );
		if ( gw ) {
			gw->output_ip6( gw, packet.get(), &addr );
		}
	}

	void _broadcastRTable() {
		for ( auto& n : _netifs ) {
			n.sendRRP();
		}
	}

	const char* _createAddress ( int id ) {
		std::ostringstream s;
		s << "fc07::" << id;
		return s.str().c_str();
	}

	RoutingTable _rtable;
	netif _netif;
	std::vector< Netif > _netifs;
	rtos::Timer _mappingTimer;
};

} // namespace _rofi