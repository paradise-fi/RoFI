#pragma once

#include "lwip++.hpp"

#include <networking/protocol.hpp>
#include <atoms/util.hpp>
#include <algorithm>
#include <set>
#include <functional>

namespace rofinet {

class RRP : public Protocol {
    using Update = std::pair< Route, RoutingTable::Record >;

    enum class Command : uint8_t { Call = 0, Response = 1, Stubby = 2, Hello = 3, HelloResponse = 4, Sync = 5 };
    enum class Operation : bool { Add = true, Remove = false };

    static const int EntrySize = Ip6Addr::size() + 2 + sizeof( Operation );

    std::vector< std::reference_wrapper< const Interface > > _managedInterfaces;
    std::vector< Update > _updates;
    // interface name, routing update, record -> on which interface the Protocol::Route originated
    std::vector< std::pair< const std::string, Update > > _toSendExcept;
    std::map< const std::string, std::vector< Update > > _toSendFrom;
    std::map< std::string, Command > _interfacesCommands;
    unsigned _processedAfterMsg = 0;

    bool amIStubby() const {
        auto records = routingTableCB();
        if ( records.empty() )
            return false;

        auto outInterface = records.front().best()->name();
        return std::ranges::all_of( records, [ &outInterface ]( const RoutingTable::Record& r ) {
            return std::ranges::all_of( r.gateways(), [ &outInterface ]( const RoutingTable::Gateway& gw ) {
                return gw.name() == outInterface;
            } );
        } );
    }

    std::optional< Command > getResponseCmd( Command received, bool newRecords ) const {
        if ( received == Command::Hello )
            return { Command::HelloResponse };

        // TODO: Rewrite this insanity
        if ( newRecords ) {
            if ( received == Command::Call ) {
                if ( amIStubby() ) {
                    return { Command::Stubby };
                } else {
                    return { Command::Response };
                }
            }
        } else {
            if ( received == Command::Call ) {
                return { amIStubby() ? Command::Stubby : Command::Response };
            } else if ( amIStubby() ) {
                return { Command::Sync };
            }
        }

        return std::nullopt;
    }

    void setOtherCommands( Command responseCmd, const std::string& interfaceName, bool gotCall ) {
        _interfacesCommands[ interfaceName ] = responseCmd;
        if ( gotCall ) {
            for ( auto& i : _managedInterfaces ) {
                if ( i.get().name() == interfaceName )
                    continue;
                _interfacesCommands[ i.get().name() ] = Command::Call;
            }
        }
    }

    Result update( rofi::hal::PBuf packet, const std::string& interfaceName ) {
        Command command = as< Command >( packet.payload() );
        int count = static_cast< int >( as< uint8_t >( packet.payload() + 1 ) );
        auto data = packet.payload() + 2;

        for ( int i = 0; i < count; i++ ) {
            Ip6Addr ip  = as< Ip6Addr >( data );
            auto mask   = as< uint8_t >( data + Ip6Addr::size() );
            auto cost   = as< uint8_t >( data + Ip6Addr::size() + 1 );
            auto action = as< Operation >( data + Ip6Addr::size() + 2 );
            RoutingTable::Record rec{ ip, mask, interfaceName, cost };

            Update update{ action == Operation::Add ? Route::ADD : Route::RM, rec };
            _updates.push_back( update );
            auto it = std::ranges::find_if( _toSendExcept, [ &update ]( const auto& r ) {
                return r.second == update;
            } );
            if ( it == _toSendExcept.end() )
                _toSendExcept.push_back( { interfaceName, update } );

            data += EntrySize;
        }

        auto responseCmd = getResponseCmd( command, count > 0 );
        if ( responseCmd )
            setOtherCommands( *responseCmd, interfaceName, command == Command::Call || command == Command::Hello );
        else
            _interfacesCommands.erase( interfaceName );

        return count > 0 ? Result::ROUTE_UPDATE : Result::NOTHING_NEW;
    }

    PBuf createMsg( const std::string& interfaceName, Command cmd ) {
        auto fromOtherIfs = [ &interfaceName ]( auto& p ) { return std::get< 0 >( p ) != interfaceName; };

        int count = static_cast< int >( std::ranges::count_if( _toSendExcept, fromOtherIfs ) )
                + static_cast< int >( _toSendFrom.contains( interfaceName ) ? _toSendFrom[ interfaceName ].size() : 0 );
        auto packet = PBuf::allocate( 2 + count * EntrySize );
        as< Command >( packet.payload() ) = cmd;
        as< uint8_t >( packet.payload() + 1 ) = static_cast< uint8_t >( count );
        auto data = packet.payload() + 2;

        // TODO: join these two for cycles
        for ( const auto& p : _toSendExcept | std::views::filter( fromOtherIfs ) ) {
            const RoutingTable::Record& record = p.second.second;
            assert( record.best().has_value() && "Record without a gateway should NOT be propagated" );
            as< Ip6Addr >( data ) = record.ip();
            as< uint8_t >( data + Ip6Addr::size() ) = record.mask();
            as< uint8_t >( data + Ip6Addr::size() + 1 ) = static_cast< uint8_t >( 1 + record.best()->cost() );
            as< Operation >( data + Ip6Addr::size() + 2 ) = p.second.first == Route::ADD ? Operation::Add : Operation::Remove;
            data += EntrySize;
        }

        for ( const auto& p : _toSendFrom[ interfaceName ] ) {
            const RoutingTable::Record& record = p.second;
            assert( record.best().has_value() && "Record without a gateway should NOT be propagated" );
            as< Ip6Addr >( data ) = record.ip();
            as< uint8_t >( data + Ip6Addr::size() ) = record.mask();
            as< uint8_t >( data + Ip6Addr::size() + 1 ) = static_cast< uint8_t >( 1 + record.best()->cost() );
            as< Operation >( data + Ip6Addr::size() + 2 ) = p.first == Route::ADD ? Operation::Add : Operation::Remove;
            data += EntrySize;
        }

        return packet;
    }

public:
    using Cost = uint8_t;

