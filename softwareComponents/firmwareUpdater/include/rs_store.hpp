#pragma once

#include "ordered_dict.hpp"
#include <chrono>
#include <limits>
#include <set>

using rofi::updater::messages::ChunkDescriptor;
using Clock = std::chrono::steady_clock;
using Sec = std::chrono::seconds;

template< typename Key, typename Range >
class RecentlySeenStore {
    static inline auto now() {
        return static_cast< Range >( duration_cast< Sec >( Clock::now().time_since_epoch() ).count() % std::numeric_limits< Range >::max() );
    }
public:
    explicit RecentlySeenStore( Range timeout, int maxCapacity = -1 ) : _timeout( timeout ), _maxCapacity( maxCapacity ) {}

    bool recentlySeen( const Key& dsc ) {
        bool seen = _map.contains( dsc );
        if ( seen ) {
            seen = _map.at( dsc ) >= now();
        }
        if ( seen ) {
            _map.moveToEnd( dsc );
            return true;
        }
        return false;
    }

    void markRecentlySeen( const Key& dsc ) {
        if ( _map.contains( dsc ) ) {
            _map.at( dsc ) = now() + _timeout;
            _map.moveToEnd( dsc );
            return;
        }

//        _removeObsolete();  // todo: needed?
        if ( _map.size() == _maxCapacity ) {
            _map.popItem();
        }

        _map.at( dsc ) = now() + _timeout;
    }

private:
    const Range _timeout;
    const int _maxCapacity;
    OrderedMap< Key, Range > _map;
};

template< typename Key, typename Range, typename Requester >
class RequestStore {
    static inline auto now() {
        return static_cast< Range >( duration_cast< Sec >( Clock::now().time_since_epoch() ).count() % std::numeric_limits< Range >::max() );
    }
public:
    explicit RequestStore( Range timeout, int maxCapacity = -1 ) : _timeout( timeout ), _maxCapacity( maxCapacity ) {}

    std::set< Requester > getRequesters( const Key& key ) {
        if ( ! _map.contains( key ) || _map.at( key ).first < now() ) {
            _cleanInFlightRequests( key );
            return {};
        }

        _map.moveToEnd( key );
        return _map.at( key ).second;
    }

    void markRequestInFlightFor( const Key& key, const Requester& requester, bool inFlight = true ) {
        if ( ! inFlight ) {
            if ( ! _map.contains( key ) ) return;
            auto& [ time, requesters ] = _map.at( key );

            if ( requesters.contains( requester ) ) {
                auto it = requesters.find( requester );
                requesters.erase( it );
            }
            _cleanInFlightRequests( key );
            return;
        }

        // if it is an expired record remove it to clear expired device entries
        _cleanInFlightRequests( key );
        bool contains = _map.contains( key );

        if ( _map.size() == _maxCapacity && ! contains ) {
            _map.popItem();
        }

        auto [ it, _ ] = _map.try_emplace( key, std::pair< Range, std::set< Requester > >( 0, {} ) );
        auto& [ k, pr ] = *it;
        auto& [ _ord, v ] = pr;
        auto& [ time, requesters ] = v;

        time = now() + _timeout;
        requesters.emplace( requester );

        _map.moveToEnd( key );
    }

    bool isRequestInFlightForAnybody( const Key& key ) {
        return _cleanInFlightRequests( key );
    }

private:
    const Range _timeout;
    const int _maxCapacity;
    OrderedMap< Key, std::pair< Range, std::set< Requester > > > _map;

    bool _cleanInFlightRequests( const Key& key ) {
        // Does the cleanup on the key and returns true if the record remains present thus is valid
        if ( ! _map.contains( key ) ) {
            return false;
        }
        auto& [ time, requesters ] = _map.at( key );
        if ( time < now() || requesters.empty() ) {
            _map.erase( key );
            return false;
        }

        _map.moveToEnd( key );  // todo: possibly remove statement
        return true;
    }
};
