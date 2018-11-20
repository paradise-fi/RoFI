#pragma once
#include <Arduino.h>

struct InterruptGuard {
    InterruptGuard() { cli(); }
    ~InterruptGuard() { sei(); }

    InterruptGuard( const InterruptGuard& ) = delete;
    InterruptGuard& operator=( const InterruptGuard& ) = delete;
};

template < int size, int poolSize >
struct Blob_ {
    Blob_() : _mem( nullptr ) {}
    Blob_( const Blob_& ) = delete;
    Blob_& operator=( const Blob_& ) = delete;
    Blob_( Blob_&& o ): _mem( o._mem ) { o.free(); }
    Blob_& operator=( Blob_&& o ) { _mem = o._mem; o.free(); return *this; }
    ~Blob_() { free(); }

    const uint8_t& operator[]( uint8_t idx ) const { return _mem[ idx ]; }
    uint8_t& operator[]( uint8_t idx ) { return _mem[ idx ]; }
    operator bool(){ return _mem; }

    static Blob_ allocate() {
        InterruptGuard _;
        for ( uint8_t i = 0; i != poolSize; i++ ) {
            if ( !candidate( i ).taken ) {
                candidate( i ).taken = true;
                return Blob_( candidate( i ).memory );
            }
        }
        return Blob_( nullptr );
    }

    uint8_t* _mem;
private:
    Blob_( uint8_t* mem ): _mem( mem ) {}

    void free() {
        for ( uint8_t i = 0; i != poolSize; i++ ) {
            if ( candidate( i ).memory == _mem ) {
                candidate( i ).taken = false;
                _mem = nullptr;
                return;
            }
        }
    }

    struct BlobCandidate {
        uint8_t memory[ size ];
        bool taken;
    };

    static BlobCandidate& candidate( uint8_t i ) {
        static BlobCandidate _blobs[ poolSize ];
        return _blobs[ i ];
    }

};

using Blob = Blob_< 255, 5 >;