#pragma once

#include <lwip/def.h>
#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/dhcp.h>
#include <lwip/ethip6.h>
#include <lwip/nd6.h>
#include <lwip/mld6.h>

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

namespace _rofi {
	class RoIF6;

inline std::ostream& operator<<( std::ostream& o, ip6_addr_t a ) {
	o << IP6_ADDR_BLOCK1(&a) << ":" << IP6_ADDR_BLOCK2(&a) << ":" << IP6_ADDR_BLOCK3(&a)
		<< ":" << IP6_ADDR_BLOCK4(&a) << ":" << IP6_ADDR_BLOCK5(&a) << ":" << IP6_ADDR_BLOCK6(&a)
		<< ":" << IP6_ADDR_BLOCK7(&a) << ":" << IP6_ADDR_BLOCK8(&a);
	return o;
}

// C++ wrapper for lwip addr for IPv6
struct Ip6Addr : ip6_addr_t {
	Ip6Addr( const char* str ) {
        ip6_addr_t ipad;
		ip6addr_aton(str, &ipad);
		addr[0] = ipad.addr[0]; addr[1] = ipad.addr[1];
		addr[2] = ipad.addr[2]; addr[3] = ipad.addr[3];
    }
    Ip6Addr( ip6_addr addr ): ip6_addr( addr ) {}

    bool operator==( const Ip6Addr& o ) const {
		return addr[0] == o.addr[0] && addr[1] == o.addr[1] && addr[2] == o.addr[2] && addr[3] == o.addr[3];
	}

    static int size() { return 16; }
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

class Netif {
public:
	Netif( PhysAddr pAddr, gpio_num_t dockCs, spi_host_device_t spiHost )
	: _pAddr(pAddr), _dock( spiHost, dockCs ) {
		_dock.onReceive( [this]( Dock& dock, int contentType, PBuf&& data ) {
			_onPacket( dock, contentType, std::move( data ) );
		} );

		netif_add( &_netif, NULL, NULL, NULL, this, init, tcpip_input );
		dhcp_stop( &_netif );
		netif_set_up( &_netif );
	}

	void addAddress( Ip6Addr addr ) {
		s8_t i = 1;
		netif_add_ip6_address( &_netif, ( ip6_addr_t* ) addr.addr, &i );
		netif_ip6_addr_set_state( &_netif, i, IP6_ADDR_TENTATIVE );
	}

	void setUp() {
		netif_set_default( &_netif );
		netif_create_ip6_linklocal_address( &_netif, 0 );
		netif_set_up( &_netif );
	}

	static void f(netif* n, u8_t s) {
		std::cout << std::hex << "nd6_callback! s: " << (int) s << "\n\t netif->ip_addr: " << *((ip6_addr_t*) &(n->ip_addr))
		<< " netif->output_ip6: " << *((ip6_addr_t*) &(n->output_ip6)) << std::dec << std::endl;
	}

	static err_t init( struct netif* n ) {
		auto self = reinterpret_cast< Netif* >( n->state );
		n->hwaddr_len = 6;
		std::copy_n( self->_pAddr.addr, 6, n->hwaddr );
		n->mtu = 120;
		n->name[ 0 ] = 'r'; n->name[ 1 ] = 'o';
		n->output_ip6 = output;
		n->input = netif_input;
		n->linkoutput = linkOutput;
		n->flags |= NETIF_FLAG_LINK_UP
	                //| NETIF_FLAG_IGMP
	        	    | NETIF_FLAG_MLD6
    	            | NETIF_FLAG_UP;
		n->num = _seqNum();
		nd6_set_cb( n, f );
		return ERR_OK;
	}

	static err_t output( struct netif* n, struct pbuf *p, const ip6_addr_t *ipaddr) {
		auto self = reinterpret_cast< Netif* >( n->state );
		self->_send( Ip6Addr( *ipaddr ), PBuf::reference( p ) );
		return ERR_OK;
	}

	static err_t linkOutput( struct netif* n, struct pbuf *p ) {
		assert( false && "Link output should not be directly used" );
		return ERR_OK;
	}

	void printAddresses() const {
		std::cout << "Addresses are \n" << std::hex;
		for ( const auto& n : _netif.ip6_addr ) {
			std::cout << "\t" << *((ip6_addr_t*) &n) << std::endl;
		}
		std::cout << std::dec;
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
        } else {
			std::cout << "_onPacket got contentType different from 0! " << std::endl;
		}
    }

    void _send( Ip6Addr addr, PBuf&& packet ) {
		_dock.sendBlob( 0, std::move( packet ) );
    }

