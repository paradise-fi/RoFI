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
                default:
                    ROFI_UNREACHABLE( "Route was modified" );
                    break;
            }
        }

        return res;
    }

    void propagateRouteUpdates( Protocol* proto ) {
        if ( !proto )
            return;

        for ( auto& interface : _interfaces ) {
            if ( interface.isVirtual() )
                continue;

            if ( !proto->manages( interface ) )
                continue;

            bool changed = proto->afterMessage( interface, [ this, &interface, proto ]( PBuf&& msg ) {
                                    interface.sendProtocol( proto->address(), std::move( msg ) );
                        }, nullptr );
            if ( changed ) {
                processConfigUpdates( proto->getConfigUpdates() );
                processRouteUpdates( *proto, proto->getRouteUpdates() );
            }
        }

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

    void processPossibleChanges( Protocol* protocol ) {
        if ( !protocol )
            return;

        bool b = false;
        if ( protocol->hasConfigUpdates() ) {
            processConfigUpdates( protocol->getConfigUpdates() );
            b = true;
        }
        if ( protocol->hasRouteUpdates() ) {
            processRouteUpdates( *protocol, protocol->getRouteUpdates() );
            b = true;
        }
        if ( b )
            propagateRouteUpdates( protocol );
    }

    void onInterfaceChange( const Interface* i, hal::ConnectorEvent e ) {
        if ( e != hal::ConnectorEvent::Connected && e != hal::ConnectorEvent::Disconnected )
            assert( false && "onInterfaceChange got a different event than (dis)connect" );

        for ( auto& proto : _protocols ) {
            if ( !proto->manages( *i ) )
                continue;

            bool changed = proto->onInterfaceEvent( *i, e == hal::ConnectorEvent::Connected );
            if ( changed ) {
                processPossibleChanges( proto.get() );
            }
        }
    }

    PhysAddr createMacFromId( int id ) {
        auto m = static_cast< uint8_t >( id );
        return PhysAddr( m, m, m, m, m, m );
    }

public:
    NetworkManager() = delete;

    /**
     * @brief Create a Network Manager instance for module corresponding to given RoFI proxy.
     *
     * This constructor creates MAC adress from the proxy's id.
     */
    explicit NetworkManager( hal::RoFI rofi )
    : NetworkManager( rofi, createMacFromId( rofi.getId() ) ) {}

    /**
     * @brief Create a Network Manager instance for module corresponding to given RoFI proxy.
     *
     * This constructor uses @p p as the MAC address.
     */
    NetworkManager( hal::RoFI rofi, PhysAddr p ) {
        int connectors = rofi.getDescriptor().connectorCount;

        auto logFun = [ &logger = _logger ]( Logger::Level l, const std::string& where, const std::string& msg ) {
                    logger.log( l, where, msg );
        };

        // virtual interface
        _interfaces.emplace_back( p, logFun, std::nullopt
                    , []( const Interface*, hal::ConnectorEvent ) { ROFI_UNREACHABLE( "interface dummy cb" ); } );

        // physical interfaces (those with connectors)
        for ( int i = 0; i < connectors; i++ ) {
            _interfaces.emplace_back( p, logFun, rofi.getConnector( i ), [ this ]( auto i, auto e ) {
                                        return onInterfaceChange( i, e );
                                    }
                                );
        }
    };

    /**
     * @brief Finds an interface of given name among Network Manager's interfaces.
     */
    const Interface* findInterface( const Interface::Name& name ) const {
        for ( auto& i : _interfaces ) {
            if ( i.name() == name )
                return &i;
        }
        
        return nullptr;
    }

    /**
     * @brief Returns an interface of given name. The interface must exist within Network Manager.
     */
    const Interface& interface( const Interface::Name& name ) const {
        auto res = findInterface( name );
        if ( !res )
            throw std::runtime_error( "interface of name " + name + " does not exits" );
        return *res;
    }

    /**
     * @brief Get iterable range of interfaces within the Network Manager.
     */
    auto interfaces() const {
        return std::views::all( _interfaces );
    }

    /**
     * @brief Add given protocol into the Network Manager.
     *
     * There must NOT be a protocol with same name and/or listenner address present in the Network Manager.
     *
     * @param p protocol to add
     * @return Pointer to the added instance under control of the Network Manager.
     */
    template< std::derived_from< Protocol > Proto >
    Protocol* addProtocol( const Proto& p ) {
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

        return _protocols.back().get();
    }

    /**
     * @brief Get iterable range of protocols within the Network Manager.
     */
    auto protocols() const {
        return std::views::all( _protocols );
    }

    /**
     * @brief Add given address and mask onto a given interface.
     *
     * @return true if the address was added succesfully
     * @return false otherwise
     */
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
    /**
     * @brief Remove given address and mask from the given interface.
     *
     * @return true if the address was removed succesfully
     * @return false otherwise
     */
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

    /**
     * @brief Get protocol of a given name.
     *
     * Names are unique within the protocol.
     *
     * @return a pointer to a protocol of a given name
     */
    Protocol* getProtocol( const std::string& name ) {
        for ( auto& proto : _protocols ) {
            if ( proto->name() == name )
                return proto.get();
        }

        return nullptr;
    }

    /**
     * @brief Set protocol to run on all interfaces.
     */
    void setProtocol( Protocol& proto ) {
        for ( auto& i : _interfaces ) {
            setProtocol( proto, i );
        }
    }

    /**
     * @brief Set protocol to run on a given interface.
     *
     * This sets up all necessary listeners, starts processing protocol messages, etc.
     *
     */
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
                        bool res = proto.onMessage( interface.name(), PBuf::own( p ) );
                        if ( res ) {
                            if ( proto.hasConfigUpdates() )
                                changed = processConfigUpdates( proto.getConfigUpdates() );
                            if ( proto.hasRouteUpdates() )
                                changed = processRouteUpdates( proto, proto.getRouteUpdates() ) || changed;
                        }

                        if ( changed ) {
                            propagateRouteUpdates( &proto );
                            for ( auto& protocol : _protocols ) {
                                if ( !protocol || !protocol->manages( interface ) || protocol->name() == proto.name() )
                                    continue;
                                processPossibleChanges( protocol.get() );
                            }
                        }
                        return 1; // pbuf was eaten
                    } );

                // proto addInterface adds updates into RTEUpdates
                rteChanged = proto.addInterface( interface ) || rteChanged;
                rteChanged = processConfigUpdates( proto.getConfigUpdates() ) || rteChanged;
                // we pick those updates and add new records / remove old ones in the Routing Table
                rteChanged = processRouteUpdates( proto, proto.getRouteUpdates() ) || rteChanged;
                // and clear updates which we extracted in the previous step
                proto.clearUpdates();
                break;
            }
        }

        propagateRouteUpdates( &proto );
    }

    /**
     * @brief Remove protocol from the given interface.
     *
     * This unsets all listeners, stops processing protocol's messages.
     *
     */
    void removeProtocol( Protocol& proto, const Interface& i ) {
        for ( auto& interface : _interfaces ) {
            if ( interface == i ) {
                interface.removeProtocol( proto.address() );
                proto.removeInterface( i );
            }
        }
    }

    // TODO: Make these state setting function more effective and remove code duplication
    /**
     * @brief Set the interface up. The interface starts processing trafic.
     */
    void setUp( const Interface& i ) {
        for ( auto& interface : _interfaces ) {
            if ( interface == i ) {
                // TODO: Add the address into the table!
                interface.setUp();
                break;
            }
        }
    }

    /**
     * @brief Set all interfaces up.
     *
     */
    void setUp() {
        for ( auto& interface : _interfaces ) {
            // TODO: Call the callback for connect?
            setUp( interface );
        }
    }

    /**
     * @brief Set the interface down. The interface stops processing trafic.
     */
    void setDown( const Interface& i ) {
        for ( auto& interface : _interfaces ) {
            if ( interface == i ) {
                // TODO: Remove the address from the table!
                // Call the callback for disconnect?
                setUp( interface );
                break;
            }
        }
    }

    /**
     * @brief Set all interfaces down.
     */
    void setDown() {
        for ( auto& interface : _interfaces ) {
            setDown( interface );
        }
    }

    /**
     * @brief Get network manager's routing table.
     */
    const RoutingTable& routingTable() const {
        return _routingTable;
    }

    /**
     * @brief Add a static route into the routing table.
     *
     * @return true if the route was succesfully added
     * @return false if the interface does not exist or the route was not added
     */
    bool addRoute( const Ip6Addr ip, uint8_t mask, const Interface::Name& ifname, RoutingTable::Cost cost ) {
        return findInterface( ifname ) && _routingTable.add( ip, mask, ifname, cost );
    }


    /**
     * @brief Remove a static route into the routing table.
     *
     * @return true if the route was succesfully removed
     * @return false if the interface does not exist or the route was not removed
     */
    bool rmRoute( const Ip6Addr ip, uint8_t mask, const Interface::Name& ifname ) {
        return findInterface( ifname ) && _routingTable.removeInterface( ip, mask, ifname );
    }

