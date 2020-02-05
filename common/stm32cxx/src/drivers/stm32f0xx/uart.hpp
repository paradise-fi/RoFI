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

    static constexpr int handlerCount = 2;

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

    static auto& handlers( USART_TypeDef *u ) {
        if ( u == USART1 )
            return Self::_uarts[ 0 ];
        if ( u == USART2 )
            return Self::_uarts[ 1 ];
        assert( false && "Invalid USART specified" );
    }
};

template < typename Self >
struct TxOn {
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    int alternativeFun( USART_TypeDef *periph ) {
        if ( periph == USART1 ) {
            if ( self()._pin._periph == GPIOB && self()._pin._pos == 6 )
                return LL_GPIO_AF_0;
        }
        // ToDo: Implement more
        assert( false && "Incorrect TX pin" );
    }
};

template < typename Self >
struct RxOn {
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    int alternativeFun( USART_TypeDef *periph ) {
        if ( periph == USART1 ) {
            if ( self()._pin._periph == GPIOB && self()._pin._pos == 7 )
                return LL_GPIO_AF_0;
        }
        // ToDo: Implement more
        assert( false && "Incorrect RX pin" );
    }
};

} // namespace detail

inline int LL_DMAMUX_REQ_RX( USART_TypeDef * /* uart */ ) {
    assert( false && "Invalid USART specified" );
}

inline int LL_DMAMUX_REQ_TX( USART_TypeDef * /* uart */ ) {
    assert( false && "Invalid USART specified" );
}