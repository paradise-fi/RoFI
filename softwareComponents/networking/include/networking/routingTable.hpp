#pragma once

#include "lwip++.hpp"
#include <networking/interface.hpp>

#include <vector>
#include <list>
#include <algorithm>
#include <optional>
#include <ranges>
#include <functional>

namespace rofi::net {
    class Protocol;

/**
 * Class representing routing table of the platform. This class also manages
 * the underlying lwip library, therefore users should not interfere and
 * manage the forwarding table in lwip themselves.
*/
class RoutingTable {
public:
    using Cost = int;

    /**
     * \brief Class representing single gateway with its cost, output interface,
     * and protocol via witch the gateway was discovered.
    */
    class Gateway {
        // TODO: Check the name (length and format; maybe using format?!)
        Interface::Name _ifname;
        Cost _cost;
        const Protocol* _learnedFrom;

        public:
            Gateway( const Interface::Name& i, Cost c )
            : Gateway( i, c, nullptr ) {}

            Gateway( const Interface::Name& i, Cost c, const Protocol* learnedFrom )
            : _ifname( i ), _cost( c ), _learnedFrom( learnedFrom ) {}

            Cost cost() const { return _cost; }
            Interface::Name name() const { return _ifname; }
            const Protocol* learnedFrom() const { return _learnedFrom; }

            /**
             * @brief Update the cost with a new value, if the new value is lower.
             *
             * \return true if the cost changed, false otherwise
             */
            bool improveCost( Cost c ) {
                if ( _cost > c ) {
                    _cost = c;
                    return true;
                }
                return false;
            }

            auto operator<=>( const Gateway& ) const = default;
    };

    /**
     * \brief Class representing a record of the routing table. Each network (i.e., ip + mask)
     * has its own record.
    */
    class Record {
        Ip6Addr _ip;
        uint8_t _mask;
        std::list< Gateway > _gws;
    public:

        Record( const Ip6Addr& ip, uint8_t mask )
        : _ip( ip ), _mask( mask ) {}

        Record( const Ip6Addr& ip, uint8_t mask, const Interface::Name& n, Cost c, const Protocol* from = nullptr )
        : _ip( ip ), _mask( mask ) { _gws.push_front( { n, c, from } ); }

        /**
         * \brief Add a new gateway for the given record.
         * 
         * \return true if the gateway was added.
        */
        bool addGateway( const Interface::Name& n, Cost c ) {
            return addGateway( n, c, nullptr );
        }

        /**
         * \brief Overloaded version of the previous function.
        */
        bool addGateway( Interface::Name n, Cost c, const Protocol* learnedFrom ) {
            Gateway newGw( n, c, learnedFrom );
            auto it = std::find_if( _gws.begin(), _gws.end(), [ &newGw ]( auto& g ) {
                return g.name() == newGw.name() && g.learnedFrom() == newGw.learnedFrom();
            } );

            if ( it != _gws.end() ) {
                if ( it->cost() == c )
                    return false; // the same record already exists
                if ( it->cost() == 0 )
                    return false; // do not add records to local interfaces/addresses

                // the record exist but has a different cost -- use the better one
                return it->improveCost( c );
            }

            // if the last has better cost than the new one, add it right at the end
            if ( !_gws.empty() && _gws.back().cost() <= c ) {
                _gws.push_back( newGw );
                return false;
            }

            auto best = _gws.front();
            it = _gws.begin();
            while ( it != _gws.end() ) {
                if ( *it == newGw ) // the record already exists
                    return false;

                if ( it->cost() > c ) {
                    _gws.insert( it, newGw );
                    break;
                }

                it++;
            }
            
            return best != _gws.front(); // return true if the best has changed
        }

        /**
         * \brief Remove given gateway.
         * 
         * \return true if the gateway was succesfully removed.
        */
        bool removeGateway( const Gateway& gw ) {
            return removeGateway( gw.name(), gw.cost(), gw.learnedFrom() );
        }

        /**
         * \brief Overloaded version of removeGateway.
        */
       // TODO: Consider removing this overload.
        bool removeGateway( const Interface::Name& n, Cost c, const Protocol* learnedFrom = nullptr ) {
            if ( _gws.empty() )
                return false;

            auto best = _gws.front();
            Gateway newGw( n, c, learnedFrom );
            _gws.remove_if( [ &newGw ]( auto& gw ) { return gw == newGw; } );
            return _gws.empty() || best != _gws.front();
        }

        /**
         * \brief Remove all gateways from the record with given interface name.
        */
        bool removeGateway( const Interface::Name& n ) {
            if ( _gws.empty() )
                return false;

            auto best = _gws.front();
            _gws.remove_if( [ &n ]( auto& gw ) { return gw.name() == n; } );
            return _gws.empty() || best != _gws.front();
        }

        /**
         * \brief Get Record's IP.
        */
        Ip6Addr ip() const { return _ip; }
        /**
         * \brief Get Record's mask.
        */
        uint8_t mask() const { return _mask; }

        /**
         * \brief Get number of gateways..
        */
        std::size_t size() const {
            return _gws.size();
        }

        /**
         * Get the best (with the lowest cost) gateway for given network if it exists.
        */
        std::optional< Gateway > best() const {
            return _gws.empty() ? std::nullopt : std::optional( _gws.front() );
        }

        /**
         * \brief Get iterable range of Record's gateways.
        */
        auto gateways() const {
            return std::views::all( _gws );
        }

        bool compareNetworks( const Record& r ) const {
            return compareNetworks( r.ip(), r.mask() );
        }

        bool compareNetworks( const Ip6Addr& ip, uint8_t mask ) const {
            return _ip == ip && _mask == mask;
        }

        auto operator<=>( const Record& ) const = default;
    };

