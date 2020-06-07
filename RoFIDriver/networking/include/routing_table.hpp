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

#define STUB           1
#define AUTOSUMMARY    1
#define SUMMARY_DEPTH  1

using Cost = uint8_t;

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

        finishedSumarizing = sumarizing.size() <= 1;
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

    bool merge( const Record& r ) {
        return op( &Record::_merge, r );
    }

    bool disjoin( const Record& r ) {
        return op( &Record::_disjoin, r );
    }

    bool remove( const std::string& str ) {
        Record tmp( ip, mask, Gateway( str.c_str(), getCost() ) );
        return op( &Record::_remove, tmp );
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
        int size = 0;
        for ( const auto& g : gws ) {
            if ( strcmp( g.name, "null" ) == 0 )
                continue;
            size++;
        }
        return size <= 1;
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
    Record* summarized = nullptr;
    std::vector< Record* > sumarizing;

    bool op( const std::function< void( Record*, const Record& ) >& f, const Record& r ) {
        if ( !this->hasSameNetwork( r ) )
            return false;

        std::size_t size = gws.size();
        auto first       = gws.front();

        std::bind( f, this, r )(); // invoke immediately

        update( first, size );
        return size != gws.size();
    }

    void _merge( const Record& r ) {
        if ( getCost() == 0 ) // do not merge with loopback
            return;
        std::list< Gateway > toMerge = r.gws;
        gws.merge( toMerge, []( const Gateway& g1, const Gateway& g2 ) {
            if ( std::string( g1.name ) == "null" )
                return false; // null will be always last

            return g1.cost <= g2.cost;
        } );

        for ( auto it = gws.begin(); it != gws.end(); it++ ) {
            auto current = it;
            while ( current != gws.end() ) {
                current++;
                if ( strcmp( current->name, it->name ) == 0 )
                    current = gws.erase( current );
            }
        }
    }

    void _disjoin( const Record& r ) {
        for ( auto g : r.gws ) {
            gws.remove_if( [ &g ]( const Gateway& gw ) {
                return strcmp( g.name, gw.name ) == 0;
            } );
        }
    }

    void _remove( const Record& r ) {
        std::string name = r.getGwName();
        std::size_t size = gws.size();
        gws.remove_if( [ &name ]( const Gateway& g ) { return strcmp( name.c_str(), g.name ) == 0; } );
    }

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
        o << "\t" << flag << g.name << " [" << static_cast< int >( g.cost ) << "]"
            << ( r.isStub() ? " stubby" : "      " ) << "\n";
        flag = ' ';
    }
    return o;
}


class RTable {
public:
    enum class Command : uint8_t { Call = 0, Response = 1, Stubby = 2, Hello = 3, HelloResponse = 4, Sync = 5 };
    enum class Action  : int     { RespondToAll = 0, CallToAll = 1, Respond = 2, Nothing = 3, OnHello = 4
                                 , HelloToAll = 5 };

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
                addToChanges( r, Operation::Remove );

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

    bool isToAll( Action act ) const {
        return act == Action::CallToAll || act == Action::RespondToAll;
    }

    void clearChanges() {
        changes.clear();
    }

    enum class Operation : bool { Add = true, Remove = false };

    struct Entry {
        Ip6Addr ip;
        uint8_t mask;
        Cost cost;
        Operation action;

        static int size() {
            return sizeof( Operation ) + Ip6Addr::size() + 1 + sizeof( Cost );
        }

    };

