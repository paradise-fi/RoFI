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
template < typename T >
class HandleSet {
    using Container = std::vector< std::optional< T > >;

public:
    enum class handle_type : typename Container::size_type {};

private:
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

        Iterator(): _set( nullptr ), _it() {}
        Iterator( HandleSetPtr set, UnderlayingIterator it ):
            _set( set ), _it( it )
        {
            assert( _set );
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

        reference operator*() const {
            assert( _it->has_value() && "No value held" );
            return _it->value();
        }

        pointer operator->() const {
            assert( _it->has_value() && "No value held" );
            return &_it->value();
        }

        bool operator==( const Iterator& other ) const noexcept {
            return _it == other._it;
        }
        bool operator!=( const Iterator& other ) const noexcept {
            return !( *this == other );
        }

        handle_type get_handle() const noexcept {
            assert( _set );
            return static_cast< handle_type >( _it - _set->_elems.begin() );
        }

    private:
        void afterIncrement() {
            assert( _set );
            while ( _it != _set->_elems.end() && !_it->has_value() )
                _it++;
        }

        void afterDecrement() {
            assert( _set );
            while( _it != _set->_elems.begin() && !_it->has_value() )
                _it--;
        }

        HandleSetPtr _set;
        UnderlayingIterator _it;
    };

public:
    using value_type = T;
    using size_type = typename Container::size_type;
    using difference_type = typename Container::difference_type;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = Iterator< false >;
    using const_iterator = Iterator< true >;

    /**
     * \brief Exchange the contents of the container with `other`
     */
    void swap( HandleSet& other ) {
        using std::swap;
        swap( _elems, other._elems );
        swap( _freeHandles, other._freeHandles );
    }

    /**
     * \brief Check if the container has no elements
     */
    [[nodiscard]] bool empty() const {
        return size() == 0;
    }

    /**
     * \brief Return the number of elements in the container
     */
    size_type size() const {
        return _elems.size() - _freeHandles.size();
    }

    /**
     * \brief Request the removal of unused capacity
     */
    void shrink_to_fit() {
        _elems.shrink_to_fit();
        _freeHandles.shrink_to_fit();
    }

    /**
     * \brief Increase capacity so it's greater or equal to `newCapacity`
     */
    void reserve( size_type newCapacity ) {
        _elems.reserve( newCapacity );
    }

    /**
     * \brief Erase all elements from the container
     */
    void clear() {
        _elems.clear();
        _freeHandles.clear();
    }

    /**
     * \brief Emplace new element and get its handle
     */
    template< typename... Args >
    handle_type emplace( Args&&... args ) {
        if ( _freeHandles.empty() ) {
            _elems.emplace_back( std::forward< Args >( args )... );
            return static_cast< handle_type >( _elems.size() - 1 );
        }
        handle_type handle = _freeHandles.back();
        _freeHandles.pop_back();
        _elems[ static_cast< size_type >( handle ) ].emplace( std::forward< Args >( args )... );
        return handle;
    }

    /**
     * \brief Insert new element and get its handle
     */
    handle_type insert( const T& value ) {
        return emplace( value );
    }

    /**
     * \brief Insert new element and get its handle
     */
    handle_type insert( T&& value )
    {
        return emplace( std::move( value ) );
    }

    /**
     * \brief Erase element based on its handle
     */
    void erase( handle_type handle ) {
        _elems[ static_cast< size_type >( handle ) ].reset();
        _freeHandles.push_back( handle );
    }

    /**
     * \brief Return an iterator to the first element
     */
    iterator begin() noexcept {
        return iterator( this, _elems.begin() );
    }

    /**
     * \brief Return an iterator to the first element
     */
    const_iterator begin() const noexcept {
        return const_iterator( this, _elems.begin() );
    }

    /**
     * \brief Return an iterator to the first element
     */
    const_iterator cbegin() const noexcept {
        return const_iterator( this, _elems.cbegin() );
    }

    /**
     * \brief Return the past-the-end iterator
     */
    iterator end() noexcept {
        return iterator( this, _elems.end() );
    }

    /**
     * \brief Return the past-the-end iterator
     */
    const_iterator end() const noexcept {
        return const_iterator( this, _elems.end() );
    }

    /**
     * \brief Return the past-the-end iterator
     */
    const_iterator cend() const noexcept {
        return const_iterator( this, _elems.cend() );
    }

    /**
     * \brief Access element based on its handle
     */
    const_reference operator[]( handle_type handle ) const {
        assert( contains( handle ) );
        return _elems[ static_cast< size_type >( handle ) ].value();
    }

    /**
     * \brief Access element based on its handle
     */
    reference operator[]( handle_type handle ) {
        assert( contains( handle ) );
        return _elems[ static_cast< size_type >( handle ) ].value();
    }

    /**
     * \brief Check if the container contains element based on its handle
     */
    bool contains( handle_type handle ) const {
        auto index = static_cast< size_type >( handle );
        return index < _elems.size() && _elems[ index ].has_value();
    }

    /**
     * \brief Return iterator to element based on its handle
     * Returns past-the-end iterator if no such element exists
     */
    iterator find( handle_type handle ) {
        if ( contains( handle ) ) {
            return iterator( this, _elems.begin() + static_cast< size_type >( handle ) );
        }
        return end();
    }
    /**
     * \brief Return iterator to element based on its handle
     * Returns past-the-end iterator if no such element exists
     */
    const_iterator find( handle_type handle ) const {
        if ( contains( handle ) ) {
            return const_iterator( this, _elems.begin() + static_cast< size_type >( handle ) );
        }
        return end();
    }

private:
    Container _elems;
    std::vector< handle_type > _freeHandles;
};

} // namespace atoms
