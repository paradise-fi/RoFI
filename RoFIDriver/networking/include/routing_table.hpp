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

#define STUB          1
#define AUTOSUMARY    1
#define SUMMARY_DEPTH 2

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
	Record( const Ip6Addr& ip, uint8_t mask, const Gateway& gw, const std::vector< Record* >& s )
	: ip( ip ), mask( mask ), gws( { gw } ), sumarizing( s ) {}

	~Record() {
		if ( sumarizing.size() > 0 ) {
			for ( auto& r : sumarizing ) {
				if ( r )
					r->setSumarized( nullptr );
			}
		}

		if ( isSumarized() )
			sumarized->unsumary( *this );

		if ( active ) {
			ip_rm_route( &ip, mask );
		}	
	}

	Record& activate() {
		active = static_cast< bool >( ip_add_route( &ip, mask, gws.front().name ) );
		for ( auto& s : sumarizing )
			s->setSumarized( this );

		return *this;
	}

	bool isSumarized() const {
		return sumarized != nullptr;
	}

	void setSumarized( Record* r ) {
		sumarized = r;
	}

	void unsumary( const Record& r ) {
		sumarizing.erase( std::remove_if( sumarizing.begin(), sumarizing.end(), [ &r ]( const Record* ptr ) {
			return *ptr == r;
		} ) );

		finishedSumarizing = sumarizing.size() == 0;
	}

	bool toDelete() const {
		return finishedSumarizing;
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
		return gws.size() != 0 && strcmp( "null", gws.front().name ) != 0;
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
	bool finishedSumarizing = false;
	Record* sumarized = nullptr;
	std::vector< Record* > sumarizing;

	friend std::ostream& operator<<( std::ostream& o, const Record& r );
};

