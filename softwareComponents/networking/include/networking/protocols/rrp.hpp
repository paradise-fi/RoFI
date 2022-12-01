#pragma once

#include "lwip++.hpp"

#include <networking/protocol.hpp>
#include <atoms/util.hpp>
#include <algorithm>
#include <set>
#include <functional>

namespace rofi::net {

class RRP : public Protocol {
    using Update = std::pair< Route, RoutingTable::Record >;

    enum class Command : uint8_t { Call = 0, Response = 1, Stubby = 2, Hello = 3, HelloResponse = 4, Sync = 5 };
    enum class Operation : bool { Add = true, Remove = false };

    static const int EntrySize = Ip6Addr::size() + 2 + sizeof( Operation );

    static std::string cmdStr( Command c ) {
        switch ( c ) {
            case Command::Call:
                return "Call";
            case Command::Response:
                return "Response";
            case Command::HelloResponse:
                return "Hello response";
            case Command::Hello:
                return "Hello";
            case Command::Sync:
                return "Sync";
            case Command::Stubby:
                return "Stubby";
        }

        return "";
    }

    static std::optional< bool > isVirtualInterface( const Interface::Name& ifname ) {
        auto* ptr = netif_find( ifname.c_str() );
        if ( !ptr )
            return std::nullopt;

        auto* interface = reinterpret_cast< Interface* >( ptr->state );
        if ( !interface )
            return { false };
        
        return { interface->isVirtual() };
    }

    RoutingTable::Record defaultGatewayVia( const Interface::Name& ifname ) const {
        return RoutingTable::Record( Ip6Addr( "::" ), 0, ifname, 0, this );
    }

    std::vector< std::reference_wrapper< const Interface > > _managedInterfaces;
    std::set< Interface::Name > _stubInterfaces;
    std::vector< Update > _updates;
    // interface name, routing update, record -> on which interface the Protocol::Route originated
    std::vector< std::pair< Interface::Name, Update > > _toSendExcept;
    std::map< Interface::Name, std::vector< Update > > _toSendFrom;
    std::map< Interface::Name, Command > _interfacesCommands;
    bool _stub = false;

    std::optional< Interface::Name > getPhysicalGw( const RoutingTable::Records& records ) const {
        for ( const auto& rec : records ) {
            if ( !rec.best() || isVirtualInterface( rec.best()->name() ).value_or( false ) )
                continue;
            if ( _stubInterfaces.contains( rec.best()->name() ) )
                continue;
            
            return { rec.best()->name() };
        }

        return std::nullopt;
    }

    bool amIStubby() const {
        auto records = routingTableCB();
        if ( records.empty() )
            return false;

        // ToDo: Make this more fail proof -- you need to get any _physical_ output netif.
        auto outInterface = getPhysicalGw( records );
        if ( !outInterface.has_value() )
            return false;

        bool oneWayOut = std::ranges::all_of( records, [ &outInterface ]( const RoutingTable::Record& r ) {
            return std::ranges::all_of( r.gateways(), [ &outInterface ]( const RoutingTable::Gateway& gw ) {
                return gw.name() == *outInterface || isVirtualInterface( gw.name() );
            } );
        } );

        bool isSynced = std::ranges::all_of( _interfacesCommands, []( const auto& keyVal ) {
            auto val = keyVal.second;
            return ( val != Command::Call && val != Command::Hello ) || isVirtualInterface( keyVal.first );
        } );

        return oneWayOut && isSynced;
    }

    std::optional< Command > getResponseCmd( Command received, bool newRecords ) const {
        if ( received == Command::Hello )
            return { Command::HelloResponse };

        // TODO: Rewrite this insanity
        if ( newRecords ) {
            if ( received == Command::Call ) {
                if ( _stub ) {
                    return { Command::Stubby };
                } else {
                    return { Command::Response };
                }
            }
        } else {
            if ( received == Command::Call ) {
                return { _stub ? Command::Stubby : Command::Response };
            } else if ( _stub ) {
                return { Command::Sync };
            }
        }

        return std::nullopt;
    }

