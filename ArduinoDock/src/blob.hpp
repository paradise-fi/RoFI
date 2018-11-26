#pragma once
#include <Arduino.h>

struct InterruptGuard {
    InterruptGuard() { cli(); }
    ~InterruptGuard() { sei(); }

    InterruptGuard( const InterruptGuard& ) = delete;
    InterruptGuard& operator=( const InterruptGuard& ) = delete;
};

template < uint8_t smallSize, uint8_t largeSize, uint8_t smallPoolSize, uint8_t largePoolSize >
struct Blob_ {
    Blob_() : _mem( nullptr ) {}
    Blob_( const Blob_& ) = delete;
    Blob_& operator=( const Blob_& ) = delete;
    Blob_( Blob_&& o ): _mem( nullptr ) { swap( o ); }
    Blob_& operator=( Blob_&& o ) { swap( o ); return *this; }
    ~Blob_() { free(); }

    const uint8_t& operator[]( uint8_t idx ) const { return _mem[ idx ]; }
    uint8_t& operator[]( uint8_t idx ) { return _mem[ idx ]; }
    operator bool(){ return _mem; }

    static Blob_ allocate( uint8_t size ) {
        if ( size <= smallSize )
            return _allocate< smallSize >();
        if ( size <= largeSize )
            return _allocate< largeSize >();
        return Blob_( nullptr );
    }

    void swap( Blob_& o ) {
        auto x = _mem;
        _mem = o._mem;
        o._mem = x;
    }

    void free() {
        _free< smallSize >();
        _free< largeSize >();
    }

    template < typename T >
    T& as( uint8_t offset ) {
        return *reinterpret_cast< T * >( _mem + offset );
    }

    uint8_t* _mem;
private:
    Blob_( uint8_t* mem ): _mem( mem ) {}

    template < uint8_t size >
    void _free() {
        if ( !_mem )
            return;
        for ( uint8_t i = 0; i != poolSize< size >(); i++ ) {
            if ( candidate< size >( i ).memory == _mem ) {
                candidate< size >( i ).taken = false;
                _mem = nullptr;
                return;
            }
        }
    }

    template < uint8_t size >
    static Blob_ _allocate() {
        InterruptGuard _;
        for ( uint8_t i = 0; i != poolSize< size >(); i++ ) {
            if ( !candidate< size >( i ).taken ) {
                candidate< size >( i ).taken = true;
                return Blob_( candidate< size >( i ).memory );
            }
        }
        return Blob_( nullptr );
    }

    template < uint8_t size >
    struct BlobCandidate {
        uint8_t memory[ size ];
        bool taken;
    };

    template < uint8_t s >
    static constexpr uint8_t poolSize() {
        return s == smallSize ? smallPoolSize : largePoolSize;
    };

    template < uint8_t size >
    static BlobCandidate< size >& candidate( uint8_t i ) {
        static BlobCandidate< size > _blobs[ poolSize< size >() ];
        return _blobs[ i ];
    }

};

using Blob = Blob_< 32, 255, 2, 5 >;