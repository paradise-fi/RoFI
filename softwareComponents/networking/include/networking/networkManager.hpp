#pragma once

#include "rofi_hal.hpp"
#include "lwip++.hpp"

#include <networking/interface.hpp>
#include <networking/protocol.hpp>
#include <networking/logger.hpp>

#include <atoms/unreachable.hpp>

#include <vector>
#include <deque>
#include <map>
#include <utility>

namespace rofi::net {

class NetworkManager {
    Logger _logger;
    std::deque< Interface > _interfaces;
    // running protocols: < < name of interface, name of protocol >, handle >
    // TODO: Make this garbage readable
    RoutingTable _routingTable;
    std::vector< std::unique_ptr< Protocol > > _protocols;

    bool processRouteUpdates( const Protocol& proto, const std::vector< std::pair< Protocol::Route, RoutingTable::Record > >& updates ) {
        bool res = false;
        for ( auto [ up, rec ] : updates ) {
            switch ( up ) {
                case Protocol::Route::ADD:
                    res = _routingTable.add( rec, &proto ) || res; // Pay attention to short-circuit evaluation
                    break;
                case Protocol::Route::RM:
                    res = _routingTable.remove( rec ) || res;
                    break;
                case Protocol::Route::CHANGE:
                    ROFI_UNREACHABLE( "Change is not yet implemented" );
                    // res = _routingTable.update( rec ) || res;
                    break;
                default:
                    ROFI_UNREACHABLE( "Route was modified" );
                    break;
            }
        }

        return res;
    }

    void propagateRouteUpdates( Protocol* proto ) {
        for ( auto& interface : _interfaces ) {
            if ( interface.isVirtual() )
                continue;

            if ( !proto || !proto->manages( interface ) )
                continue;

            bool changed = proto->afterMessage( interface, [ this, &interface, proto ]( PBuf&& msg ) {
                                    interface.sendProtocol( proto->address(), std::move( msg ) );
                        }, nullptr );
            if ( changed ) {
                processConfigUpdates( proto->getInterfaceUpdates() );
                processRouteUpdates( *proto, proto->getRTEUpdates() );
            }
        }

        if ( proto )
            proto->clearUpdates();
    }

    void handleIpUpdate( bool add, const Interface::Name& ifname, const Ip6Addr& ip, uint8_t mask ) {
        for ( auto& i : _interfaces ) {
            if ( i.name() != ifname )
                continue;

            if ( add ) {
                addAddress( ip, mask, i );
            } else {
                removeAddress( ip, mask, i );
            }
            break;
        }
    }

    void handleStateUpdate( bool shutDown, const Interface::Name& ifname ) {
        for ( auto& i : _interfaces ) {
            if ( i.name() != ifname )
                continue;
            
            if ( shutDown )
                setDown( i );
            else
                setUp( i );
            break;
        }
    }

    bool processConfigUpdates( const std::vector< std::pair< Protocol::ConfigAction, Protocol::ConfigChange > >& changes ) {
        bool changedAddress = false;
        for ( const auto& [ act, change ] : changes ) {
            switch ( act ) {
                case Protocol::ConfigAction::ADD_IP:
                case Protocol::ConfigAction::REMOVE_IP:
                    handleIpUpdate( act == Protocol::ConfigAction::ADD_IP
                                  , std::get< Interface::Name >( change )
                                  , std::get< Ip6Addr >( change )
                                  , std::get< uint8_t >( change ) );
                    changedAddress = true;
                    break;
                case Protocol::ConfigAction::SHUT_DOWN:
                    handleStateUpdate( true, std::get< Interface::Name >( change ) );
                    break;
                case Protocol::ConfigAction::SET_UP:
                    handleStateUpdate( false, std::get< Interface::Name >( change ) );
                    break;
                case Protocol::ConfigAction::RESPOND:
                    changedAddress = true;
                    break;
                default:
                    ROFI_UNREACHABLE( "Protocol::ConfigAction was modified" );
            }
        }

        return changedAddress;
    }

public:
    NetworkManager() = delete;

