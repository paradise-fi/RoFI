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

class RTable;

struct Record {
    const Ip6Addr ip;
    const uint8_t mask;
    std::list< Gateway > gws;

	Record() = delete;
	Record( const Ip6Addr& ip, uint8_t mask, const Gateway& gw ) : ip( ip ), mask( mask ), gws( { gw } ) {}
	Record( const Ip6Addr& ip, uint8_t mask, const Gateway& gw, const std::vector< Record* >& s )
	: ip( ip ), mask( mask ), gws( { gw } ), sumarizing( s ) {}

	~Record() {
		if ( isSumarizing() ) {
			for ( auto& r : sumarizing ) {
				if ( r )
					r->setSummarized( nullptr );
			}
		}

		if ( isSummarized() )
			summarized->unsummary( *this );

		if ( active ) {
			ip_rm_route( &ip, mask );
		}	
	}

	Record& activate() {
		if ( hasGateway() )
			active = static_cast< bool >( ip_add_route( &ip, mask, gws.front().name ) );
		for ( auto& s : sumarizing )
			s->setSummarized( this );

		return *this;
	}

	bool isSumarizing() const {
		return sumarizing.size() != 0;
	}

	bool isSummarized() const {
		return summarized != nullptr;
	}

	void setSummarized( Record* r ) {
		summarized = r;
	}

	void unsummary( const Record& r ) {
		sumarizing.erase( std::remove_if( sumarizing.begin(), sumarizing.end(), [ &r ]( Record* ptr ) {
			if ( *ptr == r )
				ptr->summarized = nullptr;
			return *ptr == r;
		} ) );

		finishedSumarizing = sumarizing.size() == 0;
	}

	void updateSummary( const Record& r ) {
		if ( !isSumarizing() )
			return;

		Cost cost   = getCost();
		Cost actual = 255;
		for ( const auto& s : sumarizing ) {
			if ( s && s->gws.size() > 0 )
				actual = std::min( actual, s->getCost() );
}

		if ( actual != 255 && cost != actual )
			gws.begin()->cost = actual;

		if ( r.gws.size() == 0 )
			unsummary( r );

		finishedSumarizing = sumarizing.size() == 0;
	}

	bool toDelete() const {
		return finishedSumarizing;
	}

