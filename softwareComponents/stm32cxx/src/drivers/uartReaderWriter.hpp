#pragma once

#include <drivers/uart.hpp>
#include <drivers/dma.hpp>
#include <system/ringBuffer.hpp>


template < typename AllocatorT, typename UartD = Uart > // The template argument allows for constexpr if
class UartReader {
public:
    using Allocator = AllocatorT;
    using Mem = typename Allocator::Block;

    UartReader( UartD& uart, Dma::Channel channel ) : _uart( uart ) {
        _channel = std::move( channel );
        assert( _channel );

        if constexpr ( Dma::supportsMuxing ) {
            LL_DMA_SetPeriphRequest( _channel, _channel, LL_DMAMUX_REQ_RX( _uart.periph() ) );
        }
        else {
            _channel.setTrigger( _uart.getDmaRxTrigger( _channel._periph, _channel._channel ) );
        }

        LL_DMA_SetDataTransferDirection( _channel, _channel, LL_DMA_DIRECTION_PERIPH_TO_MEMORY );
        _channel.setPriority( LL_DMA_PRIORITY_LOW );
        LL_DMA_SetMode( _channel, _channel, LL_DMA_MODE_NORMAL );
        LL_DMA_SetPeriphIncMode( _channel, _channel, LL_DMA_PERIPH_NOINCREMENT );
        LL_DMA_SetMemoryIncMode( _channel, _channel, LL_DMA_MEMORY_INCREMENT );
        LL_DMA_SetPeriphSize( _channel, _channel, LL_DMA_PDATAALIGN_BYTE);
        LL_DMA_SetMemorySize( _channel, _channel, LL_DMA_MDATAALIGN_BYTE);

        LL_DMA_SetPeriphAddress( _channel, _channel, _uart.getDmaTxAddr() );
        LL_USART_EnableDMAReq_RX( _uart.periph() );

        _channel.enableInterrupt();
    }

    template < typename Callback >
    void readBlock( Mem block, int offset, int size, int bitTimeout, Callback callback ) {
        _block = std::move( block );
        LL_DMA_SetMemoryAddress( _channel, _channel, uint32_t( _block.get() + offset ) );
        LL_DMA_SetDataLength( _channel, _channel, size );

        if constexpr ( UartD::supportsTimeout ) {
            if ( bitTimeout == 0 )
                _uart.disableTimout();
            else {
                _uart.enableTimeout( bitTimeout, [&, callback, size]() {
                    _channel.disable();
                    int read = size - LL_DMA_GetDataLength( _channel, _channel );
                    callback( std::move( _block ), read );
                } );
            }
        } else {
            assert( bitTimeout == 0 && "UART does not support bit timeout" );
        }

        _channel.onComplete( [&, callback, size]() {
            _channel.disable();
            int read = size - LL_DMA_GetDataLength( _channel, _channel );
            callback( std::move( _block ), read );
        } );

        _channel.enable();
    }

    void startBufferedReading( int bufferSize = 256 ) {
        _buffer = RingBuffer< char, Allocator >(
            Allocator::allocate( bufferSize ), bufferSize );
        _updateBuffer( 0 );
    }

    bool available() const {
        return !_buffer.empty();
    }

    int size() const {
        return _buffer.size();
    }

    char get() {
        return _buffer.pop_front();
    }

private:
    void _updateBuffer( int read ) {
        _buffer.advance( read );
        auto [ location, size ] = _buffer.insertPosition();
        int requested = std::min( size, 32 );
        LL_DMA_SetMemoryAddress( _channel, _channel, uint32_t( location ) );
        LL_DMA_SetDataLength( _channel, _channel, requested );
        if constexpr ( UartD::supportsTimeout ) {
            _uart.enableTimeout( 32, [&, requested]{
                _channel.disable();
                int read = requested - LL_DMA_GetDataLength( _channel, _channel );
                _updateBuffer( read );
            } );
        }

        _channel.onComplete( [&, requested]() {
            _channel.disable();
            int read = requested - LL_DMA_GetDataLength( _channel, _channel );
            _updateBuffer( read );
        } );

        _channel.enable();
    }