	PhysAddr _pAddr;
	Dock _dock;
	struct netif _netif;

	friend class RoIF6;
};


// !!! THIS IS NOT IN USE !!! // 
// This class might be used to bridge docks' netifs.
class VirtNetif {
public:
	VirtNetif( Ip6Addr addr ) {
		netif_set_default( &_netif );
		netif_add( &_netif, NULL, NULL, NULL, this, virt_init, ip_input );
		netif_add_ip6_address( &_netif, ( ip6_addr_t* ) addr.addr, 0 );
		netif_ip6_addr_set_state( &_netif, 0, IP6_ADDR_TENTATIVE );
		dhcp_stop( &_netif );
		netif_set_up( &_netif );
	}

	void addAddress( Ip6Addr addr ) {
		s8_t i = 1;
		netif_add_ip6_address( &_netif, ( ip6_addr_t* ) addr.addr, &i );
		netif_ip6_addr_set_state( &_netif, i, IP6_ADDR_TENTATIVE );
	}

	static void f(netif* n, u8_t s) {
		std::cout << std::hex << "nd6_callback! s: " << (int) s << "\n\t netif->ip_addr: " << *((ip6_addr_t*) &(n->ip_addr))
		<< " netif->output_ip6: " << *((ip6_addr_t*) &(n->output_ip6)) << std::dec << std::endl;
	}

	static err_t virt_init( struct netif* n ) {
		n->mtu = 120;
		n->hwaddr_len = 6;
		n->name[ 0 ] = 'r'; n->name[ 1 ] = 'o';
		n->output_ip6 = output;
		n->input = netif_input;
		n->linkoutput = linkOutput;
		n->flags |= NETIF_FLAG_LINK_UP
	                //| NETIF_FLAG_IGMP
	        	    | NETIF_FLAG_MLD6
    	            | NETIF_FLAG_UP;
		n->num = _seqNum();
		nd6_set_cb( n, f );
		return ERR_OK;
	}

	void printAddresses() const {
		std::cout << "Addresses are \n" << std::hex;
		for ( const auto& n : _netif.ip6_addr ) {
			std::cout << "\t" << *((ip6_addr_t*) &n) << std::endl;
		}
		std::cout << std::dec;
	}

	static err_t output( struct netif* n, struct pbuf* p, const ip6_addr_t* ipaddr ) {
		std::cout << "output called!" << std::endl;
		auto self = reinterpret_cast< VirtNetif* >( n->state );
		self->_broadcast( n, p );
		return ERR_OK;
	}

	static err_t linkOutput( struct netif* n, struct pbuf *p ) {
		assert( false && "Link output should not be directly used" );
		return ERR_OK;
	}

private:
	static int _seqNum() {
        static int num = 0;
        return num++;
    }

	void _broadcast( struct netif* n, struct pbuf *p ) {
		_brd( n, p );
	}

	void _setBroadcast( std::function< void( struct netif* n, struct pbuf *p ) > f ) {
		_brd = f;
	}

	struct netif _netif;
	std::function< void( struct netif* n, struct pbuf *p ) > _brd;

	friend class RoIF6;
};

// RoFi Network Interface for IPv6
class RoIF6 {
public:
	RoIF6( PhysAddr pAddr, std::vector< gpio_num_t > dockCs, spi_host_device_t spiHost = HSPI_HOST) {
		/* : _main( addr ) {
		_main._setBroadcast( [this]( struct netif* n, struct pbuf *p ) {
			_broadcast( n, p );
		} ); */
		_netifs.reserve( dockCs.size() );
		for ( auto cs : dockCs ) {
			_netifs.emplace_back( pAddr, cs, spiHost );
		}
	}

	void printAddresses() const {
		int counter = 0;
		// std::cout << "for _main\n"; _main.printAddresses();
		for ( const auto& n : _netifs ) {
			std::cout << "netif number " << counter++ << std::endl;
			n.printAddresses();
		}
	}

	/*
	void addAddrTo( Ip6Addr addr ) {
		_main.addAddress( addr );
	}*/

	void addAddress( Ip6Addr addr ) {
		for ( auto& n : _netifs ) {
			n.addAddress( addr );
		}
	}

	void setUp() {
		for (auto& n : _netifs) {
			n.setUp();
		}
	}

private:
	void _broadcast( struct netif* n, struct pbuf *p ) {
		std::cout << "_broadcast inside of RoIF6" << std::endl;
		for ( auto& net : _netifs ) {
			net._netif.input( p, n );
		}
	}

	// VirtNetif _main;
	std::vector< Netif > _netifs;
};

} // namespace _rofi