#pragma once

#include <cassert>
#include <condition_variable>
#include <deque>
#include <initializer_list>
#include <mutex>


template < typename T >
struct ConcurrentQueue
{
    T pop()
    {
        std::unique_lock< std::mutex > lk( _m );
        _popSig.wait( lk, [ this ] { return !_q.empty(); } );

        auto r = std::move( _q.front() );
        _q.pop_front();
        return r;
    }

    void push( const T & x )
    {
        {
            std::lock_guard< std::mutex > lk( _m );
            _q.push_back( x );
        }
        _popSig.notify_one();
    }

    template < typename... Args >
    void emplace( Args &&... args )
    {
        {
            std::lock_guard< std::mutex > lk( _m );
            _q.emplace_back( std::forward< Args >( args )... );
        }
        _popSig.notify_one();
    }

    void push( T && x )
    {
        {
            std::lock_guard< std::mutex > lk( _m );
            _q.push_back( std::move( x ) );
        }
        _popSig.notify_one();
    }

    template < typename C >
    void push( const C & c )
    {
        {
            std::lock_guard< std::mutex > lk( _m );
            _q.insert( _q.end(), c.begin(), c.end() );
        }
        _popSig.notify_all();
    }

    void push( std::initializer_list< T > il )
    {
        push<>( il );
    }

    bool empty() const
    {
        std::lock_guard< std::mutex > lk( _m );
        return _q.empty();
    }

    void clear()
    {
        std::lock_guard< std::mutex > lk( _m );
        _q.clear();
    }

private:
    std::deque< T > _q;

    mutable std::mutex _m;
    std::condition_variable _popSig;
};