// TODO: esp32 lwip
#if 0
    /**
     * @brief Enables or disables (according to @param enable) stateless DHCP on a given interface
     */
    void dhcp( const Interface& interface, bool enable ) {
        for ( auto& i : _interfaces ) {
            if ( i.name() != interface.name() )
                continue;
            if ( enable )
                i.dhcpUp( Interface::DHCP::SLAAC );
            else
                i.dhcpDown();
            break;
        }
    }
#endif

    /**
     * @brief Logs a given message with corresponding severity level.
     */
    void log( Logger::Level l, const std::string& msg ) {
        _logger.log( l, msg );
    }

    /**
     * @brief Logs a given message with corresponding severity level specifying the component
     * where the event originated.
     */
    void log( Logger::Level l, const std::string& where, const std::string& msg ) {
        _logger.log( l, where, msg );
    }

    /**
     * @brief Returns an iterable range of logs.
     */
    auto logs() const {
        return _logger.logs();
    }

    /**
     * @brief Returns an iterable range of logs only from the specified component.
     */
    auto logsFrom( const Interface& i ) const {
        return _logger.logsFrom( i.name() );
    }

    /**
     * @brief Returns an iterable range of logs with given severity level.
     */
    auto logs( Logger::Level l ) const {
        return _logger.logs( l );
    }
};

} // namespace rofi::net
