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
        _channel.disable();

        if constexpr ( Dma::supportsMuxing ) {
            LL_DMA_SetPeriphRequest( _channel, _channel, LL_DMAMUX_REQ_RX( _uart.periph() ) );
        }
        else {
            _channel.setTrigger( _uart.getDmaRxTrigger( _channel._periph, _channel._channel ) );
        }

        LL_DMA_SetDataTransferDirection( _channel, _channel, LL_DMA_DIRECTION_PERIPH_TO_MEMORY );
        _channel.setPriority( LL_DMA_PRIORITY_LOW );
        LL_DMA_SetPeriphIncMode( _channel, _channel, LL_DMA_PERIPH_NOINCREMENT );
        LL_DMA_SetMemoryIncMode( _channel, _channel, LL_DMA_MEMORY_INCREMENT );
        LL_DMA_SetPeriphSize( _channel, _channel, LL_DMA_PDATAALIGN_BYTE );
        LL_DMA_SetMemorySize( _channel, _channel, LL_DMA_MDATAALIGN_BYTE );

        LL_DMA_SetPeriphAddress( _channel, _channel, _uart.getDmaRxAddr() );
        LL_USART_EnableDMAReq_RX( _uart.periph() );

        _channel.enableInterrupt();
    }

    template < typename Callback >
    void readBlock( Mem block, int offset, int size, int bitTimeout, Callback callback ) {
        assert( !_channel.isEnabled() );
        _block = std::move( block );
        LL_DMA_SetMode( _channel, _channel, LL_DMA_MODE_NORMAL );
        LL_DMA_SetMemoryAddress( _channel, _channel, uint32_t( _block.get() + offset ) );
        LL_DMA_SetDataLength( _channel, _channel, size );

        if constexpr ( UartD::supportsTimeout ) {
            if ( bitTimeout == 0 )
                _uart.disableTimout();
            else {
                _uart.enableTimeout( bitTimeout, [this, callback, size]() {
                    _channel.disable();
                    int read = size - LL_DMA_GetDataLength( _channel, _channel );
                    callback( std::move( _block ), read );
                } );
            }
        }
        else if constexpr ( UartD::supportsIdle ) {
            assert( bitTimeout == 8 && "UART only supports idle detection" );

            _uart.enableTimeout( [this, callback, size]() {
                _channel.disable();
                int read = size - LL_DMA_GetDataLength( _channel, _channel );
                callback( std::move( _block ), read );
            } );
        }
        else {
            assert( bitTimeout == 0 && "UART does not support bit timeout" );
        }

        _channel.onComplete( [this, callback, size]() {
            _channel.disable();
            int read = size - LL_DMA_GetDataLength( _channel, _channel );
            callback( std::move( _block ), read );
        } );

        _channel.enable();
    }

    void startBufferedReading( int bufferSize = 256 ) {
        assert( !_channel.isEnabled() );
        _buffer = RingBuffer< char, Allocator >(
            Allocator::allocate( bufferSize ), bufferSize );
        _updateBuffer( 0 );
    }

    void stopBufferedReading() {
        _channel.disable();
        _buffer = {};
    }

    bool available() const {
        assert( _buffer.capacity() != 0 );
        return !_buffer.empty();
    }

    int size() const {
        assert( _buffer.capacity() != 0 );
        return _buffer.size();
    }

    char get() {
        assert( _buffer.capacity() != 0 );
        return _buffer.pop_front();
    }

    template < typename F >
    void startCircularReading( int bufferSize, int bitTimeout, F handleNewData ) {
        assert( !_channel.isEnabled() );

        _block = Allocator::allocate( bufferSize );
        assert( _block );

        LL_DMA_SetMode( _channel, _channel, LL_DMA_MODE_CIRCULAR );
        LL_DMA_SetMemoryAddress( _channel, _channel, uint32_t( _block.get() ) );
        LL_DMA_SetDataLength( _channel, _channel, bufferSize );

        _lastPos = 0;
        _newDataHandler = handleNewData;
        _bufferSize = bufferSize;
        _channel.onHalf( [this]() {
            _notifyCircular();
        } );
        _channel.onComplete( [this]() {
            _notifyCircular();
        } );
        _onTimeout( bitTimeout, [this]() {
            _notifyCircular();
        } );

        _channel.enable();
    }

    void startCircularReading( int bufferSize, int bitTimeout ) {
        assert( !_channel.isEnabled() );

        _block = Allocator::allocate( bufferSize );
        assert( _block );

        LL_DMA_SetMode( _channel, _channel, LL_DMA_MODE_CIRCULAR );
        LL_DMA_SetMemoryAddress( _channel, _channel, uint32_t( _block.get() ) );
        LL_DMA_SetDataLength( _channel, _channel, bufferSize );

        _lastPos = 0;
        _bufferSize = bufferSize;
        _channel.onHalf( []{} );
        _channel.onComplete( []{} );
        _onTimeout( bitTimeout, []{} );

        _channel.enable();
    }

    std::pair< const unsigned char *, int > getAvailableCircular() {
        int end = _bufferSize - LL_DMA_GetDataLength( _channel, _channel );
        int begin = _lastPos;
        if ( _lastPos > end ) { // We moved pass the buffer boundary
            int size = _bufferSize - _lastPos;
            _lastPos = 0;
            return { &_block[ begin ], size };
        }
        int size = end - _lastPos;
        _lastPos = end;
        return { &_block[ begin ], size };
    }

    void stopCircularReading() {
        _channel.disable();
    }

