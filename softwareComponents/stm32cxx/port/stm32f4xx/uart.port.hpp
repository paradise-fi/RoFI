#pragma once

#include <stm32f4xx_ll_bus.h>
#include <stm32f4xx_ll_rcc.h>
#include <stm32f4xx_ll_usart.h>
#include <stm32f4xx_ll_gpio.h>
#include <stm32f4xx_ll_dma.h>
#include <system/assert.hpp>

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
    constexpr const static inline bool supportsIdle = true;

    void configureFifo() {}

    void enableClock() {
        if ( self()._periph == USART1 )
            return LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_USART1 );
        if ( self()._periph == USART2 )
            return LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_USART2 );
        if ( self()._periph == USART6 )
            return LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_USART6 );
        impossible( "Unknown USART instance" );
    }

    void disableClock() {
        if ( self()._periph == USART1 )
            return LL_APB2_GRP1_DisableClock( LL_APB2_GRP1_PERIPH_USART1 );
        if ( self()._periph == USART2 )
            return LL_APB1_GRP1_DisableClock( LL_APB1_GRP1_PERIPH_USART2 );
        if ( self()._periph == USART6 )
            return LL_APB2_GRP1_DisableClock( LL_APB2_GRP1_PERIPH_USART6 );
        impossible( "Unknown USART instance" );
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
        impossible( "Not implemented" );
    }

    int _getPeriphClockSpeed() {
        LL_RCC_ClocksTypeDef rccClocks;
        LL_RCC_GetSystemClocksFreq( &rccClocks );
        if ( self()._periph == USART1 ) {
            return rccClocks.PCLK2_Frequency;
        }
        else if ( self()._periph == USART2 ) {
            return rccClocks.PCLK1_Frequency;
        }
        #if defined(USART3)
            else if ( self()._periph == USART3 ) {
                return rccClocks.PCLK1_Frequency;
            }
        #endif
        #if defined(USART6)
            else if ( self()._periph == USART6 ) {
                return rccClocks.PCLK2_Frequency;
            }
        #endif
        #if defined(UART4)
            else if ( self()._periph == UART4 ) {

                return rccClocks.PCLK1_Frequency;
            }
        #endif
        #if defined(UART5)
            else if ( self()._periph == UART5 ) {

                return rccClocks.PCLK1_Frequency;
            }
        #endif
        #if defined(UART7)
            else if ( self()._periph == UART7 ) {

                return rccClocks.PCLK1_Frequency;
            }
        #endif
        #if defined(UART8)
            else if ( self()._periph == UART8 ) {

                return rccClocks.PCLK1_Frequency;
            }
        #endif
        #if defined(UART9)
            else if ( self()._periph == UART9 ) {

                return rccClocks.PCLK2_Frequency;
            }
        #endif
        #if defined(UART10)
            else if ( self()._periph == UART10 ) {
                return rccClocks.PCLK2_Frequency;
            }
        #endif
        impossible( "Uknown peripheral" );
    }

    void setBaudrate( int baudrate ) {
        LL_USART_SetBaudRate( self()._periph,
            self()._getPeriphClockSpeed(),
            LL_USART_GetOverSampling( self()._periph ),
            baudrate );
    }

    void setDataBits( int bits ) {
        assert( bits == 8 || bits == 9 );
        auto b = bits == 8
            ? LL_USART_DATAWIDTH_8B
            : LL_USART_DATAWIDTH_9B;
        LL_USART_SetDataWidth( self()._periph, b );
    }

    void setParity( Parity parity ) {
        uint32_t llParity =
            parity == Parity::None ? LL_USART_PARITY_NONE :
            parity == Parity::Even ? LL_USART_PARITY_EVEN :
                    /*Parity::Odd*/  LL_USART_PARITY_ODD;
        LL_USART_SetParity( self()._periph, llParity );
    }

    void setStopBits( StopBits b ) {
        auto llStopBits =
            b == StopBits::N1   ? LL_USART_STOPBITS_1   :
            b == StopBits::N1_5 ? LL_USART_STOPBITS_1_5 :
              /* StopBits::N2 */  LL_USART_STOPBITS_2;
        LL_USART_SetStopBitsLength( self()._periph, llStopBits );
    }

    void _waitForEnable() {
        // This family does not need to wait for enabling
    }

    void enableTimeout() {
        LL_USART_EnableIT_IDLE( self()._periph );
    }

    template < typename Callback >
    void enableTimeout( Callback callback ) {
        self().handlers().rxTimeout = typename Self::Handler( callback );
        enableTimeout();
    }

    void disableTimout() {
        LL_USART_DisableIT_IDLE( self()._periph );
    }

    struct Handlers {
        typename Self::Handler rxTimeout;

        void _handleIsr( USART_TypeDef *uart ) {
            HANDLE_WITH( IDLE, rxTimeout );
        }
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
        impossible( "Invalid Rx channel specified" );
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
        impossible( "Invalid Tx channel specified" );
    }
};

} // namespace detail