    void setOtherCommands( Command responseCmd, const Interface::Name& interfaceName, bool gotCall ) {
        _interfacesCommands[ interfaceName ] = responseCmd;
        if ( gotCall ) {
            for ( auto& i : _managedInterfaces ) {
                if ( i.get().name() == interfaceName )
                    continue;
                _interfacesCommands[ i.get().name() ] = Command::Call;
            }
        }

        if ( !_stub && amIStubby() ) {
            for ( auto& i : _managedInterfaces ) {
                if ( i.get().name() == interfaceName )
                    continue;
                _interfacesCommands[ i.get().name() ] = Command::Hello;
            }
        }
    }

    Result update( rofi::hal::PBuf packet, const std::string& interfaceName ) {
        Command command = as< Command >( packet.payload() );

        if ( command == Command::Stubby || command == Command::Sync )
            _stubInterfaces.insert( interfaceName );
        else
            _stubInterfaces.erase( interfaceName );

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
                // TODO: Do we want to compare records or their networks?
                return r.second.second.compareNetworks( update.second );
            } );
            if ( it == _toSendExcept.end() )
                _toSendExcept.push_back( { interfaceName, update } );

            data += EntrySize;
        }

        // if we're stub and we got message from aa new interface, we destroy the stub
        auto mif = getPhysicalGw( routingTableCB() );
        if ( _stub && mif && *mif != interfaceName ) {
            _stub = false;
            _updates.push_back( { Route::RM, defaultGatewayVia( *mif ) } );
        }

        auto responseCmd = getResponseCmd( command, count > 0 );
        if ( responseCmd )
            setOtherCommands( *responseCmd, interfaceName, command == Command::Call || command == Command::Hello );
        else
            _interfacesCommands.erase( interfaceName );

        return count > 0 ? Result::ROUTE_UPDATE : Result::NO_UPDATE;
    }

    PBuf createMsg( const std::string& interfaceName, Command cmd ) {
        auto fromOtherIfs = [ &interfaceName ]( auto& p ) { return std::get< 0 >( p ) != interfaceName; };

        int count = static_cast< int >( std::ranges::count_if( _toSendExcept, fromOtherIfs ) )
            + static_cast< int >( _toSendFrom.contains( interfaceName ) ? _toSendFrom[ interfaceName ].size() : 0 );

        auto packet = PBuf::allocate( 2 + count * EntrySize );
        as< Command >( packet.payload() ) = cmd;
        as< uint8_t >( packet.payload() + 1 ) = static_cast< uint8_t >( count );
        auto data = packet.payload() + 2;

        // TODO: join these two for cycles?
        for ( const auto& p : _toSendExcept | std::views::filter( fromOtherIfs ) ) {
            const RoutingTable::Record& record = p.second.second;
            assert( record.best().has_value() && "Record without a gateway should NOT be propagated" );
            as< Ip6Addr >( data ) = record.ip();
            as< uint8_t >( data + Ip6Addr::size() ) = record.mask();
            as< uint8_t >( data + Ip6Addr::size() + 1 ) = static_cast< uint8_t >( 1 + record.best()->cost() );
            as< Operation >( data + Ip6Addr::size() + 2 ) = p.second.first == Route::ADD
                                                          ? Operation::Add
                                                          : Operation::Remove;
            data += EntrySize;
        }

        for ( const auto& p : _toSendFrom[ interfaceName ] ) {
            const RoutingTable::Record& record = p.second;
            assert( record.best().has_value() && "Record without a gateway should NOT be propagated" );
            as< Ip6Addr >( data ) = record.ip();
            as< uint8_t >( data + Ip6Addr::size() ) = record.mask();
            as< uint8_t >( data + Ip6Addr::size() + 1 ) = static_cast< uint8_t >( 1 + record.best()->cost() );
            as< Operation >( data + Ip6Addr::size() + 2 ) = p.first == Route::ADD
                                                          ? Operation::Add
                                                          : Operation::Remove;
            data += EntrySize;
        }

        return packet;
    }

