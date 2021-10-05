#pragma once

#include <system/memory.hpp>

template < typename T, typename AllocatorT >
class RingBuffer {
public:
    using Allocator = AllocatorT;
    using Mem = typename Allocator::Block;

    RingBuffer(): _head( 1 ), _tail( 0 ), _storageCapacity( 1 ) {}

    RingBuffer( Mem storage, int storageSize )
        : _head( 0 ), _tail( 0 ), _storage( std::move( storage ) ),
          _storageCapacity( storageSize / sizeof( T ) )
    {
        for ( int i = 0; i != _storageCapacity; i++ )
            new ( _data() + i ) T();
    }

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
    void advance( int pos ) {
        _tail += pos;
        if ( _tail >= _storageCapacity )
            _tail -= _storageCapacity;
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

