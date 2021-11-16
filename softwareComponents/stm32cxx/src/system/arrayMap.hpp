#pragma once

#include <vector>
#include <functional>
#include <algorithm>

template < typename Key, typename Val, typename Compare = std::less< Key >,
           typename Container = std::vector< std::pair< Key, Val > > >
class ArrayMap {
public:
    using key_type = Key;
    using mapped_type = Val;
    using value_type = std::pair< Key, Val >;
    using size_type = typename Container::size_type;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using reference = value_type&;
    using const_reference = const value_type&;

    ArrayMap() = default;
    ArrayMap( std::initializer_list< value_type > list ) {
        for ( auto& x : list )
            emplace( std::move( x ) );
    }

    auto begin() { return _container.begin(); }
    auto begin() const { return _container.cbegin(); }
    auto cbegin() const { return _container.cbegin(); }
    auto end() { return _container.end(); }
    auto end() const { return _container.cend(); }
    auto cend() const { return _container.cend(); }

    auto empty() const { return _container.empty(); }
    auto size() const { return _container.size(); }

    void clear() noexcept { _container.clear(); }

    std::pair< iterator, bool > insert( const_reference value ) {
        auto it = find( value.first );
        if ( it != cend() )
            return { it, false };
        _container.push_back( value );
        std::sort( begin(), end(), comparator() );
        return { find( value.first ), true };
    }

    template< class... Args >
    std::pair< iterator, bool > emplace( Args&&... args ) {
        _container.emplace_back( std::forward< Args >( args )... );
        auto it = std::lower_bound( begin(), end() - 1, _container.back(), comparator() );
        if ( it == end() - 1 || it->first != _container.back().first ) {
            auto key = _container.back().first;
            std::sort( begin(), end(), comparator() );
            return { find( key ), true };
        }
        auto key = it->first;
        _container.pop_back();
        return { find( key ), false };
    }

    void erase( iterator pos ) {
        using std::swap;
        swap( *pos, _container.back() );
        _container.pop_back();
        std::sort( begin(), end(), comparator() );
    }

    size_type erase( const key_type& key ) {
        auto it = find( key );
        if ( it == end() )
            return 0;
        erase( it );
        return 1;
    }

    void swap( ArrayMap& other ) {
        using std::swap;
        swap( _container, other._container );
    }

    size_type count( const Key& key ) const {
        return find( key ) == cend() ? 0 : 1;
    }

    iterator find( const Key& key ) {
        auto elem = std::lower_bound( begin(), end(), key,
            keyComparator() );
        return elem != end() && elem->first == key ? elem : end();
    }

    const_iterator find( const Key& key ) const {
        auto elem = std::lower_bound( cbegin(), cend(), key,
            keyComparator() );
        return elem != cend() && elem->first == key ? elem : cend();
    }

    Val& operator[]( const Key& key ) {
        auto it = find( key );
        if ( it != end() )
            return it->second;
        return emplace( key, Val{} ).first->second;
    }

    bool operator<( const ArrayMap &o ) const {
        return _container < o._container;
    }

    bool operator==( const ArrayMap &o ) const {
        return _container == o._container;
    }

private:
    auto comparator() const {
        return []( const_reference a, const_reference b ) {
            return Compare()( a.first, b.first );
        };
    }

    auto keyComparator() const {
        return []( const_reference a, const Key& key ) {
            return Compare()( a.first, key );
        };
    }

    Container _container;
};
