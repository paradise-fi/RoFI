#pragma once

#include "lwip++.hpp"
#include <networking/protocol.hpp>
#include <networking/interface.hpp>
#include <networking/routingTable.hpp>

#include <atoms/util.hpp>

#include <vector>
#include <map>
#include <set>

namespace rofi::net {

/**
 * Proof-of-concept routing protocol using IP layer only with reactive messages.
*/
class SimpleReactive : public Protocol {
    std::vector< std::reference_wrapper< const Interface > > _managedInterfaces;
    std::set< std::string > _interfaceWithCb;
    std::map< Interface::Name, Ip6Addr > _counterpartAddress;

    using Update = std::pair< Protocol::Route, RoutingTable::Record >;
    std::vector< Update > _updates;

    PBuf _createMsg( const std::string& interfaceName ) {
        auto records = routingTableCB();

        int count = 0;
        for ( auto& r : records ) {
            auto g = r.best();
            if ( !g || ( _sameCounterpart( g->name(), interfaceName ) && g->cost() != 0 ) )
                continue;
            count++;
        }

        auto packet = PBuf::allocate( 2 + count * ( Ip6Addr::size() + 3 ) );
        as< uint16_t >( packet.payload() ) = static_cast< uint16_t >( count );
        auto* data = packet.payload() + 2;

        for ( const auto& r : records ) {
            auto g = r.best();
            if ( !g || ( _sameCounterpart( g->name(), interfaceName ) && g->cost() != 0 ) )
                continue;

            as< Ip6Addr >( data ) = r.ip();
            as< uint8_t >( data + Ip6Addr::size() ) = r.mask();
            as< uint16_t >( data + Ip6Addr::size() + 1 ) = static_cast< uint16_t >( g->cost() );
            data += Ip6Addr::size() + 3;
        }

        return packet;
    }

    bool _addInterface( const Interface& interface ) {
        for ( auto [ ip, mask ] : interface.getAddress() ) {
            addAddressOn( interface, ip, mask );
        }

        return true;
    }

    bool _removeInterface( const Interface& interface ) {
        // remove all routes that has this interface as their gateway
        bool removed = false;
        auto records = routingTableCB();
        for ( const auto& rec : records ) {
            for ( const auto& g : rec.gateways() ) {
                if ( g.name() != interface.name() )
                    continue;

                _updates.push_back( { Route::RM
                                    , { rec.ip(), rec.mask(), interface.name(), g.cost(), g.learnedFrom() } } );
                removed = true;
            }
        }

        // remove record for the counterpart's interface
        _counterpartAddress.erase( interface.name() );

        return removed;
    }

    void _storeCounterpartAddress( const std::string& interfaceName, rofi::hal::PBuf& packet ) {
        auto* ip6hdr = static_cast< ip6_hdr * >( packet.get()->payload );
        Ip6Addr source( "::" );
        ip6_addr_copy_from_packed( source, ip6hdr->src );
        // TODO: It might be good to check if the records exists and maybe log the change?
        _counterpartAddress.insert( { interfaceName, source } );
    }

    bool _sameCounterpart( const Interface::Name recordSource, const Interface::Name outputInterface ) const {
        return recordSource == outputInterface
            || (   _counterpartAddress.contains( recordSource )
                && _counterpartAddress.contains( outputInterface )
                && (_counterpartAddress.at( recordSource ) == _counterpartAddress.at( outputInterface ) )
            );
    }

public:

    virtual bool onMessage( const std::string& interfaceName, rofi::hal::PBuf packetWithHeader ) override {
        _storeCounterpartAddress( interfaceName, packetWithHeader );
        auto packet = PBuf::own( pbuf_free_header( packetWithHeader.release(), IP6_HLEN ) );
        int count = static_cast< int >( as< uint16_t >( packet.payload() ) );
        auto data = packet.payload() + 2;

        bool somethingNew = false;
        auto records = routingTableCB();
        std::set< int > validRecords;

        for ( int i = 0; i < count; i++ ) {
            Ip6Addr ip  = as< Ip6Addr >( data );
            auto mask   = as< uint8_t >( data + Ip6Addr::size() );
            auto cost   = as< uint16_t >( data + Ip6Addr::size() + 1 ) + 5;
            RoutingTable::Record rec{ ip, mask, interfaceName, cost };

            // do we know about this route already?
            int index = -1;
            auto it = std::find_if( records.begin(), records.end(), [ &rec, this, &index ]( auto& r ) {
                index++;
                return r.compareNetworks( rec ) && r.contains( { rec.best()->name(), rec.best()->cost(), this } );
            } );
            if ( it == records.end() ) { // no, so we add it
                somethingNew = true;
                Update update{ Route::ADD, rec };
                _updates.push_back( update );
            } else {
                validRecords.insert( index );
            }

            data += Ip6Addr::size() + 3;
        }

        // remove those records, that were not used in the find above -- so they were not
        // among routes in the processed update message, and therefore are no longer valid
        for ( unsigned i = 0; i < records.size(); i++ ) {
            if ( validRecords.contains( i ) )
                continue;
            for ( auto& g : records[ i ].gateways() ) {
                if ( g.name() == interfaceName ) {
                    _updates.push_back( { Route::RM
                                        , { records[ i ].ip(), records[ i ].mask(), g.name(), g.cost(), this } } );
                    somethingNew = true;
                }
            }
        }

        return somethingNew;
    }

    virtual bool afterMessage( const Interface& i, std::function< void ( PBuf&& ) > f, void* /* args */ ) override {
        f( std::move( _createMsg( i.name() ) ) );
        return false;
    }

    virtual bool onInterfaceEvent( const Interface& interface, bool connected ) override {
        assert( manages( interface ) && "onInterfaceEvent within SimpleReactive got unmanaged interface" );

        bool res = false;
        if ( connected ) {
            res = _addInterface( interface );
            // add everything we know, to pass it to the new neighbour
            for ( auto& rec : routingTableCB() ) {
                // do not propagate routes from neighbours back to them
                if ( rec.best() && !_sameCounterpart( rec.best()->name(), interface.name() ) )
                    _updates.push_back( { Route::ADD
                                        , { rec.ip(), rec.mask(), rec.best()->name(), rec.best()->cost(), this } } );
            }
        } else {
            res = _removeInterface( interface );
        }

        return res;
    }

    virtual bool hasRouteUpdates() const override { return !_updates.empty(); }

    virtual std::vector< std::pair< Route, RoutingTable::Record > > getRouteUpdates() const override {
        return _updates;
    }

    virtual void clearUpdates() override {
        _updates.clear();
    }

    virtual bool addAddressOn( const Interface& interface, const Ip6Addr& ip, uint8_t mask ) override {
        if ( !ip.linkLocal() ) {
            // add it without mentioning the origin as we want all others to propagate it
            _updates.push_back( { Route::ADD, { ip, mask, interface.name(), 0 } } );
            return true;
        }

        return false;
    }

    virtual bool rmAddressOn( const Interface& interface, const Ip6Addr& ip, uint8_t mask ) override {
        if ( !ip.linkLocal() ) {
            Update rec{ Route::RM, { ip, mask, interface.name(), 0 } };
            _updates.push_back( rec );
            return true;
        }

        return false;
    }

    virtual bool addInterface( const Interface& interface ) override {
        if ( manages( interface ) ) // interface is already managed
            return false;

        _managedInterfaces.push_back( std::ref( interface ) );
        return _addInterface( interface );
    }

    virtual bool removeInterface( const Interface& interface ) override {
        auto it = std::find_if( _managedInterfaces.begin(), _managedInterfaces.end()
                              , [ &interface ]( const auto& i ) { return interface == i; } );

        if ( it == _managedInterfaces.end() )
            return false;

        std::swap( *it, _managedInterfaces.back() );
        _managedInterfaces.pop_back();

        return _removeInterface( interface );
    }

    virtual bool manages( const Interface& interface ) const override {
        return std::ranges::any_of( _managedInterfaces, [ &interface ]( const Interface& i ) {
            return interface == i;
        } );
    }

    virtual Ip6Addr address() const { return Ip6Addr( "ff02::a:b:b:ae" ); };
    virtual std::string name() const { return "simple-reactive"; };

};

} // namespace rofi::net