    UartD& _uart;
    Dma::Channel _channel;
    RingBuffer< char, Allocator > _buffer;
    Mem _block;
};

template < typename Reader >
class LineReader {
public:
    using Allocator = typename Reader::Allocator;
    using Mem = typename Allocator::Block;

    LineReader( Reader& reader, int lineBufferSize = 256 ) :
        _reader( reader ),
        _pos( 0 ),
        _available( false ),
        _lineBufferSize( lineBufferSize )
    { }

    bool available() {
        _readLine();
        return _available;
    }

    Mem get() {
        assert( _available );
        auto ret = std::move( _line );
        _available = false;
        return ret;
    }

private:
    bool _readLine() {
        if ( _available )
            return true;
        if ( !_line ) {
            _line = Allocator::allocate( _lineBufferSize );
            assert( _line );
            _pos = 0;
        }
        while ( _reader.available() ) {
            char chr = _reader.get();
            if ( chr == '\n' ) {
                _available = true;
                _line[ _pos ] = '\0';
                break;
            }
            if ( _pos < _lineBufferSize - 1 ) {
                _line[ _pos ] = chr;
                _pos++;
            }
        }
        return _available;
    }

    Reader& _reader;
    Mem _line;
    int _pos;
    bool _available;
    int _lineBufferSize;
};

template < typename AllocatorT, typename UartD = Uart > // The template argument allows for constexpr if
class UartWriter {
public:
    using Allocator = AllocatorT;
    using Mem = typename Allocator::Block;

    UartWriter( UartD& uart, Dma::Channel channel ) : _uart( uart ) {
        _channel = std::move( channel );
        assert( _channel );

        if constexpr ( Dma::supportsMuxing ) {
            LL_DMA_SetPeriphRequest( _channel, _channel, LL_DMAMUX_REQ_TX( _uart.periph() ) );
        }
        else {
            _channel.setTrigger( _uart.getDmaTxTrigger( _channel._periph, _channel._channel ) );
        }
        LL_DMA_SetDataTransferDirection( _channel, _channel, LL_DMA_DIRECTION_MEMORY_TO_PERIPH );
        _channel.setPriority( LL_DMA_PRIORITY_LOW );
        LL_DMA_SetMode( _channel, _channel, LL_DMA_MODE_NORMAL );
        LL_DMA_SetPeriphIncMode( _channel, _channel, LL_DMA_PERIPH_NOINCREMENT );
        LL_DMA_SetMemoryIncMode( _channel, _channel, LL_DMA_MEMORY_INCREMENT );
        LL_DMA_SetPeriphSize( _channel, _channel, LL_DMA_PDATAALIGN_BYTE );
        LL_DMA_SetMemorySize( _channel, _channel, LL_DMA_MDATAALIGN_BYTE );

        LL_DMA_SetPeriphAddress( _channel, _channel, _uart.getDmaTxAddr() );
        LL_USART_EnableDMAReq_TX( _uart.periph() );

        _channel.enableInterrupt();
    }

    template < typename Callback >
    void writeBlock( Mem block, int offset, int size, Callback callback ) {
        _block = std::move( block );
        LL_DMA_SetMemoryAddress( _channel, _channel, uint32_t( _block.get() + offset ) );
        LL_DMA_SetDataLength( _channel, _channel, size );

        _channel.onComplete( [&, callback, size]() {
            _channel.disable();
            int written = size - LL_DMA_GetDataLength( _channel, _channel );
            callback( std::move( _block ), written );
        } );

        _channel.enable();
    }

    void abort() {
        _channel.disable();
    }

private:
    UartD& _uart;
    Dma::Channel _channel;
    Mem _block;
};