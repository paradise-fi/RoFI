#pragma once

#include "atoms/util.hpp"
#include "rofi_hal.hpp"

#include <tuple>
#include <list>
#include <functional>
#include <vector>
#include <cstring>
#include <optional>
#include <numeric>
#include <iostream>

#define STUB 1

using Cost = unsigned;

namespace rofinet {
	using namespace rofi::hal;

struct Gateway {
	char name[ 5 ];
	Cost cost;

	Gateway() = delete;
	Gateway( const char* n, Cost c ) : cost( c ) {
		strcpy( name, n );
	};

	bool operator==( const Gateway& g ) const {
		return g.cost == cost && strcmp( name, g.name ) == 0;
	}
	bool operator!=( const Gateway& g ) const {
		return !( g == *this );
	}
};

struct Record {
    Ip6Addr ip;
    uint8_t mask;
    std::list< Gateway > gws;

	Record() = delete;
	Record( const Ip6Addr& ip, uint8_t mask, const Gateway& gw ) : ip( ip ), mask( mask ), gws( { gw } ) {}
	Record( const Ip6Addr& ip, uint8_t mask, const Gateway& gw, std::function< void() > f )
	: ip( ip ), mask( mask ), gws( { gw } ), cb( f ) {}

	~Record() {
		if ( sumarized() )
			cb();

		if ( active ) {
			ip_rm_route( &ip, mask );
		}	
	}

	Record& activate() {
		active = static_cast< bool >( ip_add_route( &ip, mask, gws.front().name ) );
		return *this;
	}

	bool sumarized() const {
		return static_cast< bool >( cb );
	}

	void setCb( std::function< void() > f ) {
		cb = f;
	}

	void update( const Gateway& r ) const {
	if ( gws.size() == 0 || r != gws.front() )
		ip_update_route( &ip, mask, gws.front().name );
}

	bool merge( const Record& r ) {
		if ( !hasSameNetwork( r ) )
			return false;

		std::size_t size = gws.size();
		auto first       = gws.front();
		std::list< Gateway > toMerge = r.gws;
		gws.merge( toMerge, []( const Gateway& g1, const Gateway& g2 ) {
			return g1.cost <= g2.cost;
		} );
		gws.unique();

		update( first );
		return size != gws.size();
	}

	bool disjoin( const Record& r ) {
		if ( !hasSameNetwork( r ) )
			return false;
		
		std::size_t size = gws.size();
		auto first       = gws.front();
		for ( auto g : r.gws ) {
			gws.remove( g );
		}

		update( first );
		return size != gws.size();
	}

	bool remove( const std::string& name ) {
		if ( gws.size() == 0 ) {
			return false;
		}

		std::size_t size = gws.size();
		auto first       = gws.front();

		gws.remove_if( [ &name ]( const Gateway& g ) { return strcmp( name.c_str(), g.name ) == 0; } );

		update( first );
		return size != gws.size();
	}

    Cost getCost() const { // should be called on valid record!
		return gws.front().cost;
	}

    std::string getGateway() const {
		return gws.size() != 0 ? gws.front().name : "null";
	}

	bool hasGateway() const {
		return gws.size() != 0 && strcmp("null", gws.front().name ) != 0;
	}

    bool hasSameNetwork( const Record& rec ) const {
		return rec.mask == mask && ( Ip6Addr( mask ) & ip ) == ( Ip6Addr( mask ) & rec.ip );
	}

	bool isStub() const {
		auto ptr = netif_find( gws.front().name );
		return ptr && as< Netif* >( ptr->state )->isStub();
	}

    bool operator==( const Record& rec ) const {
		return rec.ip == ip && rec.mask == mask && rec.gws == gws;
	}
    bool operator!=( const Record& rec ) const {
		return !( *this == rec );
	}
private:
	bool active = false;
	std::function< void() > cb;
};


class RTable {
public:
	enum class Command : uint8_t { Call = 0, Response = 1, Stubby = 2 };
	enum class Action  : int     { BroadcastRespond = 0, BroadcastCall = 1, Respond = 2, Nothing = 3 };

