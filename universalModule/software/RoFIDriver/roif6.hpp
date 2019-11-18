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


static void ncb(netif* n, u8_t s) {
	// std::cout << std::hex << "nd6_callback! " << std::endl;
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

struct Entry {
	Ip6Addr addr;
	uint8_t mask;

	static int size() { return Ip6Addr::size() + 1; }
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

struct Record {
	Ip6Addr ip;
	uint8_t mask;
	gpio_num_t dockC;

	void print() const {
		std::cout << std::hex << ip << std::dec << "        " << (int) mask << "                " << (int) dockC;
	}
};

class RoutingTable {
	std::vector< Record > _records;
public:
	enum class Command : uint8_t { Call = 0, Response = 1 };

	void addRecord( const Record& r ) {
		addRecord( r.ip, r.mask, r.dockC );
	}

	void addRecord( const Ip6Addr& addr, uint8_t mask, gpio_num_t dock ) {
		for ( auto& rec : _records ) {
			if ( addr == rec.ip ) { // TODO: mask and dock should be checked
				return;
			}
		}
		_records.push_back( { addr, mask, dock } );
	}

	void removeRecord( const Record& r ) {
		removeRecord( r.ip, r.mask, r.dockC );
	}

	void removeRecord( const Ip6Addr& addr, uint8_t mask, gpio_num_t dock ) {
		// TODO
	}

	const Record* search( const Ip6Addr& ip ) const {
		std::cout << std::hex << " searching for " << ip << std::dec << std::endl;
		for ( const auto& r : _records ) {
			if ( r.ip == ip) // TODO: mask should be checked too!
				return &r;
		}
		return nullptr;
	}

	PBuf toSend() const {
		auto p = PBuf::allocate( 3 +  Entry::size() * _records.size() );
        as< Command >( p.payload() + 0 ) = Command::Response;
        as< uint8_t >( p.payload() + 1 ) = Ip6Addr::size();
        as< uint8_t >( p.payload() + 2 ) = _records.size();
		auto dataField = p.payload() + 3;
		for ( const auto& rec : _records ) {
			as< Ip6Addr >( dataField ) = rec.ip;
			as< uint8_t >( dataField + Ip6Addr::size() ) = rec.mask;
			dataField += Ip6Addr::size() + 1;
		}
        return p;
	}

	void update( const PBuf& packet, gpio_num_t dock ) {
        if ( packet.size() < 3 )
            return;
        if ( as< uint8_t >( packet.payload() + 1 ) != Ip6Addr::size() )
            return;
        int count = as< uint8_t >( packet.payload() + 2 );
        if ( packet.size() != 3 + count * Entry::size() )
            return;
        for ( int i = 0; i != count; i++ ) {
            const uint8_t *entry = packet.payload() + 3 + i * Entry::size();
            Ip6Addr addr = as< Ip6Addr >( entry );
            uint8_t mask = as< uint8_t >( entry + Ip6Addr::size() + 1 );
            addRecord( addr, mask, dock );
        }
		print();
    }

	void print() const {
		std::cout << "ip                      mask                dock\n";
		for ( const auto& r : _records ) {
			r.print();
			std::cout << std::endl;
		}
	}
};


class Netif {
public:
	Netif( PhysAddr pAddr, gpio_num_t dockCs, spi_host_device_t spiHost, RoutingTable& rt )
	: _pAddr(pAddr), _dock( spiHost, dockCs ), _rtable( rt ) {
		_dock.onReceive( [this]( Dock& dock, int contentType, PBuf&& data ) {
			_onPacket( dock, contentType, std::move( data ) );
		} );

		netif_add( &_netif, NULL, NULL, NULL, this, init, tcpip_input );
	}

	void addAddress( Ip6Addr addr, s8_t i ) {
		netif_add_ip6_address( &_netif, ( ip6_addr_t* ) addr.addr, &i );
		if (i < LWIP_IPV6_NUM_ADDRESSES && i > 0)
			netif_ip6_addr_set_state( &_netif, i, IP6_ADDR_TENTATIVE );
	}

	void setUp() {
		netif_set_default( &_netif );
		netif_create_ip6_linklocal_address( &_netif, 0 );
		netif_set_up( &_netif );
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
		n->flags |= NETIF_FLAG_LINK_UP | NETIF_FLAG_MLD6 | NETIF_FLAG_UP;
		n->num = _seqNum();
		nd6_set_cb( n, ncb );
		return ERR_OK;
	}

	static err_t output( struct netif* n, struct pbuf *p, const ip6_addr_t *ipaddr) {
		auto self = reinterpret_cast< Netif* >( n->state );
		self->_send( /* Ip6Addr( *ipaddr ),*/ PBuf::reference( p ) );
		if ( self->_loopback ) {
			pbuf_ref( p );
			self->_loopback( p, n );
		} else {
			std::cout << "loopback is null! \n";
		}
		return ERR_OK;
	}

	static err_t linkOutput( struct netif* n, struct pbuf *p ) {
		assert( false && "Link output should not be directly used" );
		return ERR_OK;
	}

	void printAddresses() const {
		std::cout << "Addresses are \n" << std::hex;
		for ( const auto& n : _netif.ip6_addr ) {
			std::cout << "\t" <<  *( ( ip6_addr_t* ) &n ) << std::endl;
		}
		std::cout << std::dec;
	}

	gpio_num_t getDockGPIO() const {
		return _dock.id();
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
			_rtable.update( packet, dock.id() );
		}
    }

    void _send( PBuf&& packet, int contentType = 0 ) {
		_dock.sendBlob( contentType, std::move( packet ) );
    }

	PhysAddr _pAddr;
	Dock _dock;
	struct netif _netif;
	std::function< void( struct pbuf*, struct netif* )> _loopback;
	RoutingTable& _rtable;

	friend class RoIF6;
};


class VirtNetif {
public:
	VirtNetif( Ip6Addr addr, RoutingTable& rt )
	: _rtable( rt ) {
		netif_set_default( &_netif );
		netif_add( &_netif, NULL, NULL, NULL, this, virt_init, tcpip_input );
		netif_add_ip6_address( &_netif, static_cast< ip6_addr_t* >( &addr ), 0 );
		netif_ip6_addr_set_state( &_netif, 0, IP6_ADDR_TENTATIVE );
		dhcp_stop( &_netif );
		netif_set_up( &_netif );
		_rtable.addRecord( addr, 0, static_cast< gpio_num_t >( 0 ) );
	}

	void addAddress( const Ip6Addr& addr ) {
		netif_add_ip6_address( &_netif, as< ip6_addr_t* >( &addr.addr ), nullptr );
	}

	void addAddress( const Ip6Addr& addr, s8_t index ) {
		netif_add_ip6_address( &_netif, as< ip6_addr_t* >( &addr.addr ), &index );
		netif_ip6_addr_set_state( &_netif, index, IP6_ADDR_TENTATIVE );
	}

	const ip6_addr_t* getAddress( int index ) const {
		return as< const ip6_addr_t* >( &_netif.ip6_addr[index] );
	}

	const ip6_addr_t* getAddress() const {
		return getAddress( 0 );
	}

	static err_t virt_init( struct netif* n ) {
		n->mtu = 120;
		n->hwaddr_len = 6;
		n->name[ 0 ] = 'r'; n->name[ 1 ] = 'o';
		n->output_ip6 = output;
		n->input = netif_input;
		n->linkoutput = linkOutput;
		n->flags |= NETIF_FLAG_LINK_UP | NETIF_FLAG_MLD6 | NETIF_FLAG_UP;
		n->num = _seqNum();
		nd6_set_cb( n, ncb );
		return ERR_OK;
	}

	void printAddresses() const {
		std::cout << "Addresses are \n" << std::hex;
		for ( const auto& n : _netif.ip6_addr ) {
			std::cout << "\t" << *( (ip6_addr_t*) &n ) << std::endl;
		}
		std::cout << std::dec;
	}

	static err_t output( struct netif* n, struct pbuf* p, const ip6_addr_t* ipaddr ) {
		auto self = reinterpret_cast< VirtNetif* >( n->state );
		auto* rec = self->_rtable.search( Ip6Addr( *ipaddr ) );
		if ( rec ) {
			std::cout << "res is not null, sending via dock " << rec->dockC << std::endl;
			self->_sendBy( n, p, rec->dockC );
		}
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

	void _broadcast( struct netif* n, struct pbuf *p, const ip6_addr_t* ipaddr ) {
		_brd( n, p, ipaddr );
	}

	void _setBroadcast( std::function< void( struct netif* n, struct pbuf *p, const ip6_addr_t* ipaddr ) > f ) {
		_brd = f;
	}

	void _setSendBy( std::function< void( struct netif* n, struct pbuf* p, gpio_num_t dock ) > f ) {
		_sendBy = f;
	}

	struct netif _netif;
	RoutingTable& _rtable;
	std::function< void( struct netif* n, struct pbuf *p, const ip6_addr_t* ipaddr ) > _brd;
	std::function< void( struct netif* n, struct pbuf* p, gpio_num_t dock ) > _sendBy;

	friend class RoIF6;
};

// RoFi Network Interface for IPv6
class RoIF6 {
public:
	RoIF6( Ip6Addr addr, PhysAddr pAddr, std::vector< gpio_num_t > dockCs, spi_host_device_t spiHost = HSPI_HOST)
	: _main( addr, _rtable ) {
		_main._setBroadcast( [this]( struct netif* n, struct pbuf *p, const ip6_addr_t* ipaddr ) {
			_broadcast( n, p, ipaddr );
		} );

		_main._setSendBy( [this]( struct netif* n, struct pbuf* p, gpio_num_t dock ) {
			_sendBy( n, p, dock );
		});

		_netifs.reserve( dockCs.size() );
		for ( auto cs : dockCs ) {
			_netifs.emplace_back( pAddr, cs, spiHost, _rtable );
			_netifs.back()._loopback = [this]( struct pbuf* p, struct netif* n ) {
				std::cout << "loopback called\n";
				_main._netif.input( p, n );
			};
		}
	}

	void printAddresses() const {
		int counter = 0;
		std::cout << "for _main\n"; _main.printAddresses();
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
		std::cout << "VirtNetif addresses \n";
		_main.printAddresses();
		std::cout << "netifs addresses \n";
		for ( auto& n : _netifs ) {
			n.printAddresses();
		}
	}

	void setUp() {
		for ( auto& n : _netifs ) {
			n.setUp();
		}

		_mappingTimer = rtos::Timer( 5000 / portTICK_PERIOD_MS, rtos::Timer::Type::Periodic,
            [this]() {
                for ( auto& n : _netifs ) {
					n._send( _rtable.toSend(), 1 );
				}
            } );
        _mappingTimer.start();
	}

private:
	void _broadcast( struct netif* n, struct pbuf* p, const ip6_addr_t* ipaddr ) {
		// std::cout << "_broadcast inside of RoIF6" << std::endl;
		for ( auto& net : _netifs ) {
			pbuf_ref( p );
			net._netif.output_ip6( &net._netif, p, ipaddr );
		}
	}

	bool _sendBy( struct netif* n, struct pbuf* p, gpio_num_t dock ) {
		if ( dock == 0 ) { // special case when we want to target our virtual netif
			std::cout << "_main here \n";
			_main._netif.input( p, n );
			return true;
		}

		for ( auto& net :_netifs ) {
			if ( net.getDockGPIO() == dock ) {
				std::cout << "dock used " << dock << std::endl;
				net._send( PBuf::reference( p ) );
				return true;
			}
		}
		return false;
	}

	RoutingTable _rtable;
	VirtNetif _main;
	std::vector< Netif > _netifs;
	rtos::Timer _mappingTimer;
};

} // namespace _rofi