    explicit NetworkManager( hal::RoFI rofi ) {
        int connectors = rofi.getDescriptor().connectorCount;
        uint8_t moduleID = static_cast< uint8_t >( rofi.getId() );
        PhysAddr p( moduleID, moduleID, moduleID, moduleID, moduleID, moduleID );

        auto logFun = [ &logger = _logger ]( Logger::Level l, const std::string& where, const std::string& msg ) {
                    logger.log( l, where, msg );
        };

        _interfaces.emplace_back( p, logFun ); // virtual interface
        for ( int i = 0; i < connectors; i++ ) {
            _interfaces.emplace_back( p, logFun, rofi.getConnector( i ) );
        }
    };

    NetworkManager( hal::RoFI rofi, PhysAddr p ) {
        int connectors = rofi.getDescriptor().connectorCount;

        auto logFun = [ &logger = _logger ]( Logger::Level l, const std::string& where, const std::string& msg ) {
                    logger.log( l, where, msg );
        };

        _interfaces.emplace_back( p, logFun ); // virtual interface
        for ( int i = 0; i < connectors; i++ ) {
            _interfaces.emplace_back( p, logFun, rofi.getConnector( i ) );
        }
    };

    std::optional< std::reference_wrapper< const Interface > > findInterface( const Interface::Name& name ) const {
        for ( auto& i : _interfaces ) {
            if ( i.name() == name )
                return { i };
        }
        
        return std::nullopt;
    }

    std::reference_wrapper< const Interface > interface( const Interface::Name& name ) const {
        auto res = findInterface( name );
        assert( res.has_value() && "No interface of that name" );
        return res.value();
    }

    // TODO: Improve the return value type
    auto interfaces() const {
        return std::views::all( _interfaces );
    }

    template< std::derived_from< Protocol > Proto >
    void addProtocol( const Proto& p ) {
        auto it = std::ranges::find_if( _protocols, [ &p ]( auto& proto ) {
            return proto->name() == p.name() || proto->address() == p.address();
        } );

        if ( it == _protocols.end() ) {
            _protocols.push_back( std::make_unique< Proto >( p ) );
            Protocol* ptr = _protocols.back().get();
            ptr->setRoutingTableCB( [ ptr, this ]() { return _routingTable.recordsLearnedFrom( *ptr ); } );
        } else {
            throw std::runtime_error( "" );
        }
    }

    auto protocols() const {
        return std::views::all( _protocols );
    }

    bool addAddress( const Ip6Addr& ip, uint8_t mask, const Interface& i ) {
        for ( auto& interface : _interfaces ) {
            if ( interface == i ) {
                bool added = interface.addAddress( ip, mask );
                if ( added ) {
                    _routingTable.add( ip, mask, i.name(), 0 );
                    for ( auto& proto : _protocols ) {
                        if ( proto && proto->manages( i ) ) {
                            proto->addAddressOn( i, ip, mask );
                        }
                    }
                }
                return added;
            }
        }

        return false;
    }

    // TODO: Remove the code duplication!
    bool removeAddress( const Ip6Addr& ip, uint8_t mask, const Interface& i ) {
        for ( auto& interface : _interfaces ) {
            if ( interface == i ) {
                bool removed = interface.removeAddress( ip, mask );
                if ( removed ) {
                    //_routingTable.remove( ip, mask, RoutingTable::Gateway{ i.name(), 0 } );
                    _routingTable.removeInterface( ip, mask, i.name() );
                    for ( auto& proto : _protocols ) {
                        if ( proto && proto->manages( i ) ) {
                            proto->rmAddressOn( i, ip, mask );
                        }
                    }
                }
                return removed;
            }
        }

        return false;
    }

    Protocol* getProtocol( const std::string& name ) {
        for ( auto& proto : _protocols ) {
            if ( proto->name() == name )
                return proto.get();
        }

        return nullptr;
    }