	RTable() = default;

    bool add( const Ip6Addr& ip, uint8_t mask, Cost cost, Netif* n ) {
		return add( Record( ip, mask, Gateway( n ? n->getName().c_str() : "null", cost ) ) );
	}

	bool add( const Record& rec ) {
		if ( !rec.hasGateway() )
			return false;

		return addRecord( rec );
	}

	bool remove( const Ip6Addr& ip, uint8_t mask, Netif* n ) {
		return remove( Record( ip, mask, Gateway( n ? n->getName().c_str() : "null", 0 ) ) );
	}

	bool remove( const Ip6Addr& ip, uint8_t mask ) {
		return remove( ip, mask, nullptr );
	}

	bool remove( const Record& rec ) {
		if ( rec.hasGateway() && strcmp( rec.getGateway().c_str(), "null" ) != 0 ) {
			auto r = search( rec );
			if ( !r ) {
				return false;
			}
			if ( r->remove( rec.getGateway() ) && !r->hasGateway() ) {
				records.remove( *r );
				return true;
			}
			return false;
		}

		std::size_t size = records.size();
		records.remove_if( [ &rec ]( const Record& r ) {
			return rec.hasSameNetwork( r );
		} );

		return size != records.size();
	}
    
	bool removeForIf( Netif* n ) {
		if ( !n ) {
			return false;
		}

		std::size_t size = records.size();
		records.remove_if( [ &n ]( Record& r ) {
			return r.remove( n->getName() ) && !r.hasGateway();
		} );
		return size != records.size();
	}
    
	Record* search( const Ip6Addr& ip, uint8_t mask ) {
		Record rec( ip, mask, { "null", 0 } );
		return search( rec );
	}

	Record* search( const Record& rec ) {
		auto it = std::find_if( records.begin(), records.end(), [ &rec ]( Record& r ) {
			return rec.hasSameNetwork( r );
		} );

		if ( it != records.end() ) {
			return &( *it );
		}

		return nullptr;
	}

	bool setDefaultGW( Netif* n ) {
		return add( Ip6Addr( "::" ), 0, 0, n );
	}

	bool removeDefaultGW( Netif* n ) {
		return remove( Ip6Addr( "::" ), 0, n );
	}

	bool hasDefaultGW() {
		return search( Ip6Addr( "::" ), 0 ) != nullptr;
	}

	bool isStub() const {
		return stub != nullptr;
	}

	bool isCall( Command cmd ) const {
		return cmd == Command::Call;
	}

	struct Entry {
		Ip6Addr ip;
		uint8_t mask;
		Cost cost;

		static int size() {
			return Ip6Addr::size() + 1 + sizeof( Cost );
		}
	};

	PBuf createRRPmsg( Command cmd ) {
		return createRRPmsgIfless( nullptr, cmd );
	}

	PBuf createRRPmsgIfless( Netif* n, Command cmd ) {
		int count = n && n->isStub() ? 0 : ( records.size() - recForIf( n ) );
		auto packet = PBuf::allocate( 2 + count * Entry::size() );
		as< Command >( packet.payload() ) = cmd;
		as< uint8_t >( packet.payload() + 1 ) = static_cast< uint8_t >( count );

		if ( isCall( cmd ) )
			counter++;

		if ( count == 0)
			return packet;

		auto data = packet.payload() + 2;
		for ( const auto& rec : records ) {
			if ( n && ( n->getName() == rec.getGateway() || n->isStub() ) )
				continue; // do not propagate to ignored netif or stubby netif
			
			as< Ip6Addr >( data ) = rec.ip;
			as< uint8_t >( data + Ip6Addr::size() ) = rec.mask;
			as< Cost >( data + Ip6Addr::size() + 1 ) = rec.getCost() + 1;
			data += Entry::size();
		}

		return packet;
	}


