#pragma once

#include <stm32g0xx_ll_dma.h>
#include <stm32g0xx_ll_bus.h>
#include <cassert>

// STM32 LL is stupid. There is a special function for each flag for each
// channel. We do not want to make assumptions about arrangement of the bits,
// therefore... behold. C-macros.
#define GEN_FIRED( FLAG ) static bool _fired ## FLAG ( int channel ) { \
    if ( !LL_DMA_IsEnabledIT_ ## FLAG ( DMA1, channel ) )              \
        return false;                                                  \
    switch ( channel ) {                                               \
    case 1: return  LL_DMA_IsActiveFlag_ ## FLAG ## 1( DMA1 );         \
    case 2: return  LL_DMA_IsActiveFlag_ ## FLAG ## 2( DMA1 );         \
    case 3: return  LL_DMA_IsActiveFlag_ ## FLAG ## 3( DMA1 );         \
    case 4: return  LL_DMA_IsActiveFlag_ ## FLAG ## 4( DMA1 );         \
    case 5: return  LL_DMA_IsActiveFlag_ ## FLAG ## 5( DMA1 );         \
    case 6: return  LL_DMA_IsActiveFlag_ ## FLAG ## 6( DMA1 );         \
    case 7: return  LL_DMA_IsActiveFlag_ ## FLAG ## 7( DMA1 );         \
    }                                                                  \
    assert( false && "Invalid channel" );                              \
    __builtin_trap();                                                  \
}

#define GEN_CLEAR( FLAG ) static void _clear ## FLAG ( int channel ) { \
    switch ( channel ) {                                        \
    case 1: LL_DMA_ClearFlag_ ## FLAG ## 1( DMA1 ); break;      \
    case 2: LL_DMA_ClearFlag_ ## FLAG ## 2( DMA1 ); break;      \
    case 3: LL_DMA_ClearFlag_ ## FLAG ## 3( DMA1 ); break;      \
    case 4: LL_DMA_ClearFlag_ ## FLAG ## 4( DMA1 ); break;      \
    case 5: LL_DMA_ClearFlag_ ## FLAG ## 5( DMA1 ); break;      \
    case 6: LL_DMA_ClearFlag_ ## FLAG ## 6( DMA1 ); break;      \
    case 7: LL_DMA_ClearFlag_ ## FLAG ## 7( DMA1 ); break;      \
    }                                                           \
}

namespace detail {

template < typename Self >
class Dma {
protected:
    GEN_FIRED( HT );
    GEN_FIRED( TC );
    GEN_FIRED( TE );

    GEN_CLEAR( HT );
    GEN_CLEAR( TE );
    GEN_CLEAR( TC );

    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    static constexpr int channelCount = 7;
    static const constexpr bool supportsMuxing = true;

    template < typename ChSelf >
    class Channel {
        ChSelf& self() {
            return *static_cast< ChSelf * >( this );
        }

        const ChSelf& self() const {
            return *static_cast< ChSelf * >( this );
        }

        void enable() {
            LL_DMA_EnableChannel( self()._periph, self()._channel );
        }

        bool isEnabled() {
            return LL_DMA_IsEnabledChannel( self()._periph, self()._channel );
        }

        void disable() {
            LL_DMA_DisableChannel( self()._periph, self()._channel );
        }

        void enableInterrupt( int priority = 0 ) {
            assert( self()._periph == DMA1 );

            if ( self()._channel == 1 ) {
                NVIC_SetPriority( DMA1_Channel1_IRQn, priority );
                NVIC_EnableIRQ( DMA1_Channel1_IRQn );
            }
            else if ( self()._channel <= 3 ) {
                NVIC_SetPriority( DMA1_Channel2_3_IRQn, priority );
                NVIC_EnableIRQ( DMA1_Channel2_3_IRQn );
            }
            else if ( self()._channel <= 7 ) {
                NVIC_SetPriority( DMA1_Ch4_7_DMAMUX1_OVR_IRQn, priority );
                NVIC_EnableIRQ( DMA1_Ch4_7_DMAMUX1_OVR_IRQn );
            } else {
                assert( false && "Invalid channel" );
            }
        }

        void disableInterrupt( int priority = 0 ) {
            assert( self()._periph == DMA1 );

            if ( self()._channel == 1 ) {
                NVIC_DisableIRQ( DMA1_Channel1_IRQn );
            }
            else if ( self()._channel <= 3 ) {
                NVIC_DisableIRQ( DMA1_Channel2_3_IRQn );
            }
            else if ( self()._channel <= 7 ) {
                NVIC_DisableIRQ( DMA1_Ch4_7_DMAMUX1_OVR_IRQn );
            } else {
                assert( false && "Invalid channel" );
            }
        }

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

        void setPriority( uint32_t priority ) {
            LL_DMA_SetChannelPriorityLevel( self()._periph, self()._channel, priority );
        }
    };

    static void _enableClock( DMA_TypeDef *periph ) {
        assert( periph == DMA1 );
        LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_DMA1 );
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
            if ( _firedHT( channel ) ) {
                _clearHT( channel );
                self()._half();
            }

            if ( _firedTC( channel ) ) {
                _clearTC( channel );
                self()._complete();
            }

            if ( _firedTE( channel ) ) {
                _clearTE( channel );
                self()._error();
            }
        }
    };
};

} // namespace detail

#undef GEN_FIRED
#undef GEN_CLEAR
