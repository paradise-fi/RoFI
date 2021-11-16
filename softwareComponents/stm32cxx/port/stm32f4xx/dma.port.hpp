#pragma once

#pragma once

#include <stm32f4xx_ll_dma.h>
#include <stm32f4xx_ll_bus.h>
#include <system/assert.hpp>

// STM32 LL is stupid. There is a special function for each flag for each
// channel. We do not want to make assumptions about arrangement of the bits,
// therefore... behold. C-macros.
#define GEN_FIRED( FLAG ) \
static bool _fired ## FLAG ( DMA_TypeDef *dma, int channel ) { \
    if ( !LL_DMA_IsEnabledIT_ ## FLAG ( dma, channel ) )       \
        return false;                                          \
    switch ( channel ) {                                       \
    case 0: return  LL_DMA_IsActiveFlag_ ## FLAG ## 0( dma );  \
    case 1: return  LL_DMA_IsActiveFlag_ ## FLAG ## 1( dma );  \
    case 2: return  LL_DMA_IsActiveFlag_ ## FLAG ## 2( dma );  \
    case 3: return  LL_DMA_IsActiveFlag_ ## FLAG ## 3( dma );  \
    case 4: return  LL_DMA_IsActiveFlag_ ## FLAG ## 4( dma );  \
    case 5: return  LL_DMA_IsActiveFlag_ ## FLAG ## 5( dma );  \
    case 6: return  LL_DMA_IsActiveFlag_ ## FLAG ## 6( dma );  \
    case 7: return  LL_DMA_IsActiveFlag_ ## FLAG ## 7( dma );  \
    }                                                          \
    assert( false && "Invalid channel" );                      \
    __builtin_trap();                                          \
}

#define GEN_CLEAR( FLAG ) \
static void _clear ## FLAG ( DMA_TypeDef *dma, int channel ) { \
    switch ( channel ) {                                       \
    case 0: LL_DMA_ClearFlag_ ## FLAG ## 0( dma ); break;      \
    case 1: LL_DMA_ClearFlag_ ## FLAG ## 1( dma ); break;      \
    case 2: LL_DMA_ClearFlag_ ## FLAG ## 2( dma ); break;      \
    case 3: LL_DMA_ClearFlag_ ## FLAG ## 3( dma ); break;      \
    case 4: LL_DMA_ClearFlag_ ## FLAG ## 4( dma ); break;      \
    case 5: LL_DMA_ClearFlag_ ## FLAG ## 5( dma ); break;      \
    case 6: LL_DMA_ClearFlag_ ## FLAG ## 6( dma ); break;      \
    case 7: LL_DMA_ClearFlag_ ## FLAG ## 7( dma ); break;      \
    }                                                          \
}


namespace detail {

// One important note; F4 family calls "channels" by "streams"
template < typename Self >
class Dma {
protected:
    GEN_FIRED( HT );
    GEN_FIRED( TC );
    GEN_FIRED( TE );

    GEN_CLEAR( HT );
    GEN_CLEAR( TE );
    GEN_CLEAR( TC );
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    static constexpr int channelCount = 8;
    static constexpr bool supportsMuxing = false;

    static const constexpr IRQn_Type dma1Irq[8] = {
        DMA1_Stream0_IRQn,
        DMA1_Stream1_IRQn,
        DMA1_Stream2_IRQn,
        DMA1_Stream3_IRQn,
        DMA1_Stream4_IRQn,
        DMA1_Stream5_IRQn,
        DMA1_Stream6_IRQn,
        DMA1_Stream7_IRQn
    };

    static const constexpr IRQn_Type dma2Irq[8] = {
        DMA2_Stream0_IRQn,
        DMA2_Stream1_IRQn,
        DMA2_Stream2_IRQn,
        DMA2_Stream3_IRQn,
        DMA2_Stream4_IRQn,
        DMA2_Stream5_IRQn,
        DMA2_Stream6_IRQn,
        DMA2_Stream7_IRQn
    };

    template < typename ChSelf >
    class Channel {
        ChSelf& self() {
            return *static_cast< ChSelf * >( this );
        }

        const ChSelf& self() const {
            return *static_cast< ChSelf * >( this );
        }
    public:
        void enable() {
            LL_DMA_EnableStream( self()._periph, self()._channel );
        }

        bool isEnabled() {
            return LL_DMA_IsEnabledStream( self()._periph, self()._channel );
        }

        void disable() {
            LL_DMA_DisableStream( self()._periph, self()._channel );
        }

        void enableInterrupt( int priority = 0 ) {
            IRQn_Type irq =
                self()._periph == DMA1 ? dma1Irq[ self()._channel ] :
                                         dma2Irq[ self()._channel ];
            NVIC_SetPriority( irq, priority );
            NVIC_EnableIRQ( irq );
        }

        void disableInterrupt() {
            IRQn_Type irq =
                self()._periph == DMA1 ? dma1Irq[ self()._channel ] :
                                         dma2Irq[ self()._channel ];
            NVIC_DisableIRQ( irq );
        }

        void setPriority( uint32_t priority ) {
            LL_DMA_SetStreamPriorityLevel( self()._periph, self()._channel, priority );
        }

        void setTrigger( uint32_t trigger ) {
            LL_DMA_SetChannelSelection( self()._periph, self()._channel, trigger );
        }
    protected:
        void _enableOnComplete() {
            LL_DMA_EnableIT_TC( self()._periph, self()._channel );
        }

        void _disableOnComplete() {
            LL_DMA_DisableIT_TC( self()._periph, self()._channel );
        }

        void _enableOnHalf() {
            LL_DMA_EnableIT_HT( self()._periph, self()._channel );
        }

        void _disableOnHalf() {
            LL_DMA_DisableIT_TC( self()._periph, self()._channel );
        }

        void _enableOnError() {
            LL_DMA_EnableIT_TE( self()._periph, self()._channel );
        }

        void _disableOnError() {
            LL_DMA_DisableIT_TE( self()._periph, self()._channel );
        }
    };


    static void _enableClock( DMA_TypeDef *periph ) {
        if ( periph == DMA1 )
            return LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_DMA1 );
        if ( periph == DMA2 )
            return LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_DMA2 );
        assert( false && "Invalid DMA instance" );
    }


    template < typename ChSelf >
    struct ChannelData {
        ChSelf& self() {
            return *static_cast< ChSelf * >( this );
        }
        const ChSelf& self() const {
            return *static_cast< ChSelf * >( this );
        }

        void _handleIsr( DMA_TypeDef *periph, int channel ) {
            if ( _firedHT( periph, channel ) ) {
                _clearHT( periph, channel );
                self()._half();
            }

            if ( _firedTC( periph, channel ) ) {
                _clearTC( periph, channel );
                self()._complete();
            }

            if ( _firedTE( periph, channel ) ) {
                _clearTE( periph, channel );
                self()._error();
            }
        }
    };
};

} // namespace detail