#pragma once

#if defined(STM32G0xx)
    #include <drivers/stm32g0xx/dma.hpp>
#elif defined(STM32F0xx)
    #include <drivers/stm32f0xx/dma.hpp>
#else
    #error "Unsuported MCU family"
#endif

#include <array>
#include <cassert>
#include <functional>

extern "C" void DMA1_Channel1_IRQHandler();
extern "C" void DMA1_Channel2_3_IRQHandler();
extern "C" void DMA1_Ch4_7_DMAMUX1_OVR_IRQHandler();

class Dma: public detail::Dma< Dma > {
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
            Dma::enableInterrupt( _channel, priority );
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

        void abort() {
            if ( !LL_DMA_IsEnabledChannel( DMA1, _channel ) )
                return;
            LL_DMA_DisableChannel( DMA1, _channel );
            auto &handlers = Dma::channel( _channel );
            if ( handlers._complete )
                handlers._complete( _channel );
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
        assert( channel >= 1 && channel < channelCount + 1 );
        return _channels[ channel - 1 ];
    }


    static std::array< Handlers, channelCount > _channels;

    friend void DMA1_Channel1_IRQHandler();
    friend void DMA1_Channel2_3_IRQHandler();
    friend void DMA1_Ch4_7_DMAMUX1_OVR_IRQHandler();
};