#pragma once

#include <lwip/netif.h>
#include "dock.hpp"

namespace _rofi {

struct PhysAddr;

// C++ wrapper for lwip addr for IPv6
struct Ip6Addr : ip6_addr_t {
	Ip6Addr( const char* str ) {
		ip6addr_aton( str, this );
    }
    Ip6Addr( ip6_addr addr ): ip6_addr( addr ) {}
	Ip6Addr( ip6_addr&& other ) {
		std::swap( addr, other.addr );
	}

    bool operator==( const Ip6Addr& o ) const {
		return addr[0] == o.addr[0] && addr[1] == o.addr[1] && addr[2] == o.addr[2] && addr[3] == o.addr[3];
	}

    static int size() { return 16; }
};

inline std::ostream& operator<<( std::ostream& o, const Ip6Addr& a ) {
	o << ip6addr_ntoa( &a );
	return o;
}

struct Entry {
	Ip6Addr addr;
	uint8_t mask;

	static int size() { return Ip6Addr::size() + 1; }
};

struct Record {
	Ip6Addr ip;
	uint8_t mask;
	gpio_num_t dockC;

	Record() : ip( "::" ), mask( 0 ), dockC( static_cast< gpio_num_t >( 0 ) ) {};

	Record operator=( const Record& r ) {
		ip = r.ip; mask = r.mask; dockC = r.dockC;
		return *this;
	}

	void print() const {
		std::cout << ip << "         " << mask << "                " << dockC;
	}
};

class RoutingTable {
public:
	enum class Command : uint8_t { Call = 0, Response = 1 };

	void addRecord( const Record& r ) {
		assert( false && "Toto nechceme");
		addRecord( r.ip, r.mask, nullptr );
	}

	void addRecord( const Ip6Addr& addr, uint8_t mask, struct netif* n ) {
		ip_add_route( &addr, mask, n );
	}

	void removeRecord( const Record& r ) {
		removeRecord( r.ip, r.mask, nullptr );
	}

	bool getRecord( int i, Record& rec ) const {
		const rt_entry* r = ip_get_record( i );
		if ( r != nullptr ) {
			rec.ip = r->addr;
			rec.mask = r->mask;
			return true;
		}

		return false;
	}

	bool removeRecord( const Ip6Addr& addr, uint8_t mask, struct netif* n ) {
		return ip_rm_route( &addr, mask, n );
	}

	PBuf toSend() const {
		int records_size = ip_get_size();
		auto p = PBuf::allocate( 3 +  Entry::size() * records_size );
        as< Command >( p.payload() + 0 ) = Command::Response;
        as< uint8_t >( p.payload() + 1 ) = Ip6Addr::size();
        as< uint8_t >( p.payload() + 2 ) = static_cast< uint8_t >( records_size );
		auto dataField = p.payload() + 3;
		Record rec;
		for ( int i = 0; getRecord( i, rec ); i++ ) {
			as< Ip6Addr >( dataField ) = rec.ip;
			as< uint8_t >( dataField + Ip6Addr::size() ) = rec.mask;
			dataField += Ip6Addr::size() + 1;
		}
        return p;
	}

	void update( const PBuf& packet, gpio_num_t dock, struct netif* n ) {
        if ( packet.size() < 3 ) {
            return;
		}
        if ( as< uint8_t >( packet.payload() + 1 ) != Ip6Addr::size() ) {
            return;
		}
        int count = as< uint8_t >( packet.payload() + 2 );
        if ( packet.size() != 3 + count * Entry::size() ) {
            return;
		}
        for ( int i = 0; i != count; i++ ) {
            const uint8_t *entry = packet.payload() + 3 + i * Entry::size();
            Ip6Addr addr = as< Ip6Addr >( entry );
            uint8_t mask = as< uint8_t >( entry + Ip6Addr::size() );
			addRecord( addr, mask, n );
        }
    }

	void print() const {
		ip_print_table();
	}
};

} // namespace _rofi