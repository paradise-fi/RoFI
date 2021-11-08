#pragma once

#include <vector>
#include <functional>

template < typename Key, typename Val, typename Compare = std::less< Key >,
           typename Container = std::vector< std::pair< Key, Val > > >
class LinearMap {
public:
    using key_type = Key;
    using mapped_type = Val;
    using value_type = std::pair< Key, Val >;
    using size_type = typename Container::size_type;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using reference = value_type&;
    using const_reference = const value_type&;

    LinearMap() = default;
    LinearMap( std::initializer_list< value_type > list ) {
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
        return { find( value.first ), true };
    }

    template< class... Args >
    std::pair< iterator, bool > emplace( Args&&... args ) {
        _container.emplace_back( std::forward< Args >( args )... );
        auto it = find( _container.back().first );
        if ( it == end() - 1 )
            return { it, true };
        _container.pop_back();
        return { it, false };
    }

    void erase( iterator pos ) {
        using std::swap;
        swap( *pos, _container.back() );
        _container.pop_back();
    }

    size_type erase( const key_type& key ) {
        auto it = find( key );
        if ( it == end() )
            return 0;
        erase( it );
        return 1;
    }

    void swap( LinearMap& other ) {
        using std::swap;
        swap( _container, other._container );
    }

    size_type count( const Key& key ) const {
        return find( key ) == cend() ? 0 : 1;
    }

    iterator find( const Key& key ) {
        auto e = end();
        for ( iterator i = begin(); i != e; i++ )
            if ( i->first == key )
                return i;
        return e;
    }

    const_iterator find( const Key& key ) const {
        auto e = cend();
        for ( const_iterator i = cbegin(); i != e; i++ )
            if ( i->first == key )
                return i;
        return e;
    }

    Val& operator[]( const Key& key ) {
        auto it = find( key );
        if ( it != end() )
            return it->second;
        return emplace( key, Val{} ).first->second;
    }

    bool operator==( const LinearMap& other ) {
        if ( size() != other.size() )
            return false;
        for ( const auto& x : _container ) {
            auto it = other.find( x.first );
            if ( it == other.end() || it->second == x.second )
                return false;
        }
        return true;
    }

private:
    Container _container;
};
