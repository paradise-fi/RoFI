#pragma once

#include <stm32g0xx_ll_dma.h>
#include <array>
#include <cassert>
#include <functional>

extern "C" void DMA1_Channel1_IRQHandler();
extern "C" void DMA1_Channel2_3_IRQHandler();
extern "C" void DMA1_Ch4_7_DMAMUX1_OVR_IRQHandler();

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

class Dma {
    GEN_FIRED( HT );
    GEN_FIRED( TC );
    GEN_FIRED( TE );

    GEN_CLEAR( HT );
    GEN_CLEAR( TE );
    GEN_CLEAR( TC );
public:
    using Handler = std::function< void( int /* channel */ ) >;

    class Channel {
        Channel( int channel ): _channel( channel ) {
            Dma::channel( channel )._available = false;
        }

        friend class Dma;
    public:
        Channel(): _channel( 0 ) {}
        ~Channel() {
            if ( _channel )
                Dma::channel( _channel )._available = true;
        }

        Channel( const Channel& ) = delete;
        Channel& operator=( const Channel& ) = delete;
        Channel( Channel&& o ) {
            swap( o );
        }
        Channel& operator=( Channel&& o ) {
            swap( o );
            return *this;
        }

        // Channel& channel() {
        //     return Dma::channel( _channel );
        // }

        operator bool() {
            return _channel != 0;
        }

        operator uint32_t() {
            return _channel;
        }

        void swap( Channel& o ) {
            using std::swap;
            swap( _channel, o._channel );
        }

        void enableInterrupt( int priority = 0 ) {
            if ( _channel == 1 ) {
                NVIC_SetPriority( DMA1_Channel1_IRQn, priority );
                NVIC_EnableIRQ( DMA1_Channel1_IRQn );
            }
            else if ( _channel <= 3 ) {
                NVIC_SetPriority( DMA1_Channel2_3_IRQn, priority );
                NVIC_EnableIRQ( DMA1_Channel2_3_IRQn );
            }
            else if ( _channel <= 7 ) {
                NVIC_SetPriority( DMA1_Ch4_7_DMAMUX1_OVR_IRQn, priority );
                NVIC_EnableIRQ( DMA1_Ch4_7_DMAMUX1_OVR_IRQn );
            } else {
                assert( false && "Invalid channel" );
            }
        }

        template < typename Callback >
        void onComplete( Callback callback ) {
            Dma::channel( _channel )._complete = Handler( callback );
            LL_DMA_EnableIT_TC( DMA1, _channel );
        }

        void disableOnComplete() {
            LL_DMA_DisableIT_TC( DMA1, _channel );
        }

        template < typename Callback >
        void onHalf( Callback callback ) {
            Dma::channel( _channel )._half = Handler( callback );
            LL_DMA_EnableIT_HT( DMA1, _channel );
        }

        void disableOnHalf() {
            LL_DMA_DisableIT_TC( DMA1, _channel );
        }

        template < typename Callback >
        void onError( Callback callback ) {
            Dma::channel( _channel )._error = Handler( callback );
            LL_DMA_EnableIT_TE( DMA1, _channel );
        }

        void disableOnError() {
            LL_DMA_DisableIT_TE( DMA1, _channel );
        }

        int _channel = 0;
    };

    static Channel allocate( int chan = 0 ) {
        if ( chan != 0 ) {
            if ( channel( chan )._available )
                return Channel( chan );
            return Channel();
        }
        for ( int i = 0; i != _channels.size(); i++ ) {
            if ( !_channels[ i ]._available )
                continue;
            return Channel( i + 1 );
        }
        return Channel();
    }
private:

    struct Handlers {
        void onHalf( Handler h ) { _half = h; }
        void onComplete( Handler h ) { _complete = h; }
        void onError( Handler h ) { _error = h; }

        void _handleIsr( int channel ) {
            if ( _firedHT( channel ) ) {
                _clearHT( channel );
                _half( channel );
            }

            if ( _firedTC( channel ) ) {
                _clearTC( channel );
                _complete( channel );
            }

            if ( _firedTE( channel ) ) {
                _clearTE( channel );
                _error( channel );
            }
        }

        Handler _half;
        Handler _complete;
        Handler _error;
        bool _available = true;
    };

    static Handlers& channel( int channel ) {
        assert( channel >= 1 && channel < 8 );
        return _channels[ channel - 1 ];
    }


    static std::array< Handlers, 7 > _channels;

    friend void DMA1_Channel1_IRQHandler();
    friend void DMA1_Channel2_3_IRQHandler();
    friend void DMA1_Ch4_7_DMAMUX1_OVR_IRQHandler();
};