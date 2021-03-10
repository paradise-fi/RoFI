#pragma once

#include <stm32g0xx_ll_dma.h>
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
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    static constexpr int channelCount = 7;

    static void enableInterrupt( int channel, int priority ) {
        if ( channel == 1 ) {
            NVIC_SetPriority( DMA1_Channel1_IRQn, priority );
            NVIC_EnableIRQ( DMA1_Channel1_IRQn );
        }
        else if ( channel <= 3 ) {
            NVIC_SetPriority( DMA1_Channel2_3_IRQn, priority );
            NVIC_EnableIRQ( DMA1_Channel2_3_IRQn );
        }
        else if ( channel <= 7 ) {
            NVIC_SetPriority( DMA1_Ch4_7_DMAMUX1_OVR_IRQn, priority );
            NVIC_EnableIRQ( DMA1_Ch4_7_DMAMUX1_OVR_IRQn );
        } else {
            assert( false && "Invalid channel" );
        }
    }
};

} // namespace detail