	Action update( PBuf packet, Netif* n ) {
		Action action = onRRPmsg( packet, n );
		bool changed = false;

		#if STUB
		if ( !isStub() && shouldBeStub() ) {
			changed = true;
			makeStub();
		} else if ( isStub() && !shouldBeStub() ) {
			changed = true;
			destroyStub();
		}
		#endif

		return !changed ? action : ( isStub() ? Action::BroadcastRespond : Action::BroadcastCall );
	}

private:
	std::list< Record > records;
	Netif* stub = nullptr;
	volatile unsigned counter = 0;

	bool addRecord( const Record& rec ) {
		bool changed = false;
		auto r = search( rec );
		if ( r ) {
			if ( *r != rec ) {
				changed = r->merge( rec );
			}
		} else {
			records.push_back( rec );
			records.back().activate();
			changed = true;
		}

		return changed;
	}

	int recForIf( Netif* n ) const {
		if ( !n )
			return 0;

		return std::accumulate( records.begin(), records.end(), 0, [ &n ]( int acc, const Record& r ) {
			return r.getGateway() == n->getName() ? acc + 1 : acc;
		} );
	}
	
	Action onRRPmsg( rofi::hal::PBuf packet, Netif* n ) {
		bool changed = false;
		Command cmd = as< Command >( packet.payload() );
		int count = static_cast< int >( as< uint8_t >( packet.payload() + 1) );
		auto data = packet.payload() + 2;

		n->setStub( cmd == Command::Stubby );

		removeForIf( n );
		for ( int i = 0; i < count; i++ ) {
			Ip6Addr ip   = as< Ip6Addr >( data );
			uint8_t mask = as< uint8_t >( data + Ip6Addr::size() );
			Cost cost    = as< Cost >( data + Ip6Addr::size() + 1 );
			changed |= add( Record( ip, mask, Gateway( n->getName().c_str(), cost ) ) );
			data += Entry::size();
		}

		if ( !isCall( cmd ) )
			counter--;

		return ( changed && isSynced() ) ? ( isCall( cmd ) ? Action::BroadcastRespond : Action::BroadcastCall )
		                                 : ( isCall( cmd ) ? Action::Respond : Action::Nothing );
	}

	bool hasOneOutOrStub() const {
		std::string name = "";
		bool sameNames = true;
		for ( const auto& rec : records ) {
			if ( ( rec.getCost() == 0 && rec.mask != 0 ) || rec.isStub() ) // it's loopback or stub if -> skip it
				continue;
			if ( name == "" )
				name = rec.getGateway();
			else
				sameNames &= name == rec.getGateway();
		}

		return name != "" && sameNames; 
	}

	bool isSynced() const {
		return counter == 0;
	}

	bool shouldBeStub() const {
		return isSynced() && hasOneOutOrStub();
	}

	Netif* getStubbyIf() {
		Netif* n = nullptr;
		for ( const auto& r : records ) {
			if ( r.getCost() == 0 )
				continue;
			auto tmp = netif_find( r.getGateway().c_str() );
			n = static_cast< Netif* >( tmp ? tmp->state : nullptr );
			break;
		}
		return n;
	}


	void makeStub() {
		stub = getStubbyIf();
		removeForIf( stub );
		setDefaultGW( stub );
	}

	void destroyStub() {
		stub = nullptr;
		removeDefaultGW( stub );
	}

	friend std::ostream& operator<<( std::ostream& o, const RTable& rt );
};

std::ostream& operator<<( std::ostream& o, const RTable& rt ) {
	for ( const auto& r : rt.records ) {
		o << r.ip << "/" << static_cast< int >( r.mask ) << "\n";
		char sym = '*';
		for ( const auto& g : r.gws ) {
			o << "\t" << sym << g.name << " [" << g.cost << "]\n";
			sym = ' ';
		}
	}

	if ( rt.isStub() )
		o << "stub node\n";
	return o;
}

} // namespace rofinet
