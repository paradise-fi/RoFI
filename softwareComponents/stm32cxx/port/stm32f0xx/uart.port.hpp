#pragma once

#include <stm32f0xx_ll_bus.h>
#include <stm32f0xx_ll_usart.h>
#include <stm32f0xx_ll_gpio.h>
#include <cassert>

namespace detail {

template < typename Self >
class Uart {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    struct Handlers {
        typename Self::Handler rxTimeout;

        void _handleIsr( USART_TypeDef *uart ) {
            HANDLE_WITH( RTO, rxTimeout );
        }
    };

    void configureFifo() {}

    void enableClock() {
        if ( self()._periph == USART1 )
            LL_APB1_GRP2_EnableClock( LL_APB1_GRP2_PERIPH_USART1 );
        else
            assert( false && "Unknown USART instance" );
    }

    void disableClock() {
        if ( self()._periph == USART1 )
            LL_APB1_GRP2_DisableClock( LL_APB1_GRP2_PERIPH_USART1 );
        else
            assert( false && "Unknown USART instance" );
    }

    void _enableInterrupt( int priority = 0 ) {
        if ( self()._periph == USART1 ) {
            NVIC_SetPriority( USART1_IRQn, priority );
            NVIC_EnableIRQ( USART1_IRQn );
        }
        else
            assert( false && "Not implemented" );
    }

    void enableTimeout() {
        LL_USART_EnableRxTimeout( self()._periph );
        LL_USART_EnableIT_RTO( self()._periph );
    }

    template < typename Callback >
    void enableTimeout( int bitDuration, Callback callback ) {
        LL_USART_SetRxTimeout( self()._periph, bitDuration );
        self().handlers().rxTimeout = Handler( callback );
        enableTimeout();
    }

    void disableTimout() {
        LL_USART_DisableRxTimeout( self()._periph );
        LL_USART_DisableIT_RTO( self()._periph );
    }
};

} // namespace detail

inline int LL_DMAMUX_REQ_RX( USART_TypeDef * /* uart */ ) {
    assert( false && "Invalid USART specified" );
    __builtin_trap();
}

inline int LL_DMAMUX_REQ_TX( USART_TypeDef * /* uart */ ) {
    assert( false && "Invalid USART specified" );
    __builtin_trap();
}