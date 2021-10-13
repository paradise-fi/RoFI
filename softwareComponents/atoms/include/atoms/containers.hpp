#pragma once

#include <vector>
#include <optional>
#include <cassert>

namespace atoms {

/**
 * \brief std::vector-based set representation providing reusable integer ids
 * for the inserted elements.
 *
 * You are supposed to insert elements via HandleSet::insert(), which returns unique
 * element ID. Then you can acces this element using HandleSet::operator[]() or
 * erase it using HandleSet::erase(). The insertion is performed in amortized
 * constant time, deletion and element access is performed in constant time.
 *
 * Otherwise this container behaves the same as containers from the standard
 * library.
 */
template < typename T >
class HandleSet {
    using Container = std::vector< std::optional< T > >;

    template < bool IsConst >
    class Iterator {
        using UnderlayingIterator = typename std::conditional_t< IsConst,
            typename Container::const_iterator, typename Container::iterator >;
        using HandleSetPtr = typename std::conditional_t< IsConst,
            const HandleSet*, HandleSet* >;
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = typename UnderlayingIterator::difference_type;
        using reference = typename std::conditional_t< IsConst, T const &, T & >;
        using pointer = typename std::conditional_t< IsConst, T const *, T * >;

        Iterator(): _set( nullptr ) {}
        Iterator( HandleSetPtr set, UnderlayingIterator it ):
            _set( set ), _it( it )
        {
            afterIncrement();
        }

        Iterator( const Iterator& ) = default;
        Iterator& operator=( const Iterator& ) = default;

        Iterator& operator++() {
            ++_it;
            afterIncrement();
            return *this;
        }

        Iterator operator++( int ) {
            auto copy = *this;
            ++( *this );
            return copy;
        }

        Iterator& operator--() {
            --_it;
            afterDecrement();
            return *this;
        }

        Iterator operator--( int ) {
            auto copy = *this;
            --( *this );
            return copy;
        }

        template < bool _IsConst = IsConst >
        std::enable_if_t< _IsConst, reference > operator*() const {
            assert( _it->has_value() && "No value held" );
            return _it->value();
        }

        template < bool _IsConst = IsConst >
        std::enable_if_t< !_IsConst, reference > operator*() const {
            assert( _it->has_value() && "No value held" );
            return _it->value();
        }

         template < bool _IsConst = IsConst >
        std::enable_if_t< _IsConst, pointer > operator->() const {
            assert( _it->has_value() && "No value held" );
            return &_it->value();
        }

        template < bool _IsConst = IsConst >
        std::enable_if_t< !_IsConst, pointer > operator->() const {
            assert( _it->has_value() && "No value held" );
            return &_it->value();
        }

        bool operator==( const Iterator& other ) {
            return _it == other._it;
        }

        bool operator!=( const Iterator& other ) {
            return _it != other._it;
        }
    private:
        void afterIncrement() {
            if ( _set->_elems.empty() )
                return;
            while ( _it != _set->_elems.end() && !_it->has_value() )
                _it++;
        }

        void afterDecrement() {
            if ( _set->_elems.empty() )
                return;
            while( _it != _set->_elems.begin() && !_it->has_value() )
                _it--;
        }

        HandleSetPtr _set;
        UnderlayingIterator _it;
    };
public:
    using value_type = T;
    using size_type = typename Container::size_type;
    using id_type = size_type;
    using difference_type = typename Container::difference_type;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = Iterator< false >;
    using const_iterator = Iterator< true >;

    void swap( HandleSet& other ) {
        using std::swap;
        swap( _elems, other._elems );
        swap( _freeIdxs, other._freeIdxs );
    }

    size_type size() const {
        return _elems.size() - _freeIdxs.size();
    }

    void shrink_to_fit() {
        _elems.shrink_to_fit();
        _freeIdxs.shrink_to_fit();
    }

    /**
     * \brief Insert new element and get its ID
     */
    id_type insert( const T& value ) {
        if ( _freeIdxs.empty() ) {
            _elems.push_back( value );
            return _elems.size() - 1;
        }
        size_type idx = _freeIdxs.back();
        _freeIdxs.pop_back();
        _elems[ idx ] = value;
        return idx;
    }

    /**
     * \brief Insert new element and get its ID
     */
    template< typename TT = T>
    auto insert( T&& value )
        -> std::enable_if_t<
            std::is_move_constructible_v< TT > && std::is_move_assignable_v< TT >,
            id_type >
    {
        if ( _freeIdxs.empty() ) {
            _elems.push_back( std::move( value ) );
            return _elems.size() - 1;
        }
        size_type idx = _freeIdxs.back();
        _freeIdxs.pop_back();
        _elems[ idx ] = std::move( value );
        return idx;
    }

    /**
     * \brief Erase element based on its ID
     */
    void erase( id_type id ) {
        _elems[ id ] = std::nullopt;
        _freeIdxs.push_back( id );
    }

    iterator begin() noexcept {
        return iterator( this, _elems.begin() );
    }

    const_iterator begin() const noexcept {
        return const_iterator( this, _elems.begin() );
    }

    const_iterator cbegin() const noexcept {
        return const_iterator( this, _elems.cbegin() );
    }

    iterator end() noexcept {
        return iterator( this, _elems.end() );
    }

    const_iterator end() const noexcept {
        return const_iterator( this, _elems.end() );
    }

    const_iterator cend() const noexcept {
        return const_iterator( this, _elems.cend() );
    }

    /**
     * \brief Access element based on its ID
     */
    const_reference operator[]( id_type id ) const {
        return _elems[ id ].value();
    }

    /**
     * \brief Access element based on its ID
     */
    reference operator[]( id_type id ) {
        return _elems[ id ].value();
    }

private:
    Container _elems;
    std::vector< size_type > _freeIdxs;
};

} // namespace atoms