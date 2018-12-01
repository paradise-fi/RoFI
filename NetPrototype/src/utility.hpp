#pragma once

#include <cstdint>
#include <iostream>
#include <string>

template < typename T >
T& as( void* p ) {
    return *reinterpret_cast< T * >( p );
}

template < typename T >
const T& as( const void* p ) {
    return *reinterpret_cast< const T * >( p );
}



struct ScopeTracker {
    ScopeTracker( const char* name ) : _name( name ), _indent( ++getCounter() ) {
        std::cout << indent( _indent -1 ) << "START: " << _name << "\n";
    }

    ~ScopeTracker() {
        std::cout << indent( _indent -1 ) << "END: " << _name << "\n";
        getCounter()--;
    }

    std::ostream& operator()() {
        return std::cout << indent( _indent );
    }

    static int& getCounter() {
        static int i = 0;
        return i;
    }

    std::string indent( int i ) {
        return std::string( 2 * i, ' ' );
    }

    const char* _name;
    int _indent;
};

struct DummyScopeTracker {
    template < typename T >
    DummyScopeTracker operator<<( const T& ) { return {}; }
    DummyScopeTracker operator()() { return {}; }
};

struct TransactionTracker {
    TransactionTracker( const char* name ) : _name( name ), _id( ++getCounter() ) {
        //std::cout << "START: " << _name << " " << _id << "\n";
    }

    ~TransactionTracker() {
       // std::cout << "END: " << _name << " " << _id << "\n";
    }

    std::ostream& operator()() {
        return std::cout;// << _id << ": ";
    }

    static int& getCounter() {
        static int i = 0;
        return i;
    }

    const char* _name;
    int _id;
};

// #define TRACK() ScopeTracker tr( __func__ );
// #define TRACK() auto tr = []()-> std::ostream& { return std::cout; }; tr();
// #define TRACK() DummyScopeTracker tr; tr();
#define TRACK() TransactionTracker tr( __func__ );