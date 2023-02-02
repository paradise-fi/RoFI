#pragma once

#include <system/memory.hpp>

template < typename T, typename AllocatorT = memory::Pool >
class RingBuffer {
public:
    using Allocator = AllocatorT;
    using Mem = typename Allocator::Block;

    RingBuffer(): _head( 1 ), _tail( 0 ), _storageCapacity( 1 ) {}

    RingBuffer( Mem storage, int storageSize )
        : _head( 0 ), _tail( 0 ), _storage( std::move( storage ) ),
          _storageCapacity( storageSize / sizeof( T ) )
    {
        assert( _storage.get() );
        for ( int i = 0; i != _storageCapacity; i++ )
            new ( _data() + i ) T();
    }

    RingBuffer( int count ):
        RingBuffer( Allocator::allocate( sizeof( T ) * count ), sizeof( T ) * count )
    {}

    int capacity() const {
        return _storageCapacity - 1;
    }

    int size() const {
        if ( _head <= _tail )
            return _tail - _head;
        return _tail - _head + _storageCapacity;
    }

    int available() const {
        return capacity() - size();
    }

    bool empty() const {
        return _head == _tail;
    }

    bool full() const {
        return _next( _tail ) == _head;
    }

    T& operator[]( int idx ) {
        return _data()[ _index( idx ) ];
    }

    const T& operator[]( int idx ) const {
        return _data()[ _index( idx ) ];
    }

    /** Push the element into the queue and possibly throw away the oldest
     ** element if there is no space. Return false if overflow happened. **/
    bool push_back_force( T val ) {
        bool ret = true;
        if ( full() ) {
            _next( _head ); // Make room, discard first element
            ret = false;
        }

        _data()[ _tail ] = std::move( val );
        _tail = _next( _tail );
        return ret;
    }

    bool push_back( T val ) {
        if ( full() )
            return false;
        _data()[ _tail ] = std::move( val );
        _tail = _next( _tail );
        return true;
    }

    T pop_front() {
        int head = _head;
        _head = _next( _head );
        return std::move( _data()[ head ] );
    }

    // Return longest continuous segment of memory after tail
    std::pair< T*, int > insertPosition() {
        if ( _tail == _storageCapacity )
            return { _data(), _head - 1 };
        if ( _head <= _tail )
            return { _data() + _tail, std::min( _storageCapacity - _tail, available() ) };
        return { _data() + _tail, _head - _tail - 1 };
    }

    // Notify the ring buffer about newly inserted elements
    void advanceWrite( int pos ) {
        _tail += pos;
        if ( _tail >= _storageCapacity )
            _tail -= _storageCapacity;
    }

    // Return longest continuous segment of available elements
    std::pair< T*, int > readPosition() {
        if ( _head == _storageCapacity )
            return { _data(), _tail };
        if ( _head > _tail )
            return { _data() + _head, _storageCapacity - _head };
        return { _data() + _head, _tail - _head };
    }

    // Notify the ring buffer about popped elements
    void advanceRead( int pos ) {
        _head += pos;
        if ( _head >= _storageCapacity )
            _head -= _storageCapacity;
    }

    void clear() {
        _head = _tail = 0;
    }

private:
    int _next( int idx ) const {
        if ( ++idx == _storageCapacity )
            return 0;
        return idx;
    }

    int _index( int idx ) const {
        idx += _head;
        if ( idx >= _storageCapacity )
            idx -= _storageCapacity;
        return idx;
    }

    T *_data() { return reinterpret_cast< T *>( _storage.get() ); }

    int _head, _tail;
    Mem _storage;
    int _storageCapacity;
};