    virtual ~RRP() = default;

    RoutingTable::Cost convertCost( Cost c ) { return static_cast< RoutingTable::Cost >( c ); }
    Cost convertCost( RoutingTable::Cost c ) { return static_cast< Cost >( c ); }

    virtual void afterMessage( const Interface& i, std::function< void ( PBuf&& ) > f, void* /* args */ ) override {
        if ( _interfacesCommands.contains( i.name() ) )
            f( std::move( createMsg( i.name(), _interfacesCommands[ i.name() ] ) ) );
        else
            std::cout << "skipping afterMessage for " << i.name() << std::endl;

        _toSendFrom[ i.name() ].clear();

        // afterMessage was called on all interfaces (or more precisely callCount == #interfaces)
        // TODO: Think about securing that it was called once for each...
        _processedAfterMsg++;
        if ( _processedAfterMsg == _managedInterfaces.size() - 1 ) {
            _toSendExcept.clear();
            _updates.clear();
        }
    }

    virtual Result onMessage( const std::string& interfaceName, rofi::hal::PBuf packet ) override {
        auto packetWithoutHeader = pbuf_free_header( packet.release(), IP6_HLEN );
        return update( std::move( rofi::hal::PBuf::own( packetWithoutHeader ) ), interfaceName );
    }

    virtual std::vector< std::pair< Route, RoutingTable::Record > > getRTEUpdates() const override {
        return _updates;
    }

    virtual void clearUpdates() override {
        _updates.clear();
    }

    virtual bool addAddressOn( const Interface& interface, const Ip6Addr& ip, uint8_t mask ) override {
        if ( !ip.linkLocal() ) {
            Update rec{ Route::ADD, { ip, mask, interface.name(), 0 } };
            _updates.push_back( rec );
            auto it = std::ranges::find_if( _toSendExcept, [ &rec ]( const auto& r ) {
                return r.second == rec;
            } );
            if ( it == _toSendExcept.end() )
                _toSendExcept.push_back( { interface.name(), rec } );
            return true;
        }

        return false;
    }

    virtual bool rmAddressOn( const Interface& interface, const Ip6Addr& ip, uint8_t mask ) override {
        if ( !ip.linkLocal() ) {
            Update rec{ Route::RM, { ip, mask, interface.name(), 0 } };
            _updates.push_back( rec );
            auto it = std::ranges::find_if( _toSendExcept, [ &rec ]( const auto& r ) {
                return r.second == rec;
            } );
            if ( it == _toSendExcept.end() )
                _toSendExcept.push_back( { interface.name(), rec } );
            return true;
        }

        return false;
    }

    virtual bool addInterface( const Interface& interface ) override {
        // TODO: add asserts
        if ( manages( interface ) ) // interface is already managed
            return false;

        auto learned = routingTableCB();

        for ( auto l : learned ) {
            auto it = std::ranges::find_if( _toSendFrom[ interface.name() ], [ &l ]( const Update& r ) {
                return r.second == l;
            } );
            if ( it == _toSendFrom[ interface.name() ].end() )
                _toSendFrom[ interface.name() ].push_back( { Route::ADD, l } );
        }

        _interfacesCommands.emplace( interface.name(), Command::Hello );

        _managedInterfaces.push_back( std::ref( interface ) );
        bool added = false;
        for ( auto [ ip, mask ] : interface.getAddress() ) {
            added = addAddressOn( interface, ip, mask ) || added;
        }

        return added || !std::ranges::empty( learned );
    }

    virtual bool removeInterface( const Interface& interface ) override {
        // TODO: add asserts
        auto it = std::find_if( _managedInterfaces.begin(), _managedInterfaces.end()
                              , [ &interface ]( auto i ) { return interface == i; } );
        
        if ( it == _managedInterfaces.end() )
            return false;

        std::swap( *it, _managedInterfaces.back() );
        _managedInterfaces.pop_back();
        // add addresses into changes
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

    virtual rofi::hal::Ip6Addr getIp() const override { return Ip6Addr( "ff02::1f" ); }
    virtual std::string name() const override { return "rrp"; }
    virtual std::string info() const override {
        std::string str = Protocol::info();
        return str + "; stub: " + ( amIStubby() ? "yes" : "no" );
    }
};

}
