#pragma once

#include <forward_list>
#include <unordered_map>

template< typename K, typename V >
class OrderedMap {
    using Map = typename std::unordered_map< K, std::pair< unsigned int, V > >;

public:
    using iterator = typename Map::iterator;
    using const_iterator = typename Map::const_iterator;
    using size_type = typename Map::size_type;

    OrderedMap() = default;

    bool contains( const K& key ) {
        return _map.contains( key );
    }

    auto size() const {
        return _map.size();
    }

    void moveToEnd( const K& key ) {
        _map.at( key ).first = _latestHit++;
    }

    V& at( const K& key ) {
        return _map.at( key ).second;
    }

    size_type erase( const K& key ) {
        return _map.erase( key );
    }

    void popItem() {
        auto it = std::min_element(
                _map.begin(), _map.end(),
                []( const auto& a, const auto& b ) { return a.second.first < b.second.first; }
        );
        _map.erase( it );
    }

    template< class... Args >
    std::pair< iterator, bool > try_emplace( const K& k, Args&&... args ) {
        return _map.try_emplace( k, _latestHit++, std::forward< Args >(args)... );
    }

    template< class... Args >
    std::pair< iterator, bool > try_emplace( K&& k, Args&&... args ) {
        return _map.try_emplace( std::forward< K >( k ), _latestHit++, std::forward< Args >(args)... );
    }

    template< class... Args >
    iterator try_emplace( const_iterator hint, const K& k, Args&&... args ) {
        return _map.try_emplace( hint, k, _latestHit++, std::forward< Args >( args )... );
    }

    template< class... Args >
    iterator try_emplace( const_iterator hint, K&& k, Args&&... args ) {
        return _map.try_emplace( hint, std::forward< K >( k ), _latestHit++, std::forward< Args >( args )... );
    }

private:
    Map _map;
    unsigned int _latestHit = 0;
};