    void setProtocol( Protocol& proto, const Interface& i ) {
        bool rteChanged = false;

        for ( auto& interface : _interfaces ) {
            if ( interface == i ) {
                interface.setProtocol( proto.address()
                  , [ this, &proto, &interface ]( void*, raw_pcb*, struct pbuf* p, const ip_addr_t* ) {
                        if ( !p ) {
                            return 0;
                        }

                        auto* ip6hdr = static_cast< ip6_hdr * >( p->payload );
                        Ip6Addr destination( "::" );
                        ip6_addr_copy_from_packed( destination, ip6hdr->dest );
                        if ( destination != proto.address() )
                            return 0;

                        bool changed = false;
                        auto res = proto.onMessage( interface.name(), PBuf::own( p ) );
                        if ( res == Protocol::Result::INTERFACE_UPDATE || res == Protocol::Result::ALL_UPDATE ) {
                            changed = processConfigUpdates( proto.getInterfaceUpdates() );
                        }
                        if ( res == Protocol::Result::ROUTE_UPDATE || res == Protocol::Result::ALL_UPDATE ) {
                            changed = processRouteUpdates( proto, proto.getRTEUpdates() ) || changed;
                        }

                        if ( changed ) {
                            propagateRouteUpdates( &proto );
                            for ( auto& protocol : _protocols ) {
                                if ( !protocol || !protocol->manages( interface ) || protocol->name() == proto.name() )
                                    continue;
                                if ( !protocol->getInterfaceUpdates().empty() || !protocol->getRTEUpdates().empty() ) {
                                    processConfigUpdates( protocol->getInterfaceUpdates() );
                                    processRouteUpdates( *protocol, protocol->getRTEUpdates() );
                                    propagateRouteUpdates( protocol.get() );
                                }
                            }
                        }
                        return 1; // pbuf was eaten
                    } );

                // proto addInterface adds updates into RTEUpdates
                rteChanged = proto.addInterface( interface ) || rteChanged;
                rteChanged = processConfigUpdates( proto.getInterfaceUpdates() ) || rteChanged;
                // we pick those updates and add new records / remove old ones in the Routing Table
                rteChanged = processRouteUpdates( proto, proto.getRTEUpdates() ) || rteChanged;
                // and clear updates which we extracted in the previous step
                proto.clearUpdates();
                break;
            }
        }

        propagateRouteUpdates( &proto );
    }

    void removeProtocol( Protocol& proto, const Interface& i ) {
        for ( auto& interface : _interfaces ) {
            if ( interface == i ) {
                interface.removeProtocol( proto.address() );
                proto.removeInterface( i );
            }
        }
    }

    // TODO: Make these state setting function more effective and remove code duplication
    void setUp( const Interface& i ) {
        for ( auto& interface : _interfaces ) {
            if ( interface == i ) {
                // TODO: Add the address into the table!
                interface.setUp();
                break;
            }
        }
    }

    void setUp() {
        for ( auto& interface : _interfaces ) {
            setUp( interface );
        }
    }

    void setDown( const Interface& i ) {
        for ( auto& interface : _interfaces ) {
            if ( interface == i ) {
                // TODO: Remove the address from the table!
                setUp( interface );
                break;
            }
        }
    }

    void setDown() {
        for ( auto& interface : _interfaces ) {
            setDown( interface );
        }
    }

    const RoutingTable& routingTable() const {
        return _routingTable;
    }

    bool addRoute( const Ip6Addr ip, uint8_t mask, const Interface::Name& ifname, RoutingTable::Cost cost ) {
        return findInterface( ifname ) && _routingTable.add( ip, mask, ifname, cost );
    }

    bool rmRoute( const Ip6Addr ip, uint8_t mask, const Interface::Name& ifname ) {
        return findInterface( ifname ) && _routingTable.removeInterface( ip, mask, ifname );
    }

    void log( Logger::Level l, const std::string& msg ) {
        _logger.log( l, msg );
    }

    void log( Logger::Level l, const std::string& where, const std::string& msg ) {
        _logger.log( l, where, msg );
    }

    auto logs() const {
        return _logger.logs();
    }

    auto logsFrom( const Interface& i ) const {
        return _logger.logsFrom( i.name() );
    }

    auto logs( Logger::Level l ) const {
        return _logger.logs( l );
    }

};

} // namespace rofi::net