std::ostream& operator<<( std::ostream& o, const Record& r ) {
	char flag = '*';
	o << r.ip << "/" << static_cast< int >( r.mask );
	if ( r.isSumarized() )
		o << " sumarized by " << r.sumarized->ip << "/" << static_cast< int >( r.sumarized->mask );
	o << "\n";
	for ( auto& g : r.gws ) {
		o << "\t" << flag << g.name << " [" << g.cost << "]\n";
	}
	return o;
}

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
		bool sumarized = false;
		if ( rec.hasGateway() && rec.getGateway() != "null" ) {
			auto r = search( rec );
			if ( !r ) {
				return false;
			}
			if ( r->remove( rec.getGateway() ) && !r->hasGateway() ) {
				sumarized = r->isSumarized();
				records.remove( *r );
				if ( sumarized )
					checkSumarizing();
				return true;
			}
			return false;
		}

		std::size_t size = records.size();
		records.remove_if( [ &rec, &sumarized ]( const Record& r ) {
			if ( rec.hasSameNetwork( r ) ) {
				sumarized |= r.isSumarized();
				return true;
			}
			return false;
		} );

		if ( sumarized )
			checkSumarizing();

		return size != records.size();
	}
    
	bool removeForIf( Netif* n ) {
		if ( !n ) {
			return false;
		}

		std::size_t size = records.size();
		records.remove_if( [ &n ]( Record& r ) {
			return ( r.remove( n->getName() ) && !r.hasGateway() ) || r.toDelete();
		} );
		return size != records.size();
	}
    
	Record* search( const Ip6Addr& ip, uint8_t mask ) {
		Record rec( ip, mask, { "null", 0 } );
		return search( rec );
	}

	Record* search( const Record& rec ) {
		auto it = records.end();
		for ( auto searched = records.begin(); searched != records.end(); searched++ ) {
			if ( rec.hasSameNetwork( *searched ) ) {
				it = searched;
				break;
			}
		}

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

	PBuf createRRPmsg( Command cmd = Command::Call ) {
		return createRRPmsgIfless( nullptr, cmd );
	}

	PBuf createRRPmsgIfless( Netif* n, Command cmd = Command::Call ) {
		int count = n && n->isStub() ? 0 : ( records.size() - recForIfOrSummarized( n ) );
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
			if ( rec.isSumarized() )
				continue; // do not propagate routes, which are sumarized
			
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

	int wouldSumarize( std::list< Record>::iterator last, uint8_t prefix ) {
		if ( last == records.begin() )
			return 0;

		int wouldSumarizeCount = 0;
		uint8_t oldmask = last->mask;
		Ip6Addr mask = Ip6Addr( oldmask - prefix );
		Ip6Addr ip   = last->ip & mask;
		while ( oldmask == ( last )->mask ) {
			if ( ip == ( last->ip & mask ) )
				wouldSumarizeCount++;

			if ( last == records.begin() )
				break;
			--last;
		}

		return wouldSumarizeCount;
	}

	bool sumarize( std::list< Record >::iterator last, uint8_t prefix ) {
		uint8_t oldmask = last->mask;
		Cost cost       = last->getCost();
		Ip6Addr mask( oldmask - prefix );
		Ip6Addr ip = last->ip & mask;
		auto it    = last;
		std::vector< Record* > toBeSumarized;

		while ( it->mask == oldmask ) {
			if ( ( it->ip & mask ) == ip ) {
				toBeSumarized.push_back( &( *it ) );
				cost = std::min( cost, it->getCost() );
			}
			if ( it == records.begin() )
				break;
			it--;
		}

		if ( toBeSumarized.size() > 0 ) {
			it = records.insert( ++last, Record( ip, oldmask - prefix, Gateway( "null", cost ), toBeSumarized ) );
			it->activate();
			return true;
		}

		return false;
	}

	bool trySumarize( const std::list< Record >::iterator& last ) {
		int count = 0, newCount = 0;
		int prefix = 1, minimalPrefix = 1;
		for ( ; prefix <= SUMMARY_DEPTH; prefix++ ) {
			newCount = wouldSumarize( last, prefix );
			if ( newCount == count ) {
				continue;
			}
			count = newCount;
			minimalPrefix = prefix;
		}

		return ( count > 0 ) && sumarize( last, minimalPrefix );
	}

	void checkSumarizing() {
		for ( auto it = records.begin(); it != records.end(); it++ ) {
			if ( it->toDelete() ) {
				records.erase( it );
				break;
			}
		}
	}

	void addSorted( const Record& rec ) {
		auto it = records.begin();
		for ( ; it != records.end(); it++ ) {
			if ( it->mask > rec.mask ) {
				it = records.insert( it, rec );
				it->activate();
				break;
			}
		}

		if ( it == records.end() ) {
			it = records.insert( it, rec );
			it->activate();
		}

		#if AUTOSUMARY
		if ( records.size() > 1 )
			trySumarize( it );
		#endif
	}

	bool addRecord( const Record& rec ) {
		bool changed = false;
		auto r = search( rec );
		if ( r ) {
			if ( *r != rec ) {
				changed = r->merge( rec );
			}
		} else {
			addSorted( rec );
			changed = true;
		}

		return changed;
	}

	int recForIfOrSummarized( Netif* n ) const {
		if ( !n )
			return 0;

		return std::accumulate( records.begin(), records.end(), 0, [ &n ]( int acc, const Record& r ) {
			return ( r.getGateway() == n->getName() || r.isSumarized() ) ? acc + 1 : acc;
		} );
	}
	
	Action onRRPmsg( rofi::hal::PBuf packet, Netif* n ) {
		bool changed = false;
		Command cmd = as< Command >( packet.payload() );
		int count = static_cast< int >( as< uint8_t >( packet.payload() + 1) );
		auto data = packet.payload() + 2;

		n->setStub( cmd == Command::Stubby );

		int size = records.size();
		removeForIf( n );
		for ( int i = 0; i < count; i++ ) {
			Ip6Addr ip   = as< Ip6Addr >( data );
			uint8_t mask = as< uint8_t >( data + Ip6Addr::size() );
			Cost cost    = as< Cost >( data + Ip6Addr::size() + 1 );
			changed |= add( Record( ip, mask, Gateway( n->getName().c_str(), cost ) ) );
			data += Entry::size(); // changed is used with PA
		}
		changed = size != records.size();

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
		o << r;
	}

	if ( rt.isStub() )
		o << "stub node\n";
	return o;
}

} // namespace rofinet
