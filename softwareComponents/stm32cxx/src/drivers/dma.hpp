#pragma once

#include <dma.port.hpp>

#include <array>
#include <cassert>
#include <functional>

#include <system/util.hpp>

/**
 * Dma driver manages two aspects of DMA:
 * - allocation of the channels and
 * - the possibility to add arbitrary callbacks on DMA events.
 *
 * Managing of DMA sources and targets is up to the user or other drivers.
 *
 * As there are different naming conventions across the families, we define a
 * new naming that (hopefully) does not clash with the existing one.
 * We use:
 * - *peripheral* to denote DMA1 or DMA2
 * - *channel* to a part of DMA performing the transition (sometimes called stream)
 * - *trigger* to denote the triggering signal for DMA.
 */
class Dma: public detail::Dma< Dma > {
public:
    static const constexpr std::initializer_list< DMA_TypeDef * > availablePeripherals = {
        #ifdef DMA1
            DMA1,
        #endif
        #ifdef DMA2
            DMA2,
        #endif
    };

    class Channel: public detail::Dma< Dma >::Channel< Channel > {
        Channel( DMA_TypeDef *periph, int channel ):
            _periph( periph ),
            _channel( channel )
        {
            Dma::channelData( periph, channel )._available = false;
            Dma::_enableClock( periph );
        }

        friend class Dma;
    public:
        Channel(): _periph( nullptr ), _channel( 0 ) {}
        ~Channel() {
            if ( _periph ) {
                abort();
                Dma::channelData( _periph, _channel )._available = true;
            }
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

        operator DMA_TypeDef*() const {
            return _periph;
        }

        operator bool() const {
            return _periph != nullptr;
        }

        operator uint32_t() const {
            return _channel;
        }

        void swap( Channel& o ) {
            using std::swap;
            swap( _periph, o._periph );
            swap( _channel, o._channel );
        }

        template < typename Callback >
        void onComplete( Callback callback ) {
            Dma::channelData( _periph, _channel )._complete = Handler( callback );
            _enableOnComplete();
        }

        void disableOnComplete() {
            Dma::channelData( _periph, _channel )._complete = {};
            _disableOnComplete();
        }

        template < typename Callback >
        void onHalf( Callback callback ) {
            Dma::channelData( _periph, _channel )._half = Handler( callback );
            _enableOnHalf();
        }

        void disableOnHalf() {
            Dma::channelData( _periph, _channel )._half = {};
            _disableOnHalf();
        }

        template < typename Callback >
        void onError( Callback callback ) {
            Dma::channelData( _periph, _channel )._error = Handler( callback );
            _enableOnError();
        }

        void disableOnError() {
            Dma::channelData( _periph, _channel )._error = {};
            _disableOnError();
        }

        void abort() {
            if ( !isEnabled() )
                return;
            disable();
            ChannelData &handlers = Dma::channelData( _periph, _channel );
            if ( handlers._complete )
                handlers._complete();
        }

        DMA_TypeDef* _periph = nullptr;
        int _channel = 0;
    };

    static Channel allocate( DMA_TypeDef* periph, int chan = -1 ) {
        if ( chan != -1 ) {
            if ( channelData( periph, chan )._available )
                return Channel( periph, chan );
            return Channel();
        }

        assert( supportsMuxing && "This family does not support arbitrary allocation" );

        int periphIdx = indexOf( periph, availablePeripherals );
        assert( periphIdx >= 0 );
        for ( unsigned i = 0; i != _channelData[ periphIdx ].size(); i++ ) {
            if ( !_channelData[ periphIdx ][ i ]._available )
                continue;
            return Channel( periph, i + 1 );
        }
        return Channel();
    }

private:
    using Handler = std::function< void() >;
    struct ChannelData: public detail::Dma< Dma >::ChannelData< ChannelData > {
        Handler _half;
        Handler _complete;
        Handler _error;
        bool _available = true;
    };

    using ChannelsData = std::array< ChannelData, channelCount + 1 >; // We add + 1 as some families index from 0, some from 1
    static std::array< ChannelsData, availablePeripherals.size() > _channelData;

public:
    static ChannelData& channelData( DMA_TypeDef *periph, int channel ) {
        int periphIdx = indexOf( periph, availablePeripherals );
        assert( periphIdx >= 0 );
        assert( channel >= 0 && channel <= channelCount );

        return _channelData[ periphIdx ][ channel ];
    }
};