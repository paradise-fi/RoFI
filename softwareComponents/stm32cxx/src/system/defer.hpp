#pragma once

#include <stm32cxx.config.hpp>

#include <function2/function2.hpp>
#include <functional>
#include <system/ringBuffer.hpp>
#include <drivers/hal.hpp>
#include <system/irq.hpp>

class Defer {
public:
    using Job = fu2::unique_function< void( void ) >;
    static void job( Job j ) {
        instance()._schedule( 0, std::move( j ) );
    }

    static void schedule( int ms, Job j ) {
        instance()._schedule( ms, std::move( j ) );
    }

    static bool run() {
        return instance()._run();
    }

private:
    using Item = std::pair< uint32_t, Job >;
    Defer(): _queue( 32 )
    { }

    bool _run() {
        if ( _queue.empty() )
            return false;
        auto [ timePoint, j ] = _queue.pop_front();
        if ( static_cast< int >( timePoint - HAL_GetTick() ) <= 0 ) {
            assert( j );
            j();
        }
        else {
            // There can new jobs scheduled within an interrupt. Pulling out of the
            // queue is safe, however, pushing back again is not. Guard the
            // interrupts for pushing back
            // IrqGuard guard( STM32CXX_IRQ_HIGH_PRIORITY ); - TBA: Problem with USB priority
            IrqMask guard;
            _queue.push_back( { timePoint, std::move( j ) } );
        }
        return !_queue.empty();
    }

    void _schedule( int ms, Job j ) {
        bool result = _queue.push_back( { HAL_GetTick() + ms, std::move( j ) } );
        assert( result );
    }

    static Defer& instance() {
        static Defer d;
        return d;
    }

    RingBuffer< Item, memory::Pool > _queue;
};