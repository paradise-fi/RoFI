#pragma once

#include <function2/function2.hpp>
#include <functional>
#include <system/ringBuffer.hpp>

class Defer {
public:
    using Job = fu2::unique_function< void( void ) >;
    static void job( Job j ) {
        instance()._queue.push_back( std::move( j ) );
    }

    static bool run() {
        return instance()._run();
    }

private:
    Defer(): _queue( memory::Pool::allocate( 64 ), 64 )
    { }

    bool _run() {
        if ( !_queue.empty() )
            _queue.pop_front()();
        return !_queue.empty();
    }

    static Defer& instance() {
        static Defer d;
        return d;
    }

    RingBuffer< Job > _queue;
};