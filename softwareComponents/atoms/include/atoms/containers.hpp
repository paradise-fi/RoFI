#pragma once

#include <vector>
#include <optional>
#include <cassert>

namespace atoms {

/**
 * \brief std::vector-based set representation providing reusable integer handles
 * for the inserted elements.
 *
 * You are supposed to insert elements via HandleSet::insert(), which returns unique
 * element handle. Then you can acces this element using HandleSet::operator[]() or
 * erase it using HandleSet::erase(). The insertion is performed in amortized
 * constant time, deletion and element access is performed in constant time.
 *
 * Otherwise this container behaves the same as containers from the standard
 * library.
 */
template < typename T, typename H = long unsigned int >
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
    using handle_type = H;
    using difference_type = typename Container::difference_type;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = Iterator< false >;
    using const_iterator = Iterator< true >;

    //enum class handle_type : size_type {};

    void swap( HandleSet& other ) {
        using std::swap;
        swap( _elems, other._elems );
        swap( _freeHandles, other._freeHandles );
    }

    size_type size() const {
        return _elems.size() - _freeHandles.size();
    }

    void shrink_to_fit() {
        _elems.shrink_to_fit();
        _freeHandles.shrink_to_fit();
    }

    /**
     * \brief Insert new element and get its handle
     */
    handle_type insert( const T& value ) {
        if ( _freeHandles.empty() ) {
            _elems.push_back( value );
            return static_cast< handle_type >( _elems.size() - 1 );
        }
        handle_type handle = _freeHandles.back();
        _freeHandles.pop_back();
        _elems[ static_cast< size_type >( handle ) ] = value;
        return handle;
    }

    /**
     * \brief Insert new element and get its handle
     */
    template< typename TT = T>
    auto insert( T&& value )
        -> std::enable_if_t<
            std::is_move_constructible_v< TT > && std::is_move_assignable_v< TT >,
            handle_type >
    {
        if ( _freeHandles.empty() ) {
            _elems.push_back( std::move( value ) );
            return static_cast< handle_type >( _elems.size() - 1 );
        }
        handle_type handle = _freeHandles.back();
        _freeHandles.pop_back();
        _elems[ static_cast< size_type >( handle ) ] = std::move( value );
        return handle;
    }

    /**
     * \brief Erase element based on its handle
     */
    void erase( handle_type handle ) {
        _elems[ static_cast< size_type >( handle ) ] = std::nullopt;
        _freeHandles.push_back( handle );
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
     * \brief Access element based on its handle
     */
    const_reference operator[]( handle_type handle ) const {
        return _elems[ static_cast< size_type >( handle ) ].value();
    }

    /**
     * \brief Access element based on its handle
     */
    reference operator[]( handle_type handle ) {
        return _elems[ static_cast< size_type >( handle ) ].value();
    }

private:
    Container _elems;
    std::vector< handle_type > _freeHandles;
};

} // namespace atoms