	void update( const Gateway& g, int size ) const {
	if ( active && ( gws.size() != 0 && g != gws.front() ) ) {
		ip_update_route( &ip, mask, gws.front().name );
	}
		
	if ( size != gws.size() && isSummarized() )
		summarized->updateSummary( *this );
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

    std::string getGwName() const {
		return gws.size() != 0 ? gws.front().name : "null";
	}

	Gateway getGateway() const {
		return gws.front();
	}

	bool hasGateway() const {
		return gws.size() != 0 && strcmp( "null", gws.front().name ) != 0;
	}

    bool hasSameNetwork( const Record& rec ) const {
		return rec.mask == mask && ( Ip6Addr( mask ) & ip ) == ( Ip6Addr( mask ) & rec.ip );
	}

	bool isStub() const {
		Netif* n = static_cast< Netif* >( netif_find( getGwName().c_str() ) );
		return n && n->isStub();
	}

	bool singlePath() const {
		return 1 == std::accumulate( gws.begin(), gws.end(), 0, []( int acc, const Gateway& g ) {
			return std::string( g.name ) != "null" ? ( acc + 1 ) : acc;
		} );
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

	friend RTable;
	friend std::ostream& operator<<( std::ostream& o, const Record& r );
};

std::ostream& operator<<( std::ostream& o, const Record& r ) {
	char flag = '*';
	o << r.ip << "/" << static_cast< int >( r.mask );
	if ( r.isSummarized() )
		o << " summarized by " << r.summarized->ip << "/" << static_cast< int >( r.summarized->mask );
	o << "\n";
	for ( auto& g : r.gws ) {
		o << "\t" << flag << g.name << " [" << g.cost << "]" << ( r.isStub() ? " stubby" : "      " ) << "\n";
		flag = ' ';
	}
	return o;
}


class RTable {
public:
	enum class Command : uint8_t { Call = 0, Response = 1, Stubby = 2, Hello = 3, HelloResponse = 4, Sync = 5 };
	enum class Action  : int     { BroadcastRespond = 0, BroadcastCall = 1, Respond = 2, Nothing = 3, OnHello = 4
	                             , BroadcastHello = 5 };

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
		return removeRecord( rec );
	}
    
	bool removeForIf( Netif* n ) {
		if ( !n ) {
			return false;
		}

		std::size_t size = records.size();
		records.remove_if( [ &n, this ]( Record& r ) {
			bool toRemove = ( r.remove( n->getName() ) && !r.hasGateway() ) || r.toDelete();
			if ( toRemove )
				addToChanges( r, Entry::Act::Remove );

			return toRemove;
		} );

		return size != records.size();
	}
    
	Record* search( const Ip6Addr& ip, uint8_t mask ) {
		Record rec( ip, mask, { "", 0 } );
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

	bool isHello( Command cmd ) const {
		return cmd == Command::Hello || cmd == Command::HelloResponse;
	}

	bool isResponse( Command cmd ) const {
		return cmd == Command::Response || cmd == Command::Stubby;
	}

	bool isBroadcast( Action act ) const {
		return act == Action::BroadcastCall || act == Action::BroadcastRespond;
	}

	void clearChanges() {
		changes.clear();
	}

	struct Entry {
		Action action;
		Ip6Addr ip;
		uint8_t mask;
		Cost cost;

		static int size() {
			return sizeof( Action ) + Ip6Addr::size() + 1 + sizeof( Cost );
		}

		enum class Act : bool { Add = true, Remove = false };
	};

	PBuf createRRPhello( Netif* n, Command cmd = Command::Hello ) {
		int count   = records.size() - recForIfOrSummarized( n );
		auto packet = PBuf::allocate( 2 + count * Entry::size() );
		as< Command >( packet.payload() )     = cmd;
		as< uint8_t >( packet.payload() + 1 ) = static_cast< uint8_t >( count );

		n->setStub( cmd == Command::Stubby );

		auto data = packet.payload() + 2;
		for ( const auto& rec : records ) {
			if ( n && rec.getGwName() == n->getName() )
				continue;
			if ( rec.isSummarized() )
				continue; // skip summarized records
            if ( rec.ip == Ip6Addr( "::" ) && rec.mask == 0 )
                continue;
			addEntry( data, rec );
			data += Entry::size();
		}

		return packet; 
	}

	PBuf createRRPmsg( Command cmd = Command::Call ) {
		return createRRPmsgIfless( nullptr, cmd );
	}

	PBuf createRRPmsgIfless( Netif* n, Command cmd = Command::Call ) {
		int count   = ( n && n->isStub() ) ? 0 : ( changes.size() - changesIfless( n ) );
		auto packet = PBuf::allocate( 2 + count * Entry::size() );
		as< Command >( packet.payload() )     = cmd;
		as< uint8_t >( packet.payload() + 1 ) = static_cast< uint8_t >( count );

		if ( isCall( cmd ) )
			counter++;

		if ( count == 0 )
			return packet;

		auto data = packet.payload() + 2;
		for ( const auto& [ act, rec ] : changes ) {
			if ( n && ( n->getName() == rec.getGwName() || n->isStub() ) )
				continue; // do not propagate to ignored netif or stubby netif
			if ( rec.isSummarized() )
				continue; // do not propagate routes, which are summarized
			if ( rec.getCost() == 0 && rec.mask == 0 )
				continue; // do not propagate default gw
			
			addEntry( data, rec, act );
			data += Entry::size();
		}

		return packet;
	}

	Action update( PBuf packet, Netif* n ) {
		Action action = onRRPmsg( packet, n );
		bool changed = false;

		#if STUB
		if ( isSynced() ) {
			if ( !isStub() && shouldBeStub() ) {
				changed = true;
				makeStub();
			} else if ( isStub() && !shouldBeStub() ) {
				changed = true;
				destroyStub();
				return Action::BroadcastHello;
            }
		}
		#endif

		#if AUTOSUMARY
		if ( records.size() > 1 && isBroadcast( action ) ) {
			uint8_t mask = records.back().mask;
			auto it      = records.end();
			while ( it != records.begin() ) {
				it--;
				if ( mask != it->mask ) {
					mask = it->mask;
					changed |= trySummarize( --it );
					it++; // return it back after shift in prev. line
				}
			}
		}
		#endif

		return !changed ? action : ( isStub() ? Action::BroadcastRespond : Action::BroadcastCall );
	}

	Netif* getStubOut() const {
		return stub;
	}

private:
	std::list< Record > records;
	Netif* stub = nullptr;
	volatile int counter = 0;
	std::vector< std::pair< Entry::Act, Record > > changes;

	void addEntry( uint8_t* data, const Record& rec, Entry::Act act = Entry::Act::Add ) {
		as< Ip6Addr >( data )                       = rec.ip;
		as< uint8_t >( data + Ip6Addr::size() )     = rec.mask;
		as< Cost    >( data + Ip6Addr::size() + 1 ) = rec.getCost() + 1;
		as< Entry::Act >( data + Ip6Addr::size() + 1 + sizeof( Cost ) ) = act;
	}

	int wouldSummarize( std::list< Record>::iterator last, uint8_t prefix ) {
		if ( last == records.begin() || prefix >= last->mask )
			return 0;

		int wouldSummarizeCount = 0;
		uint8_t oldmask = last->mask;
		Ip6Addr mask = Ip6Addr( oldmask - prefix );
		Ip6Addr ip   = last->ip & mask;
		while ( oldmask == ( last )->mask ) {
			if ( ip == ( last->ip & mask ) )
				wouldSummarizeCount++;

			if ( last == records.begin() )
				break;
			--last;
		}

		return wouldSummarizeCount;
	}

	bool summarize( std::list< Record >::iterator last, uint8_t prefix ) {
		uint8_t oldmask = last->mask;
		Cost cost       = last->getCost();
		Ip6Addr mask( oldmask - prefix );
		Ip6Addr ip = last->ip & mask;
		auto it    = last;
		std::vector< Record* > toBeSummarized;

		while ( it->mask == oldmask ) {
			if ( ( it->ip & mask ) == ip ) {
				toBeSummarized.push_back( &( *it ) );
				cost = std::min( cost, it->getCost() );
			}
			if ( it == records.begin() )
				break;
			it--;
		}

		if ( toBeSummarized.size() > 1 ) {
			if ( !search( Record( ip, oldmask - prefix, Gateway( "null", cost ) ) ) ) {
				it = records.insert( ++last, Record( ip, oldmask - prefix, Gateway( "null", cost ), toBeSummarized ) );
				it->activate();
				addToChanges( Record( ip, oldmask - prefix, Gateway( "null", cost ) ), Entry::Act::Add );
			}
			return true;
		}

		return false;
	}

	bool trySummarize( const std::list< Record >::iterator& last ) {
		int count = 0, newCount = 0;
		int prefix = 1, minimalPrefix = 1;
		for ( ; prefix <= SUMMARY_DEPTH; prefix++ ) {
			newCount = wouldSummarize( last, prefix );
			if ( newCount == count ) {
				continue;
			}
			count = newCount;
			minimalPrefix = prefix;
		}

		return ( count > 1 ) && summarize( last, minimalPrefix );
	}

	void checkSumarizing() {
		for ( auto it = records.begin(); it != records.end(); it++ ) {
			if ( it->toDelete() ) {
				addToChanges( *it, Entry::Act::Remove );
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
			trySummarize( it );
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

		if ( changed ) {
			addToChanges( rec, Entry::Act::Add );
		}

		return changed;
	}

	bool removeRecord( const Record& rec ) {
		bool summarized = false;
		if ( rec.hasGateway() && rec.getGwName() != "null" ) {
			auto r = search( rec );
			if ( !r ) {
				return false;
			}
			summarized = r->isSummarized();
			auto first = r->getGwName();
				records.remove( *r );
				checkSummaryAndUpdate( *r, summarized, first );
				return true;
			}
			checkSummaryAndUpdate( *r, summarized, first );
			return false;
		}

		std::size_t size = records.size();
		records.remove_if( [ &rec, &sumarized, this ]( const Record& r ) {
			if ( rec.hasSameNetwork( r ) ) {
				summarized |= r.isSummarized();
				addToChanges( r, Operation::Remove );
				return true;
			}
			return false;
		} );

		if ( summarized )
			checkSumarizing();

		return size != records.size();
	}

	int recForIfOrSummarized( Netif* n ) const {
		if ( !n )
			return 0;

		return std::accumulate( records.begin(), records.end(), 0, [ &n ]( int acc, const Record& r ) {
			return ( r.getGwName() == n->getName() || r.isSummarized() || ( r.ip == Ip6Addr( "::" ) && r.mask == 0 ) )
                   ? acc + 1 : acc;
		} );
	}

	int changesIfless( Netif* n ) const {
		int count = 0;
		for ( const auto& [ act, rec ] : changes ) {
			if ( ( n && rec.getGwName() == n->getName() ) || ( rec.getCost() == 0 && rec.mask == 0 ) || rec.isSummarized() )
				count++;
		}

		return count;
	}
	
	void addToChanges( const Record& rec, Entry::Act action ) {
		if ( rec.getCost() == 0 && rec.mask == 0 )
			return;
		if ( action == Entry::Act::Remove ) {
			changes.push_back( { action, Record( rec.ip, rec.mask, Gateway( rec.getGwName().c_str(), rec.getCost() ) ) } );
			for ( auto s : rec.sumarizing )
				changes.push_back( { Entry::Act::Add, Record( s->ip, s->mask, s->getGateway() ) } );
		} else {
			changes.push_back( { action, Record( rec.ip, rec.mask, Gateway( rec.getGwName().c_str(), rec.getCost() ) ) } );
			for ( auto s : rec.sumarizing )
				changes.push_back( { Entry::Act::Remove, Record( s->ip, s->mask, s->getGateway() ) } );
		}
	}

	Action onRRPmsg( rofi::hal::PBuf packet, Netif* n ) {
		bool changed = false;
		Command cmd  = as< Command >( packet.payload() );
		int count = static_cast< int >( as< uint8_t >( packet.payload() + 1) );
		auto data = packet.payload() + 2;

		if ( isHello( cmd ) && !n->isActive() )
			n->setActive( true );

		n->setStub( cmd == Command::Stubby );

		for ( int i = 0; i < count; i++ ) {
			Ip6Addr ip        = as< Ip6Addr >( data );
			uint8_t mask      = as< uint8_t >( data + Ip6Addr::size() );
			Cost cost         = as< Cost >( data + Ip6Addr::size() + 1 );
			Entry::Act action = as< Entry::Act >( data + Ip6Addr::size() + 1 + sizeof( Cost ) );
			if ( action == Entry::Act::Add )
				changed |= addRecord( Record( ip, mask, Gateway( n->getName().c_str(), cost ) ) );
			else // action == Entry::Act::Remove
				changed |= removeRecord( Record( ip, mask, Gateway( n->getName().c_str(), cost ) ) );
			data += Entry::size();
		}

		if ( isResponse( cmd ) )
			counter--;

		if ( cmd == Command::Hello )
			return Action::OnHello;

		if ( cmd == Command::Sync ) // I'm gateway to some stub -> do nothing or resend to my
			return Action::Nothing; // output interface in case I'm stub too

		return ( changed && cmd != Command::Sync ) ? ( isCall( cmd ) ? Action::BroadcastRespond : Action::BroadcastCall )
		                                           : ( isCall( cmd ) ? Action::Respond          : Action::Nothing       );
	}

	bool hasOneOutOrStub() const {
		std::string name = "";
		bool sameNames = true;
		bool noCycles  = true;
		for ( const auto& rec : records ) {
			if ( rec.getCost() == 0 && rec.mask != 0 )        // it's loopback -> skip it
				continue;
			if ( rec.isStub() )                               // it's stub -> skip it
				continue;
			if ( rec.getGwName() == "null" )                  // it's summarizing -> skip it
				continue;
			if ( stub && rec.getGwName() == stub->getName() ) // it's to stub out -> skip it
				continue;
			if ( name == "" )
				name = rec.getGwName();
			else
				sameNames &= name == rec.getGwName();
			noCycles = rec.singlePath();
		}

		return name != "" && sameNames && noCycles;
	}

	bool shouldBeStub() const {
		return isSynced() && hasOneOutOrStub();
	}

	Netif* getStubbyIf() {
		Netif* n = nullptr;
		for ( const auto& r : records ) {
			if ( r.getCost() == 0 )
				continue;
			auto tmp = netif_find( r.getGwName().c_str() );
			n = static_cast< Netif* >( tmp ? tmp->state : nullptr );
			break;
		}
		return n;
	}

	bool isSynced() const {
		return counter == 0;
	}

	void makeStub() {
		stub = getStubbyIf();
		if ( stub ) {
			removeForIf( stub );
			setDefaultGW( stub );
		}
	}

	void destroyStub() {
		stub = nullptr;
		for ( auto& rec : records ) {
			Netif* n = static_cast< Netif* >( netif_find( rec.getGwName().c_str() ) );
			if ( n )
				n->setStub( false );
		}
		removeDefaultGW( stub );
	}

	friend std::ostream& operator<<( std::ostream& o, const RTable& rt );
};

std::ostream& operator<<( std::ostream& o, const RTable& rt ) {
	for ( const auto& r : rt.records ) {
		o << r;
	}

	if ( rt.isStub() )
		o << "stub node (output interface: " << rt.stub->getName() << ")\n";

	return o;
}

} // namespace rofinet
