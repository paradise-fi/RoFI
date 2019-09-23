#pragma once

#define LWIPP_ND6_ALLOW_RA_UPDATES 0

#include <lwip/def.h>
#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/dhcp.h>
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

    bool operator==( const Ip6Addr& o ) const { return addr == o.addr; }

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

class AddressMapping_6 {
public:
	static const int ContentType = 1;
	enum class Command : uint8_t { Call = 0, Response = 1 };

	AddressMapping_6( const Ip6Addr& addr, const PhysAddr& guid )
		: _self{ addr, guid }
	{}

	struct Entry {
		Ip6Addr addr[ LWIP_IPV6_NUM_ADDRESSES ];
		PhysAddr guid;

		Entry ( const Ip6Addr& addr, const PhysAddr& guid )
			: addr{ "::", addr, "::" }
			, guid(guid) 
		{}

		static int size() { return Ip6Addr::size() + PhysAddr::size(); }
	};

	void onPacket( Dock& dock, const PBuf& packet ) {
		if ( packet.size() < 1 )
			return;
		auto command = as< Command >( packet.payload() );
		if ( command == Command::Call )
			sendResponse( dock );
		else if ( command == Command::Response ) {
			_parseResponse( dock, packet );
		}
	}

	void sendResponse( Dock& dock ) {
		auto p = PBuf::allocate( 4 + Entry::size() );
		as< Command >( p.payload() + 0 ) = Command::Response;
		as< uint8_t >( p.payload() + 1 ) = Ip6Addr::size();
		as< uint8_t >( p.payload() + 2 ) = PhysAddr::size();
		as< uint8_t >( p.payload() + 3 ) = LWIP_IPV6_NUM_ADDRESSES;
		auto dataField = p.payload() + 4;
		for ( int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++ ) {	
			as< Ip6Addr >( dataField + Ip6Addr::size() * i ) = _self.addr[i];
		}
		as< PhysAddr >( dataField + Ip6Addr::size() * LWIP_IPV6_NUM_ADDRESSES ) = _self.guid;
		dataField += Ip6Addr::size() * LWIP_IPV6_NUM_ADDRESSES + PhysAddr::size();
		dock.sendBlob( ContentType, std::move( p ) );
	}

	void sendCall( Dock& dock ) {
        auto p = PBuf::allocate( 1 );
        as< Command >( p.payload() ) = Command::Call;
        dock.sendBlob( ContentType, std::move( p ) );
    }

	Dock *destination( Ip6Addr add ) {
		for ( const auto& e : _entries )
			for ( int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++ )
				if ( e.first.addr[i] == add ) return e.second;
		return nullptr;
	}

	const Entry& self() const { return _self; }

private:
	void _parseResponse( Dock& d, const PBuf& packet ) {
		int count = as< uint8_t >( packet.payload() + 3 );
		// std::cout << std::dec << "Packet size: " << packet.size() << " count: " << count << " Entry::size(): " << Entry::size() << std::endl;
		if ( packet.size() < 4 )
			return;
		if ( as< uint8_t >( packet.payload() + 1) != Ip6Addr::size() )
			return;
		if ( as< uint8_t >( packet.payload() + 2) != PhysAddr::size() )
			return;

		if ( packet.size() != 4 + count * Entry::size() )
			return;

		_removeEntriesFor( d );
		for (int i = 0; i != count; i++ ) {
			const uint8_t *entry = packet.payload() + 4 + i * Entry::size();
			Ip6Addr lAddr = as< Ip6Addr >( entry );
			PhysAddr pAddr = as< PhysAddr >( entry + Ip6Addr::size() );
			_entries.push_back( { { lAddr, pAddr }, &d } );
		}
	}

	int _entriesFor( Dock& d) {
		return std::count_if( _entries.begin(), _entries.end(), [&]( const auto& e) {
			return e.second == &d;
		} );
	}

	template < typename Yield >
	void _forEachDockEntry( Dock& d, Yield yield ) {
		for ( const auto& e : _entries )
			if ( e.second == &d) yield( e.first );
	}

	void _removeEntriesFor( Dock& d ) {
		auto end = std::remove_if( _entries.begin(), _entries.end(), [&](const auto& e ){
			return e.second == &d;
		} );
		_entries.erase( end, _entries.end() );
	}

	std::vector< std::pair< Entry, Dock * > > _entries;
	Entry _self;

};

// RoFi Network Interface for IPv6
class RoIF6 {
public:
	RoIF6( PhysAddr pAddr, Ip6Addr ip, Ip6Addr mask, Ip6Addr gateway, 
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
		
		_netif.hwaddr_len = 6;
		std::copy_n( pAddr.addr, _netif.hwaddr_len, _netif.hwaddr );
		netif_add( &_netif, NULL, NULL, NULL, this, init, tcpip_input );
		//netif_add_ip6_address( &_netif, (ip6_addr_t*) ip.addr, nullptr );
		netif_ip6_addr_set_state( &_netif, 1, IP6_ADDR_TENTATIVE );

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
		netif_create_ip6_linklocal_address( &_netif, *_netif.hwaddr);
		netif_set_up( &_netif );
	}

	void printAddresses() {
		std::cout << "Addresses are \n" << std::hex;
		for ( const auto& n : _netif.ip6_addr ) {
			std::cout << "\t" << *((ip6_addr_t*) &n) << std::endl;
		}
		std::cout << std::dec;
	}

	static void f(netif* n, u8_t s) {
		std::cout << std::hex << "nd6_callback called; s: " << (int) s << " netif->ip_addr: " << *((ip6_addr_t*) &(n->ip_addr))
		<< " netif->output_ip6: " << *((ip6_addr_t*) &(n->output_ip6)) << std::dec << "\n";
	}

	static err_t init( struct netif* roif ) {
		auto self = reinterpret_cast< RoIF6* >( roif->state );
		roif->hwaddr_len = 6;
		std::copy_n( self->_mapping.self().guid.addr, 6, roif->hwaddr );
		roif->mtu = 120;
		roif->name[ 0 ] = 'r'; roif->name[ 1 ] = 'o';
		roif->output_ip6 = output;
		roif->linkoutput = linkOutput;
		roif->ip6_autoconfig_enabled = 0;
		roif->flags |= NETIF_FLAG_LINK_UP 
				| NETIF_FLAG_BROADCAST
                | NETIF_FLAG_IGMP
                | NETIF_FLAG_MLD6
                | NETIF_FLAG_UP; 
		roif->num = _seqNum();
		nd6_set_cb( roif, f );
		return ERR_OK;
	}

	static err_t output( struct netif* roif, struct pbuf *p, const ip6_addr_t *ipaddr) {
		auto self = reinterpret_cast< RoIF6* >( roif->state );
		self->_send( Ip6Addr( *ipaddr ), PBuf::reference( p ) );
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
        } else if ( contentType == AddressMapping_6::ContentType ) {
            _mapping.onPacket( dock, packet );
        }
    }

    void _send( Ip6Addr addr, PBuf&& packet ) {
        Dock* d = _mapping.destination( addr );
        if ( d )
            d->sendBlob( 0, std::move( packet ) );
    }

    Ip6Addr _mask, _gateway;
    netif _netif;
    std::vector< Dock > _docks;

    AddressMapping_6 _mapping;
    rtos::Timer _mappingTimer;
};

} // namespace _rofi