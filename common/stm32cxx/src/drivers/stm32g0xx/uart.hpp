#pragma once

#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_usart.h>

#include <drivers/gpio.hpp>
#include <drivers/dma.hpp>

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

    void configureFifo() {
        LL_USART_SetTXFIFOThreshold( self()._periph, LL_USART_FIFOTHRESHOLD_1_8 );
        LL_USART_SetRXFIFOThreshold( self()._periph, LL_USART_FIFOTHRESHOLD_1_8 );
        LL_USART_EnableFIFO( self()._periph );
    }

    void enableClock() {
        if ( self()._periph == USART1 )
            LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_USART1 );
        else if ( self()._periph == USART2 )
            LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_USART2 );
        else if ( self()._periph == USART3 )
            LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_USART3 );
        else if ( self()._periph == USART4 )
            LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_USART4 );
        else
            assert( false && "Unknown USART instance" );
    }

    void disableClock() {
        if ( self()._periph == USART1 )
            LL_APB2_GRP1_DisableClock( LL_APB2_GRP1_PERIPH_USART1 );
        else if ( self()._periph == USART2 )
            LL_APB1_GRP1_DisableClock( LL_APB1_GRP1_PERIPH_USART2 );
        else if ( self()._periph == USART3 )
            LL_APB1_GRP1_DisableClock( LL_APB1_GRP1_PERIPH_USART3 );
        else if ( self()._periph == USART4 )
            LL_APB1_GRP1_DisableClock( LL_APB1_GRP1_PERIPH_USART4 );
        else
            assert( false && "Unknown USART instance" );
    }

    void _enableInterrupt( int priority = 0 ) {
        if ( self()._periph == USART1 ) {
            NVIC_SetPriority( USART1_IRQn, priority );
            NVIC_EnableIRQ( USART1_IRQn );
        }
        else if ( self()._periph == USART2 ) {
            NVIC_SetPriority( USART2_IRQn, priority );
            NVIC_EnableIRQ( USART2_IRQn );
        }
        else if ( self()._periph == USART3 || self()._periph == USART4 ) {
            NVIC_SetPriority( USART3_4_LPUART1_IRQn, priority );
            NVIC_EnableIRQ( USART3_4_LPUART1_IRQn );
        }
    }

    static auto& handlers( USART_TypeDef *u ) {
        if ( u == USART1 )
            return Self::_uarts[ 0 ];
        if ( u == USART2 )
            return Self::_uarts[ 1 ];
        if ( u == USART3 )
            return Self::_uarts[ 2 ];
        if ( u == USART3 )
            return Self::_uarts[ 3 ];
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
            if ( self()._pin._pos == 6 )
                return LL_GPIO_AF_0;
        }
        else if ( periph == USART2 ) {
            if ( self()._pin._pos == 2 )
                return LL_GPIO_AF_1;
        }
        // ToDo: More configurations
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
            if ( self()._pin._pos == 7 )
                return LL_GPIO_AF_0;
        }
        else if ( periph == USART2 ) {
            if ( self()._pin._pos == 3 )
                return LL_GPIO_AF_1;
        }
        // ToDo: More configurations
        assert( false && "Incorrect RX pin" );
    }
};

} // namespace detail


inline int LL_DMAMUX_REQ_RX( USART_TypeDef *uart ) {
    if ( uart == USART1 )
        return LL_DMAMUX_REQ_USART1_RX;
    if ( uart == USART2 )
        return LL_DMAMUX_REQ_USART2_RX;
    if ( uart == USART3 )
        return LL_DMAMUX_REQ_USART3_RX;
    if ( uart == USART4 )
        return LL_DMAMUX_REQ_USART4_RX;
    assert( false && "Invalid USART specified" );
}

inline int LL_DMAMUX_REQ_TX( USART_TypeDef *uart ) {
    if ( uart == USART1 )
        return LL_DMAMUX_REQ_USART1_TX;
    if ( uart == USART2 )
        return LL_DMAMUX_REQ_USART2_TX;
    if ( uart == USART3 )
        return LL_DMAMUX_REQ_USART3_TX;
    if ( uart == USART4 )
        return LL_DMAMUX_REQ_USART4_TX;
    assert( false && "Invalid USART specified" );
}