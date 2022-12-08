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
 * Proof-of-concept routing protocol using IP layer only with periodic messages.
*/
class SimplePeriodic : public Protocol {
    std::vector< std::reference_wrapper< const Interface > > _managedInterfaces;
    std::set< std::string > _interfaceWithCb;

    using Update = std::pair< Protocol::Route, RoutingTable::Record >;
    std::vector< Update > _updates;
    int _period;

    PBuf _createMsg( const std::string& interfaceName ) {
        auto records = routingTableCB();

        int count = 0;
        for ( auto& r : records ) {
            auto g = r.best();
            if ( !g || g->name() == interfaceName )
                continue;
            count++;
        }

        auto packet = PBuf::allocate( 2 + count * ( Ip6Addr::size() + 3 ) );
        as< uint16_t >( packet.payload() ) = static_cast< uint16_t >( count );
        auto* data = packet.payload() + 2;

        for ( auto& r : records ) {
            auto g = r.best();
            if ( !g || g->name() == interfaceName )
                continue;

            as< Ip6Addr >( data ) = r.ip();
            as< uint8_t >( data + Ip6Addr::size() ) = r.mask();
            as< uint16_t >( data + Ip6Addr::size() + 1 ) = static_cast< uint16_t >( g->cost() );
            data += Ip6Addr::size() + 3;
        }

        return packet;
    }

    void _periodicUpdates( const Interface& i, std::function< void ( PBuf&& ) > f, void* args ) {
        f( std::move( _createMsg( i.name() ) ) );
        rofi::hal::RoFI::wait( _period, [ =, this, _i = &i ]() { _periodicUpdates( *_i, f, args ); } );
    }

public:
    SimplePeriodic() : SimplePeriodic( 4000 ) {};
    SimplePeriodic( int period ) : _period( period ) {};

    virtual bool onMessage( const std::string& interfaceName, rofi::hal::PBuf packetWithHeader ) override {
        auto packet = PBuf::own( pbuf_free_header( packetWithHeader.release(), IP6_HLEN ) );
        int count = static_cast< int >( as< uint16_t >( packet.payload() ) );
        auto data = packet.payload() + 2;

        auto updatesCount = _updates.size();

        // Remove everything we learned from this interface via this protocol
        for ( const auto& l : routingTableCB() ) {
            for ( const auto& g : l.gateways() ) {
                if ( g.name() != interfaceName )
                    continue;
                _updates.push_back( Update{ Route::RM, { l.ip(), l.mask(), interfaceName, g.cost(), g.learnedFrom() } } );
            }
        }

        for ( int i = 0; i < count; i++ ) {
            Ip6Addr ip = as< Ip6Addr >( data );
            auto mask  = as< uint8_t >( data + Ip6Addr::size() );
            auto cost  = as< uint16_t >( data + Ip6Addr::size() + 1 ) + 10;
            RoutingTable::Record rec{ ip, mask, interfaceName, cost };

            Update update{ Route::ADD, rec };
            _updates.push_back( update );

            data += Ip6Addr::size() + 3;
        }

        return updatesCount != _updates.size();
    }

    virtual bool afterMessage( const Interface& i, std::function< void ( PBuf&& ) > f, void* args ) override {
        /* Once the function is called on given interface, it saves the function and sets up the periodic
         * updates according to the given (or default) period. The function itself is then called withing
         * the function handling periods. Also, notice that the function is NEVER called withing this
         * function AND that this function does nothing once the periodic updates are set up! */
        if ( !_interfaceWithCb.contains( i.name() ) ) {
            _interfaceWithCb.emplace( i.name() );
            _periodicUpdates( i, f, args );
        }

        return false;
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
            _updates.push_back( { Route::ADD, { ip, mask, interface.name(), 0 } } );
            return true;
        }

        return false;
    }

    virtual bool rmAddressOn( const Interface& interface, const Ip6Addr& ip, uint8_t mask ) override {
        if ( !ip.linkLocal() ) {
            Update rec{ Route::RM, { ip, mask, interface.name(), 0, this } };
            _updates.push_back( rec );
            return true;
        }

        return false;
    }

    virtual bool addInterface( const Interface& interface ) override {
        if ( manages( interface ) ) // interface is already managed
            return false;

        _managedInterfaces.push_back( std::ref( interface ) );
        bool added = false;
        for ( auto [ ip, mask ] : interface.getAddress() ) {
            added = addAddressOn( interface, ip, mask ) || added;
        }

        return true;
    }

    virtual bool removeInterface( const Interface& interface ) override {
        auto it = std::find_if( _managedInterfaces.begin(), _managedInterfaces.end()
                              , [ &interface ]( auto i ) { return interface == i; } );

        if ( it == _managedInterfaces.end() )
            return false;

        std::swap( *it, _managedInterfaces.back() );
        _managedInterfaces.pop_back();
        // add addresses into changes -> remove them from the table
        for ( auto [ ip, mask ] : interface.getAddress() ) {
            _updates.push_back( { Route::RM, { ip, mask, interface.name(), 0 } } );
        }

        return true;
    }

    virtual bool manages( const Interface& interface ) const override {
        return std::ranges::any_of( _managedInterfaces, [ &interface ]( const Interface& i ) {
            return interface == i;
        } );
    }

    virtual Ip6Addr address() const { return Ip6Addr( "ff02::a:b:b:af" ); };
    virtual std::string name() const { return "simple-periodic"; };

};

} // namespace rofi::net
