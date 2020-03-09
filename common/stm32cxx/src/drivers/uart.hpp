#pragma once

#if defined(STM32G0xx)
    #include <drivers/stm32g0xx/uart.hpp>
#elif defined(STM32F0xx)
    #include <drivers/stm32f0xx/uart.hpp>
#else
    #error "Unsuported MCU family"
#endif

#include <algorithm>
#include <array>
#include <cassert>
#include <functional>

#include <drivers/peripheral.hpp>
#include <drivers/dma.hpp>
#include <drivers/gpio.hpp>
#include <system/ringBuffer.hpp>

extern "C" void USART1_IRQHandler();
extern "C" void USART2_IRQHandler();
extern "C" void USART3_4_LPUART1_IRQHandler();

#define HANDLE_WITH( FLAG, HANDLER )                 \
    if ( LL_USART_IsEnabledIT_ ## FLAG ( uart )      \
        && LL_USART_IsActiveFlag_ ## FLAG ( uart ) ) \
    {                                                \
        LL_USART_ClearFlag_ ## FLAG ( uart );        \
        HANDLER();                                   \
    }

struct Uart: public Peripheral< USART_TypeDef >, public detail::Uart< Uart > {
public:
    friend class detail::Uart< Uart >;

    template < typename... Configs >
    Uart( USART_TypeDef *periph = nullptr, Configs... configs )
        : Peripheral< USART_TypeDef >( periph )
    {
        enableClock();

        LL_USART_InitTypeDef config;
        LL_USART_StructInit( &config );
        ( configs.pre( config ), ... );
        LL_USART_Init( periph, &config );

        configureFifo();
        LL_USART_ConfigAsyncMode( _periph );

        ( configs.post( _periph ), ... );

        _enableInterrupt( 0 );
    }

    using Handler = std::function< void() >;

    void enable() {
        LL_USART_Enable( _periph );
        while( (!LL_USART_IsActiveFlag_TEACK( _periph) )
            || (!LL_USART_IsActiveFlag_REACK( _periph) ) );
    }

    void disable() {
        LL_USART_Disable( _periph );
    }

    USART_TypeDef *periph() {
        return _periph;
    }

    void send( const char *str )
    {
        while( *str )
            send( *( str++ ) );
        while ( !LL_USART_IsActiveFlag_TC( _periph ) );
    }

    void send( const uint8_t *buff, int size ) {
        for ( int i = 0; i != size; i++ )
            send( buff[ i ] );
        while ( !LL_USART_IsActiveFlag_TC( _periph ) );
    }

    void send( const char *buff, int size ) {
        send( reinterpret_cast< const uint8_t * >( buff ), size );
    }

    void send( char c ) {
        LL_USART_TransmitData8( _periph, c );
        while ( !LL_USART_IsActiveFlag_TXE( _periph ) );
    }

    void enableTimeout() {
        LL_USART_EnableRxTimeout( _periph );
        LL_USART_EnableIT_RTO( _periph );
    }

    template < typename Callback >
    void enableTimeout( int bitDuration, Callback callback ) {
        LL_USART_SetRxTimeout( _periph, bitDuration );
        handlers().rxTimeout = Handler( callback );
        enableTimeout();
    }

    void disableTimout() {
        LL_USART_DisableRxTimeout( _periph );
        LL_USART_DisableIT_RTO( _periph );
    }

protected:
    struct Handlers {
        Handler rxTimeout;

        void _handleIsr( USART_TypeDef *uart ) {
            HANDLE_WITH( RTO, rxTimeout );
        }
    };

    using detail::Uart< Uart >::handlers;

    Handlers& handlers() {
        return this->handlers( _periph );
    }

    static std::array< Handlers, handlerCount > _uarts;


    friend void USART1_IRQHandler();
    friend void USART2_IRQHandler();
    friend void USART3_4_LPUART1_IRQHandler();
};

struct UartConfigBase {
    void pre( LL_USART_InitTypeDef& ) {}
    void post( USART_TypeDef * ) {}
};

struct Baudrate: public UartConfigBase {
    Baudrate( int b ) : _baudrate( b ) {}
    void pre( LL_USART_InitTypeDef& init ) {
        init.BaudRate = _baudrate;
    }
    int _baudrate;
};

struct TxOn: public UartConfigBase, public detail::TxOn< TxOn > {
    TxOn( Gpio::Pin p ) : _pin( p ) {}
    void post( USART_TypeDef *periph ) {

        _pin.port().enableClock();

        LL_GPIO_InitTypeDef GPIO_InitStruct{};
        GPIO_InitStruct.Pin = 1 << _pin._pos;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
        GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
        GPIO_InitStruct.Alternate = alternativeFun( periph );

        LL_GPIO_Init( _pin._periph, &GPIO_InitStruct );
    }

    Gpio::Pin _pin;
};

struct RxOn: public UartConfigBase, public detail::RxOn< RxOn > {
    RxOn( Gpio::Pin p ) : _pin( p ) {}
    void post( USART_TypeDef *periph ) {

        _pin.port().enableClock();

        LL_GPIO_InitTypeDef GPIO_InitStruct{};
        GPIO_InitStruct.Pin = 1 << _pin._pos;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
        GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
        GPIO_InitStruct.Alternate = alternativeFun( periph );

        LL_GPIO_Init( _pin._periph, &GPIO_InitStruct );
    }

    Gpio::Pin _pin;
};

class UartReader {
public:
    using Mem = memory::Pool::Block;

    UartReader( Uart& uart, int dmaChannel = 0 ) : _uart( uart ) {
        _channel = Dma::allocate( dmaChannel );
        assert( _channel );

        LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_DMA1 );

        // If DMA supports muxing
        #if (defined(DMA1_CSELR_DEFAULT)||defined(DMA2_CSELR_DEFAULT))
            LL_DMA_SetPeriphRequest( DMA1, _channel, LL_DMAMUX_REQ_RX( _uart.periph() ) );
        #endif
        LL_DMA_SetDataTransferDirection( DMA1, _channel, LL_DMA_DIRECTION_PERIPH_TO_MEMORY );
        LL_DMA_SetChannelPriorityLevel( DMA1, _channel, LL_DMA_PRIORITY_LOW );
        LL_DMA_SetMode( DMA1, _channel, LL_DMA_MODE_NORMAL );
        LL_DMA_SetPeriphIncMode( DMA1, _channel, LL_DMA_PERIPH_NOINCREMENT );
        LL_DMA_SetMemoryIncMode( DMA1, _channel, LL_DMA_MEMORY_INCREMENT );
        LL_DMA_SetPeriphSize( DMA1, _channel, LL_DMA_PDATAALIGN_BYTE);
        LL_DMA_SetMemorySize( DMA1, _channel, LL_DMA_MDATAALIGN_BYTE);

        LL_DMA_SetPeriphAddress( DMA1, _channel,
            LL_USART_DMA_GetRegAddr( _uart.periph(), LL_USART_DMA_REG_DATA_RECEIVE ) );
        LL_USART_EnableDMAReq_RX( _uart.periph() );

        _channel.enableInterrupt();
    }

    template < typename Callback >
    void readBlock( Mem block, int offset, int size, int bitTimeout, Callback callback ) {
        _block = std::move( block );
        LL_DMA_SetMemoryAddress( DMA1, _channel, uint32_t( _block.get() + offset ) );
        LL_DMA_SetDataLength( DMA1, _channel, size );

        if ( bitTimeout == 0 )
            _uart.disableTimout();
        else {
            _uart.enableTimeout( bitTimeout, [&, callback, size]() {
                LL_DMA_DisableChannel( DMA1, _channel );
                int read = size - LL_DMA_GetDataLength( DMA1, _channel );
                callback( std::move( _block ), read );
            } );
        }

        _channel.onComplete( [&, callback, size]( int /* channel */ ) {
            LL_DMA_DisableChannel( DMA1, _channel );
            int read = size - LL_DMA_GetDataLength( DMA1, _channel );
            callback( std::move( _block ), read );
        } );

        LL_DMA_EnableChannel( DMA1, _channel );
    }

    void startBufferedReading( int bufferSize = 256 ) {
        _buffer = RingBuffer< char >( memory::Pool::allocate( bufferSize ), bufferSize );
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
        LL_DMA_SetMemoryAddress( DMA1, _channel, uint32_t( location ) );
        LL_DMA_SetDataLength( DMA1, _channel, requested );
        _uart.enableTimeout( 32, [&, requested]{
            LL_DMA_DisableChannel( DMA1, _channel );
            int read = requested - LL_DMA_GetDataLength( DMA1, _channel );
            _updateBuffer( read );
        } );

        _channel.onComplete( [&, requested]( int /* channel */ ) {
            LL_DMA_DisableChannel( DMA1, _channel );
            int read = requested - LL_DMA_GetDataLength( DMA1, _channel );
            _updateBuffer( read );
        } );

        LL_DMA_EnableChannel( DMA1, _channel );
    }

    Uart& _uart;
    Dma::Channel _channel;
    RingBuffer< char > _buffer;
    Mem _block;
};

