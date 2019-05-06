#pragma once

#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_usart.h>

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

struct Uart: public Peripheral< USART_TypeDef > {
public:
    template < typename... Configs >
    Uart( USART_TypeDef *periph = nullptr, Configs... configs )
        : Peripheral< USART_TypeDef >( periph )
    {
        enableClock();

        LL_USART_InitTypeDef config;
        LL_USART_StructInit( &config );
        ( configs.pre( config ), ... );
        LL_USART_Init( periph, &config );

        LL_USART_SetTXFIFOThreshold( _periph, LL_USART_FIFOTHRESHOLD_1_8 );
        LL_USART_SetRXFIFOThreshold( _periph, LL_USART_FIFOTHRESHOLD_1_8 );
        LL_USART_EnableFIFO( _periph );
        LL_USART_ConfigAsyncMode( _periph );

        ( configs.post( _periph ), ... );

        _enableInterrupt( 0 );
    }

    using Handler = std::function< void() >;

    void enableClock() {
        if ( _periph == USART1 )
            LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_USART1 );
        else if ( _periph == USART2 )
            LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_USART2 );
        else if ( _periph == USART3 )
            LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_USART3 );
        else if ( _periph == USART4 )
            LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_USART4 );
        else
            assert( false && "Unknown USART instance" );
    }

    void disableClock() {
        if ( _periph == USART1 )
            LL_APB2_GRP1_DisableClock( LL_APB2_GRP1_PERIPH_USART1 );
        else if ( _periph == USART2 )
            LL_APB1_GRP1_DisableClock( LL_APB1_GRP1_PERIPH_USART2 );
        else if ( _periph == USART3 )
            LL_APB1_GRP1_DisableClock( LL_APB1_GRP1_PERIPH_USART3 );
        else if ( _periph == USART4 )
            LL_APB1_GRP1_DisableClock( LL_APB1_GRP1_PERIPH_USART4 );
        else
            assert( false && "Unknown USART instance" );
    }

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
    void _enableInterrupt( int priority = 0 ) {
        if ( _periph == USART1 ) {
            NVIC_SetPriority( USART1_IRQn, priority );
            NVIC_EnableIRQ( USART1_IRQn );
        }
        else if ( _periph == USART2 ) {
            NVIC_SetPriority( USART2_IRQn, priority );
            NVIC_EnableIRQ( USART2_IRQn );
        }
        else if ( _periph == USART3 || _periph == USART4 ) {
            NVIC_SetPriority( USART3_4_LPUART1_IRQn, priority );
            NVIC_EnableIRQ( USART3_4_LPUART1_IRQn );
        }
    }

    struct Handlers {
        Handler rxTimeout;

        void _handleIsr( USART_TypeDef *uart ) {
            HANDLE_WITH( RTO, rxTimeout );
        }
    };

    static Handlers& handlers( USART_TypeDef *u ) {
        if ( u == USART1 )
            return _uarts[ 0 ];
        if ( u == USART2 )
            return _uarts[ 1 ];
        if ( u == USART3 )
            return _uarts[ 2 ];
        if ( u == USART3 )
            return _uarts[ 3 ];
        assert( false && "Invalid USART specified" );
    }

    Handlers& handlers() {
        return handlers( _periph );
    }
    static std::array< Handlers, 4 > _uarts;

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

struct TxOn: public UartConfigBase {
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

    int alternativeFun( USART_TypeDef *periph ) {
        if ( periph == USART1 ) {
            if ( _pin._pos == 6 )
                return LL_GPIO_AF_0;
            assert( false && "Incorrect TX pin" );
        }
        // ToDo: More configurations
        assert( false && "Incorrect TX pin" );
    }

    Gpio::Pin _pin;
};

struct RxOn: public UartConfigBase {
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

    int alternativeFun( USART_TypeDef *periph ) {
        if ( periph == USART1 ) {
            if ( _pin._pos == 7 )
                return LL_GPIO_AF_0;
            assert( false && "Incorrect TX pin" );
        }
        // ToDo: More configurations
        assert( false && "Incorrect TX pin" );
    }

    Gpio::Pin _pin;
};

inline int LL_DMAMUX_REQ_RX( USART_TypeDef *uart ) {
    if ( uart == USART1 )
        return LL_DMAMUX_REQ_USART1_RX;
    if ( uart == USART2 )
        return LL_DMAMUX_REQ_USART2_RX;
    if ( uart == USART3 )
        return LL_DMAMUX_REQ_USART3_RX;
    if ( uart == USART4 )
        return LL_DMAMUX_REQ_USART4_RX;
    assert( false && "Invalid USART specified" );
}

inline int LL_DMAMUX_REQ_TX( USART_TypeDef *uart ) {
    if ( uart == USART1 )
        return LL_DMAMUX_REQ_USART1_TX;
    if ( uart == USART2 )
        return LL_DMAMUX_REQ_USART2_TX;
    if ( uart == USART3 )
        return LL_DMAMUX_REQ_USART3_TX;
    if ( uart == USART4 )
        return LL_DMAMUX_REQ_USART4_TX;
    assert( false && "Invalid USART specified" );
}

class Reader {
public:
    using Mem = memory::Pool::Block;

    Reader( Uart& uart, int dmaChannel = 0 ) : _uart( uart ) {
        _channel = Dma::allocate( dmaChannel );
        assert( _channel );

        LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_DMA1 );

        LL_DMA_SetPeriphRequest( DMA1, _channel, LL_DMAMUX_REQ_RX( _uart.periph() ) );
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

        _uart.enableTimeout( bitTimeout, [&, callback, size]() {
            LL_DMA_DisableChannel( DMA1, _channel );
            int read = size - LL_DMA_GetDataLength( DMA1, _channel );
            callback( std::move( _block ), read );
        } );

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

class Writer {
public:
    using Mem = memory::Pool::Block;

    Writer( Uart& uart, int dmaChannel = 0 ) : _uart( uart ) {
        _channel = Dma::allocate( dmaChannel );
        assert( _channel );

        LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_DMA1 );

        LL_DMA_SetPeriphRequest( DMA1, _channel, LL_DMAMUX_REQ_TX( _uart.periph() ) );
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
            LL_DMA_DisableChannel( DMA1, _channel );;
            callback( std::move( _block ) );
        } );

        LL_DMA_EnableChannel( DMA1, _channel );
    }

private:
    Uart& _uart;
    Dma::Channel _channel;
    Mem _block;
};

#undef HANDLE_WITH