public:
    using Cost = uint8_t;

    virtual ~RRP() = default;

    virtual bool afterMessage( const Interface& i, std::function< void ( PBuf&& ) > f, void* /* args */ ) override {
        bool res = false;
        if ( !_stub && amIStubby() ) {
            auto records = routingTableCB();
            auto defaultGW = defaultGatewayVia( *getPhysicalGw( records ) );
            _stub = true;
            _interfacesCommands[ i.name() ] = Command::Sync;
            // add the default gateway, nothing happens if it is present already
            _updates.push_back( { Route::ADD, defaultGW } );
            // remove all records for the now default gw interface
            for ( const auto& rec : records ) {
                for ( const auto& g : rec.gateways() ) {
                    if ( g.name() == defaultGW.best()->name() ) {
                        _updates.push_back( { Route::RM, { rec.ip(), rec.mask(), g.name(), g.cost(), this } } );
                    }
                }
            }
            res = true;
        }

        if ( _interfacesCommands.contains( i.name() ) && !_stubInterfaces.contains( i.name() ) ) {
            f( std::move( createMsg( i.name(), _interfacesCommands[ i.name() ] ) ) );
            if ( _interfacesCommands[ i.name() ] != Command::Hello ) {
                _toSendFrom.erase( i.name() );
            }
        }

        _interfacesCommands.erase( i.name() );
        return res;
    }

    virtual Result onMessage( const std::string& interfaceName, rofi::hal::PBuf packet ) override {
        auto packetWithoutHeader = pbuf_free_header( packet.release(), IP6_HLEN );
        return update( std::move( rofi::hal::PBuf::own( packetWithoutHeader ) ), interfaceName );
    }

    virtual bool onInterfaceEvent( const Interface& interface, bool connected ) override {
        assert( manages( interface ) && "onInterfaceEvent within protocol got unmanaged interface" );

        bool res = false;
        if ( connected ) {
            res = addInterface( interface );
        } else {
            res = removeInterface( interface );
            // removeInterface removes the interface from the managed ones, but we do not want this
            _managedInterfaces.push_back( interface );
        }

        return res;
    }

    virtual std::vector< std::pair< Route, RoutingTable::Record > > getRTEUpdates() const override {
        return _updates;
    }

    virtual void clearUpdates() override {
        _updates.clear();
        if ( _toSendFrom.empty() ) {
            _toSendExcept.clear();
        }
    }

    virtual bool addAddressOn( const Interface& interface, const Ip6Addr& ip, uint8_t mask ) override {
        if ( !ip.linkLocal() ) {
            Update rec{ Route::ADD, { ip, mask, interface.name(), 0 } };
            _updates.push_back( rec );
            auto it = std::ranges::find_if( _toSendExcept, [ &rec ]( const auto& r ) {
                // TODO: Do we want to compare records or their networks?
                return r.second.second.compareNetworks( rec.second );
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
                // TODO: Do we want to compare records or their networks?
                return r.second.second.compareNetworks( rec.second );
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
                // TODO: Do we want to compare records or their networks?
                return r.second.compareNetworks( l );
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

        // remove all routes that has this interface as their gateway
        auto records = routingTableCB();
        for ( const auto& rec : records ) {
            for ( const auto& g : rec.gateways() ) {
                if ( g.name() != interface.name() )
                    continue;
                
                _updates.push_back( { Route::RM, { rec.ip(), rec.mask(), interface.name(), 0 } } );
            }
        }

        return true;
    }

    virtual bool manages( const Interface& interface ) const override {
        return std::ranges::any_of( _managedInterfaces, [ &interface ]( const Interface& i ) {
            return interface == i;
        } );
    }

    virtual rofi::hal::Ip6Addr address() const override { return Ip6Addr( "ff02::1f" ); }
    virtual std::string name() const override { return "rrp"; }
    virtual std::string info() const override {
        std::string str = Protocol::info();
        str = str + "; stub: " + ( amIStubby() ? "yes" : "no" );

        if ( !_managedInterfaces.empty() ) {
            str += "\n    manages:";
            std::string pref = " ";
            for ( const auto& i : _managedInterfaces ) {
                std::string ifName = i.get().name();
                str += pref + ifName + ( _stubInterfaces.contains( ifName ) ? " (stub)" : "" ) + "\n";
                pref = "             ";
            }
        }
        return str;
    }
};

} // namespace rofi::net