    PBuf createRRPhello( Netif* n, Command cmd = Command::Hello ) {
        int count   = records.size() - recForIfOrSummarized( n );
        auto packet = PBuf::allocate( 2 + count * Entry::size() );
        as< Command >( packet.payload() )     = cmd;
        as< uint8_t >( packet.payload() + 1 ) = static_cast< uint8_t >( count );

        n->setStub( cmd == Command::Stubby || cmd == Command::Sync );

        auto data = packet.payload() + 2;
        for ( const auto& rec : records ) {
            if ( n && rec.getGwName() == n->getName() )
                continue;
            if ( rec.isSummarized() )
                continue; // skip summarized records
            if ( rec.ip == Ip6Addr( "::" ) && rec.mask == 0 )
                continue;
            addEntry( data, rec, Operation::Add );
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
        if ( isSynced() && action != Action::OnHello ) {
            if ( !isStub() && shouldBeStub() ) {
                changed = true;
                makeStub();
            } else if ( isStub() && !shouldBeStub() ) {
                changed = true;
                destroyStub();
                return Action::HelloToAll;
            }
        }
        #endif

        return !changed ? action : ( isStub() ? Action::RespondToAll : Action::CallToAll );
    }

    Netif* getStubOut() const {
        return stub;
    }

    bool isSynced() const {
        return counter == 0;
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

private:
    std::list< Record > records;
    Netif* stub = nullptr;
    volatile int counter = 0;
    std::vector< std::pair< Operation, Record > > changes;

    void addEntry( uint8_t* data, const Record& rec, Operation act = Operation::Add ) {
        as< Ip6Addr >( data )                       = rec.ip;
        as< uint8_t >( data + Ip6Addr::size() )     = rec.mask;
        as< Cost    >( data + Ip6Addr::size() + 1 ) = rec.getCost() + 1;
        as< Operation >( data + Ip6Addr::size() + 1 + sizeof( Cost ) ) = act;
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
                addToChanges( *it, Operation::Add );
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
            if ( it->toDelete() || it->gws.size() == 0 ) {
                addToChanges( *it, Operation::Remove );
                it = records.erase( it );
            }
        }
    }

    bool addSorted( const Record& rec ) {
        bool sumarized = false;
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

        #if AUTOSUMMARY
         if ( records.size() > 1 ) {
            uint8_t mask = it->mask;
            while ( it != records.begin() && it->mask == mask ) {
                if ( trySummarize( it ) )
                    sumarized = true;
                it--;
            }
        }
        #endif

        return sumarized;
    }

    bool addRecord( const Record& rec ) {
        bool changed = false;
        bool sumarized = false;
        auto r = search( rec );
        if ( r ) {
            if ( *r != rec ) {
                Gateway tmp = r->getGateway();
                changed = r->merge( rec );
                if ( tmp != r->getGateway() ) {
                    addToChanges( Record( r->ip, r->mask, tmp ), Operation::Remove );
                    changed = true;
                }
            }
        } else {
            sumarized = addSorted( rec );
            changed = true;
        }

        if ( changed && !sumarized ) {
            addToChanges( rec, Operation::Add );
        }

        return changed;
    }

    void checkSummaryAndUpdate( const Record& r, bool summarized, const std::string& str ) {
        if ( summarized )
            checkSumarizing();

        if ( r.getGwName() != str ) { // if the route was updated, remove the old and add the new one
            addToChanges( Record( r.ip, r.mask, Gateway( str.c_str(), 0 ) ), Operation::Remove );
        }
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
            if ( r->disjoin( rec ) && !r->hasGateway() ) {
                addToChanges( *r, Operation::Remove );
                records.remove( *r );
                checkSummaryAndUpdate( *r, summarized, first );
                return true;
            }
            checkSummaryAndUpdate( *r, summarized, first );
            return false;
        }

        std::size_t size = records.size();
        records.remove_if( [ &rec, &summarized, this ]( const Record& r ) {
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
        for ( const auto& [ act, rec ] : changes ) { // criterias are described in createRRPmsg
            if ( n && ( n->getName() == rec.getGwName() || n->isStub() ) )
                count++;
            else if ( rec.getCost() == 0 && rec.mask == 0 )
                count++;
            else if ( rec.isSummarized() )
                count++;
        }

        return count;
    }

    void addToChanges( const Record& rec, Operation action ) {
        if ( rec.getCost() == 0 && rec.mask == 0 )
            return;
        if ( action == Operation::Remove ) {
            changes.push_back( { action, Record( rec.ip, rec.mask, Gateway( rec.getGwName().c_str(), rec.getCost() ) ) } );
            for ( auto s : rec.sumarizing )
                if ( s )
                    changes.push_back( { Operation::Add, Record( s->ip, s->mask, s->getGateway() ) } );
        } else {
            changes.push_back( { action, Record( rec.ip, rec.mask, Gateway( rec.getGwName().c_str(), rec.getCost() ) ) } );
            for ( auto s : rec.sumarizing )
                if ( s )
                    changes.push_back( { Operation::Remove, Record( s->ip, s->mask, s->getGateway() ) } );
        }
    }

    Action onRRPmsg( rofi::hal::PBuf packet, Netif* n ) {
        bool changed = false;
        Command cmd  = as< Command >( packet.payload() );
        int count = static_cast< int >( as< uint8_t >( packet.payload() + 1) );
        auto data = packet.payload() + 2;

        if ( isHello( cmd ) && !n->isActive() )
            n->setActive( true );

        n->setStub( cmd == Command::Stubby || cmd == Command::Sync );

        for ( int i = 0; i < count; i++ ) {
            Ip6Addr ip        = as< Ip6Addr >( data );
            uint8_t mask      = as< uint8_t >( data + Ip6Addr::size() );
            Cost cost         = as< Cost >( data + Ip6Addr::size() + 1 );
            Operation action  = as< Operation >( data + Ip6Addr::size() + 1 + sizeof( Cost ) );
            if ( action == Operation::Add )
                changed |= addRecord( Record( ip, mask, Gateway( n->getName().c_str(), cost ) ) );
            else // action == Operation::Remove
                changed |= removeRecord( Record( ip, mask, Gateway( n->getName().c_str(), cost ) ) );
            data += Entry::size();
        }

        if ( isResponse( cmd ) )
            counter--;

        if ( cmd == Command::Hello )
            return Action::OnHello;

        if ( cmd == Command::Sync ) // I'm gateway to some stub -> do nothing or resend to my
            return Action::Nothing; // output interface in case I'm stub too

        return ( changed && cmd != Command::Sync ) ? ( isCall( cmd ) ? Action::RespondToAll : Action::CallToAll )
                                                   : ( isCall( cmd ) ? Action::Respond      : Action::Nothing   );
    }

    bool hasOneOutOrStub() const {
        std::string name = "";
        bool sameNames = true;
        bool noCycles  = true;
        for ( const auto& rec : records ) {
            noCycles &= rec.singlePath();
            if ( !noCycles )
                return false;
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
        }

        return name != "" && sameNames;
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

    void makeStub() {
        stub = getStubbyIf();
        if ( stub ) {
            removeForIf( stub );
            setDefaultGW( stub );
        }
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