template < typename Reader >
class LineReader {
public:
    using Mem = memory::Pool::Block;

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
            _line = memory::Pool::allocate( _lineBufferSize );
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

class UartWriter {
public:
    using Mem = memory::Pool::Block;

    UartWriter( Uart& uart, int dmaChannel = 0 ) : _uart( uart ) {
        _channel = Dma::allocate( dmaChannel );
        assert( _channel );

        LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_DMA1 );

        // If DMA supports muxing
        #if (defined(DMA1_CSELR_DEFAULT)||defined(DMA2_CSELR_DEFAULT))
            LL_DMA_SetPeriphRequest( DMA1, _channel, LL_DMAMUX_REQ_TX( _uart.periph() ) );
        #endif
        LL_DMA_SetDataTransferDirection( DMA1, _channel, LL_DMA_DIRECTION_MEMORY_TO_PERIPH );
        LL_DMA_SetChannelPriorityLevel( DMA1, _channel, LL_DMA_PRIORITY_LOW );
        LL_DMA_SetMode( DMA1, _channel, LL_DMA_MODE_NORMAL );
        LL_DMA_SetPeriphIncMode( DMA1, _channel, LL_DMA_PERIPH_NOINCREMENT );
        LL_DMA_SetMemoryIncMode( DMA1, _channel, LL_DMA_MEMORY_INCREMENT );
        LL_DMA_SetPeriphSize( DMA1, _channel, LL_DMA_PDATAALIGN_BYTE );
        LL_DMA_SetMemorySize( DMA1, _channel, LL_DMA_MDATAALIGN_BYTE );

        LL_DMA_SetPeriphAddress( DMA1, _channel,
            LL_USART_DMA_GetRegAddr( _uart.periph(), LL_USART_DMA_REG_DATA_TRANSMIT ) );
        LL_USART_EnableDMAReq_TX( _uart.periph() );

        _channel.enableInterrupt();
    }

    template < typename Callback >
    void writeBlock( Mem block, int offset, int size, Callback callback ) {
        _block = std::move( block );
        LL_DMA_SetMemoryAddress( DMA1, _channel, uint32_t( _block.get() + offset ) );
        LL_DMA_SetDataLength( DMA1, _channel, size );

        _channel.onComplete( [&, callback, size]( int /* channel */ ) {
            LL_DMA_DisableChannel( DMA1, _channel );
            int written = size - LL_DMA_GetDataLength( DMA1, _channel );
            callback( std::move( _block ), written );
        } );

        LL_DMA_EnableChannel( DMA1, _channel );
    }

    void abort() {
        LL_DMA_DisableChannel( DMA1, _channel );
    }

private:
    Uart& _uart;
    Dma::Channel _channel;
    Mem _block;
};

#undef HANDLE_WITH