private:
    void _updateBuffer( int read ) {
        _buffer.advanceWrite( read );
        auto [ location, size ] = _buffer.insertPosition();
        int requested = std::min( size, 32 );
        LL_DMA_SetMode( _channel, _channel, LL_DMA_MODE_NORMAL );
        LL_DMA_SetMemoryAddress( _channel, _channel, uint32_t( location ) );
        LL_DMA_SetDataLength( _channel, _channel, requested );
        if constexpr ( UartD::supportsTimeout ) {
            _uart.enableTimeout( 32, [this, requested]{
                _channel.disable();
                int read = requested - LL_DMA_GetDataLength( _channel, _channel );
                _updateBuffer( read );
            } );
        }

        _channel.onComplete( [this, requested]() {
            _channel.disable();
            int read = requested - LL_DMA_GetDataLength( _channel, _channel );
            _updateBuffer( read );
        } );

        _channel.enable();
    }

    template < typename F >
    void _onTimeout( int bitTimeout, F f ) {
        if ( bitTimeout == 0 ) {
            _uart.disableTimeout();
            return;
        }
        if constexpr ( UartD::supportsTimeout ) {
            _uart.enableTimeout( bitTimeout, f );
        }
        else if constexpr ( UartD::supportsIdle ) {
            assert( bitTimeout == 8 && "UART only supports idle detection" );
            _uart.enableTimeout( f );
        }
        else {
            assert( bitTimeout == 0 && "UART does not support bit timeout" );
        }
    }

    void _notifyCircular() {
        int end = _bufferSize - LL_DMA_GetDataLength( _channel, _channel );
        if ( _lastPos > end ) { // We moved pass the buffer boundary
            int size = _bufferSize - _lastPos;
            _newDataHandler( &_block[ _lastPos ], size );
            _lastPos = 0;
        }
        int size = end - _lastPos;
        _newDataHandler( &_block[ _lastPos ], size );
        _lastPos = end;
    }

    UartD& _uart;
    Dma::Channel _channel;
    RingBuffer< char, Allocator > _buffer;
    Mem _block;
    int _lastPos, _bufferSize;
    std::function< void( const uint8_t*, int size ) > _newDataHandler;
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
        assert( !_channel.isEnabled() );
        _block = std::move( block );
        LL_DMA_SetMemoryAddress( _channel, _channel, uint32_t( _block.get() + offset ) );
        LL_DMA_SetDataLength( _channel, _channel, size );

        _channel.onComplete( [this, callback, size]() {
            _channel.disable();
            int written = size - LL_DMA_GetDataLength( _channel, _channel );
            callback( std::move( _block ), written );
        } );

        _channel.enable();
    }

    template < typename Callback >
    void write( const uint8_t* mem, int size, Callback callback ) {
        assert( !_channel.isEnabled() );
        LL_DMA_SetMemoryAddress( _channel, _channel, uint32_t( mem ) );
        LL_DMA_SetDataLength( _channel, _channel, size );

        _channel.onComplete( [this, callback]() {
            _channel.disable();
            callback();
        } );

        _channel.enable();
    }

    void abort() {
        _channel.disable();
    }

    bool busy() {
        return _channel.isEnabled();
    }

private:
    UartD& _uart;
    Dma::Channel _channel;
    Mem _block;
};
