#pragma once

#include <stm32f4xx_ll_bus.h>
#include <stm32f4xx_ll_usart.h>
#include <stm32f4xx_ll_gpio.h>
#include <stm32f4xx_ll_dma.h>
#include <cassert>

extern "C" void USART1_IRQHandler();
extern "C" void USART2_IRQHandler();
extern "C" void USART6_IRQHandler();

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

    constexpr const static inline bool supportsTimeout = false;

    void configureFifo() {}

    void enableClock() {
        if ( self()._periph == USART1 )
            return LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_USART1 );
        if ( self()._periph == USART2 )
            return LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_USART2 );
        if ( self()._periph == USART6 )
            return LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_USART6 );
        assert( false && "Unknown USART instance" );
    }

    void disableClock() {
        if ( self()._periph == USART1 )
            return LL_APB2_GRP1_DisableClock( LL_APB2_GRP1_PERIPH_USART1 );
        if ( self()._periph == USART2 )
            return LL_APB1_GRP1_DisableClock( LL_APB1_GRP1_PERIPH_USART2 );
        if ( self()._periph == USART6 )
            return LL_APB2_GRP1_DisableClock( LL_APB2_GRP1_PERIPH_USART6 );
        assert( false && "Unknown USART instance" );
    }

    void _enableInterrupt( int priority = 255 ) {
        if ( self()._periph == USART1 ) {
            NVIC_SetPriority( USART1_IRQn, priority );
            NVIC_EnableIRQ( USART1_IRQn );
            return;
        }
        if ( self()._periph == USART2 ) {
            NVIC_SetPriority( USART2_IRQn, priority );
            NVIC_EnableIRQ( USART2_IRQn );
            return;
        }
        if ( self()._periph == USART6 ) {
            NVIC_SetPriority( USART6_IRQn, priority );
            NVIC_EnableIRQ( USART6_IRQn );
            return;
        }
        assert( false && "Not implemented" );
    }

    void _waitForEnable() {
        // This family does not need to wait for enabling
    }

    struct Handlers {
        // This family has no special features, thus no extra handlers are
        // needed.
        void _handleIsr( USART_TypeDef *uart ) {}
    };

    auto getDmaRxAddr() {
        return LL_USART_DMA_GetRegAddr( self()._periph );
    }

    auto getDmaTxAddr() {
        return LL_USART_DMA_GetRegAddr( self()._periph );
    }

    auto getDmaRxTrigger( DMA_TypeDef *dma, int channel ) {
        if ( self()._periph == USART1 ) {
            if ( dma == DMA2 ) {
                if ( channel == LL_DMA_STREAM_2 )
                    return LL_DMA_CHANNEL_4;
                if ( channel == LL_DMA_STREAM_5 )
                    return LL_DMA_CHANNEL_4;
            }
        }
        if ( self()._periph == USART2 ) {
            if ( dma == DMA1 ) {
                if ( channel == LL_DMA_STREAM_5 )
                    return LL_DMA_CHANNEL_4;
                if ( channel == LL_DMA_STREAM_7 )
                    return LL_DMA_CHANNEL_6;
            }
        }
        if ( self()._periph == USART6 ) {
            if ( dma == DMA2 ) {
                if ( channel == LL_DMA_STREAM_1 )
                    return LL_DMA_CHANNEL_5;
                if ( channel == LL_DMA_STREAM_2 )
                    return LL_DMA_CHANNEL_5;
            }
        }
        assert( false && "Invalid Rx channel specified" );
    }

    auto getDmaTxTrigger( DMA_TypeDef *dma, int channel ) {
        if ( self()._periph == USART1 ) {
            if ( dma == DMA2 ) {
                if ( channel == LL_DMA_STREAM_7 )
                    return LL_DMA_CHANNEL_4;
            }
        }
        if ( self()._periph == USART2 ) {
            if ( dma == DMA1 ) {
                if ( channel == LL_DMA_STREAM_6 )
                    return LL_DMA_CHANNEL_4;
            }
        }
        if ( self()._periph == USART6 ) {
            if ( dma == DMA2 ) {
                if ( channel == LL_DMA_STREAM_6 )
                    return LL_DMA_CHANNEL_5;
                if ( channel == LL_DMA_STREAM_7 )
                    return LL_DMA_CHANNEL_5;
            }
        }
        assert( false && "Invalid Tx channel specified" );
    }
};

} // namespace detail