    /**
     * \brief Get the count of records (number of known networks).
    */
    std::size_t size() const { return _records.size(); }

    /**
     * \brief Check if the table contains any record.
     * 
     * \return true if there are no records.
    */
    bool empty() const { return _records.empty(); }

    /**
     * \brief Find a record for given network.
    */
    Record* find( const Ip6Addr& ip, uint8_t mask ) {
        auto it = _findNetworkRecord( ip, mask );
        if ( it != _records.end() ) {
            return &( *it );
        }

        return nullptr;
    }

    /**
     * \brief Add a new network to the table. If the record exists, it merges gateways of both records.
     * 
     * \return true if a record was added or (in case it exists) merging added a new gateway.
    */
    bool add( const Record& r, const Protocol* learnedFrom = nullptr ) {
        bool res = false;
        for ( auto g : r.gateways() ) {
            res = add( r.ip(), r.mask(), g.name(), g.cost(), learnedFrom ) || res;
        }

        return res;
    }

    /**
     * \brief Overloaded version of `add`.
    */
    // TODO: Do we want an overloaded version?
    bool add( const Ip6Addr& ip, uint8_t mask, const Interface::Name& n, Cost c, const Protocol* learned = nullptr ) {
        auto rec = find( ip, mask );
        if ( rec ) {
            if ( rec->best() && rec->best()->cost() == 0 && c != 0 ) // do not add external routes to local interfaces
                return false;

            auto size = rec->size();
            if ( rec->addGateway( n, c, learned ) ) // returns true if the best changed
                ip_update_route( &ip, mask, n.c_str() );
            return size != rec->size(); // true if the gw was added
        }

        _records.push_back( { ip, mask, n, c, learned } );
        ip_add_route( &ip, mask, n.c_str() ); // new record is added -- add it to lwip too
        return true;
    }

    // TODO: Reduce remove methods and separate them based on the functionality.

    /**
     * \brief Remove given record.
     * 
     * \return true if record was removed.
    */
    bool remove( const Record& r ) {
        bool res = false;
        for ( auto g : r.gateways() ) {
            res = remove( r.ip(), r.mask(), g ) || res;
        }

        return res;
    }

    /**
     * \brief Remove all gateways from the given network record with given Gateways.
    */
    bool remove( const Ip6Addr& ip, uint8_t mask, const Gateway& gw ) {
        auto it = _findNetworkRecord( ip, mask );
        if ( it != _records.end() ) {
            auto size = it->size();
            it->removeGateway( gw );
            if ( !it->best() ) { // do not leave records without gateways
                _records.erase( it );
                ip_rm_route( &ip, mask );
                return true;
            }
            ip_update_route( &ip, mask, it->best()->name().c_str() );
            return size != it->size();
        }

        return false;
    }

    /**
     * \brief Removes record for given network with all of its gateways.
    */
    bool removeNetwork( const Ip6Addr& ip, uint8_t mask ) {
        auto it = _findNetworkRecord( ip, mask );
        if ( it != _records.end() ) {
            _records.erase( it );
            ip_rm_route( &ip, mask );
            return true;
        }

        return false;
    }

    /**
     * \brief Remove all gateways from the given network record with given output interface.
    */
    // TODO: Remove duplication from remove and clean the interface a bit!
    bool removeInterface( const Ip6Addr& ip, uint8_t mask, const Interface::Name& gwn ) {
        auto it = _findNetworkRecord( ip, mask );
        if ( it != _records.end() ) {
            auto size = it->size();
            it->removeGateway( gwn );
            if ( !it->best() ) { // do not leave records without gateways
                _records.erase( it );
                ip_rm_route( &ip, mask );
                return true;
            }
            ip_update_route( &ip, mask, it->best()->name().c_str() );
            return size != it->size();
        }

        return false;
    }

    /**
     * Remove all gateways using given interface as the output. Removes records that
     * would end up empty (i.e., without any gateway).
    */
    void purge( const Interface::Name& n ) {
        auto res = _records | std::views::filter( [ &n ]( Record& r ) {
                                        r.removeGateway( n );
                                        return r.best().has_value();
                            } );
        _records = std::vector< Record >( res.begin(), res.end() );
        ip_rm_route_if( n.c_str() );
    }

    using Records = std::vector< Record >;

    /**
     * \brief Function for getting all records learned through given protocol.
    */
    Records recordsLearnedFrom( const Protocol& proto ) const {
        std::function< bool( const Gateway& ) > gwWithProto = [ ptr = &proto ]( const Gateway& gw ) {
            return gw.learnedFrom() == ptr;
        };

        std::function< bool( const Record& ) > recWithProto = [ gwWithProto ]( const Record& r ) {
            return std::ranges::any_of( r.gateways(), gwWithProto );
        };

        std::function< Record( Record ) > recDropNonProto = [ gwWithProto ]( Record r ) {
                std::vector< Gateway > gws( r.gateways().begin(), r.gateways().end() );
                for ( auto& gw : gws ) {
                    if ( !gwWithProto( gw ) )
                        r.removeGateway( gw );
                }
                return r;
        };

        Records res;
        for ( const auto& r : _records ) {
            if ( recWithProto( r ) )
                res.push_back( recDropNonProto( r ) );
        }

        return res;
    }

    friend std::ostream& operator<<( std::ostream&, const RoutingTable& );

private:
    Records _records;

    Records::iterator _findNetworkRecord( const Ip6Addr& ip, uint8_t mask ) {
        auto it = std::find_if( _records.begin(), _records.end(), [ &ip, mask ]( const Record& r ) -> bool {
            return r.compareNetworks( ip, mask );
        } );

        return it;
    }

};

} // namespace rofi::net
