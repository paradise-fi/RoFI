#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <initializer_list>

#include <drivers/peripheral.hpp>
#include <drivers/gpio.hpp>

#include <system/util.hpp>

#define HANDLE_WITH( FLAG, HANDLER )                 \
    if ( LL_USART_IsEnabledIT_ ## FLAG ( uart )      \
        && LL_USART_IsActiveFlag_ ## FLAG ( uart ) ) \
    {                                                \
        LL_USART_ClearFlag_ ## FLAG ( uart );        \
        HANDLER();                                   \
    }

#include <uart.port.hpp>

namespace detail {
    int uartAlternateFunTX( USART_TypeDef *periph, GPIO_TypeDef *port, int pos );
    int uartAlternateFunRX( USART_TypeDef *periph, GPIO_TypeDef *port, int pos );
    int uartAlternateFunCTS( USART_TypeDef *periph, GPIO_TypeDef *port, int pos );
    int uartAlternateFunRTS( USART_TypeDef *periph, GPIO_TypeDef *port, int pos );
    int uartAlternateFunDE( USART_TypeDef *periph, GPIO_TypeDef *port, int pos );
}; // namespace detail

/**
 * UART peripheral driver
 */
struct Uart: public Peripheral< USART_TypeDef >, public detail::Uart< Uart > {
public:
    friend class detail::Uart< Uart >;

    static const constexpr std::initializer_list< USART_TypeDef * > availablePeripherals = {
        #ifdef USART1
            USART1,
        #endif
        #ifdef USART2
            USART2,
        #endif
        #ifdef USART3
            USART3,
        #endif
        #ifdef USART4
            USART4,
        #endif
        #ifdef USART5
            USART5,
        #endif
        #ifdef USART6
            USART6,
        #endif
        #ifdef USART7
            USART7,
        #endif
        #ifdef USART8
            USART8,
        #endif
        #ifdef USART9
            USART9,
        #endif
        #ifdef USART10
            USART10,
        #endif
        #ifdef LPUART1
            LPUART1,
        #endif
        #ifdef LPUART2
            LPUART2,
        #endif
    };

    /**
     * Initialize UART driver. You can specify a number of configuration
     * modifiers, e.g., TxOn, RxOn, Baudrate, etc.
     */
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
        _waitForEnable();
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

protected:
    struct Handlers: public detail::Uart< Uart >::Handlers  {
        void _handleIsr( USART_TypeDef *uart ) {
            detail::Uart< Uart >::Handlers::_handleIsr( uart );
        }
    };

    static Handlers& handlers( USART_TypeDef* periph ) {
        return _uarts[ indexOf( periph, availablePeripherals ) ];
    }

    Handlers& handlers() {
        return _uarts[ indexOf( _periph, availablePeripherals ) ];
    }

    static std::array< Handlers, availablePeripherals.size() > _uarts;


    friend void USART1_IRQHandler();
    friend void USART2_IRQHandler();
    friend void USART6_IRQHandler();
    friend void USART3_4_LPUART1_IRQHandler();
};

/**
 * A base for UART configurators. All other configurators have to inherit from
 * this base and they should redefine the appropriate methods.
 */
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
        GPIO_InitStruct.Alternate =
            detail::uartAlternateFunTX( periph, _pin._periph, _pin._pos );

        LL_GPIO_Init( _pin._periph, &GPIO_InitStruct );
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
        GPIO_InitStruct.Alternate =
            detail::uartAlternateFunRX( periph, _pin._periph, _pin._pos );
    }

    Gpio::Pin _pin;
};

#undef HANDLE_WITH
