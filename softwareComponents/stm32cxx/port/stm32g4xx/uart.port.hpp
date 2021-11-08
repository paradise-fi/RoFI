#pragma once

#include <stm32g4xx_ll_bus.h>
#include <stm32g4xx_ll_usart.h>

#include <drivers/gpio.hpp>
#include <drivers/dma.hpp>

#include <system/assert.hpp>

extern "C" void USART1_IRQHandler();
extern "C" void USART2_IRQHandler();
extern "C" void USART6_IRQHandler();

namespace detail {

template < typename Self >
class Uart {
public:
    constexpr const static inline bool supportsTimeout = true;

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
        else
            assert( false && "Unknown USART instance" );
    }

    void _waitForEnable() {
        while( (!LL_USART_IsActiveFlag_TEACK( self()._periph) )
            || (!LL_USART_IsActiveFlag_REACK( self()._periph) ) );
    }

    void _enableInterrupt( int priority = 128 ) {
        if ( self()._periph == USART1 ) {
            NVIC_SetPriority( USART1_IRQn, priority );
            NVIC_EnableIRQ( USART1_IRQn );
        }
        else if ( self()._periph == USART2 ) {
            NVIC_SetPriority( USART2_IRQn, priority );
            NVIC_EnableIRQ( USART2_IRQn );
        }
    }

    void enableTimeout() {
        LL_USART_EnableRxTimeout( self()._periph );
        LL_USART_EnableIT_RTO( self()._periph );
    }

    template < typename Callback >
    void enableTimeout( int bitDuration, Callback callback ) {
        LL_USART_SetRxTimeout( self()._periph, bitDuration );
        self().handlers().rxTimeout = typename Self::Handler( callback );
        enableTimeout();
    }

    void disableTimout() {
        LL_USART_DisableRxTimeout( self()._periph );
        LL_USART_DisableIT_RTO( self()._periph );
    }

    auto getDmaRxAddr() {
        return LL_USART_DMA_GetRegAddr( self()._periph, LL_USART_DMA_REG_DATA_RECEIVE );
    }

    auto getDmaTxAddr() {
        return LL_USART_DMA_GetRegAddr( self()._periph, LL_USART_DMA_REG_DATA_TRANSMIT );
    }
};

} // namespace detail


inline int LL_DMAMUX_REQ_RX( USART_TypeDef *uart ) {
    if ( uart == USART1 )
        return LL_DMAMUX_REQ_USART1_RX;
    if ( uart == USART2 )
        return LL_DMAMUX_REQ_USART2_RX;
    assert( false && "Invalid USART specified" );
    __builtin_trap();
}

inline int LL_DMAMUX_REQ_TX( USART_TypeDef *uart ) {
    if ( uart == USART1 )
        return LL_DMAMUX_REQ_USART1_TX;
    if ( uart == USART2 )
        return LL_DMAMUX_REQ_USART2_TX;
    assert( false && "Invalid USART specified" );
    __builtin_trap();
}