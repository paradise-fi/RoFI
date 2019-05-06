#pragma once

#include <functional>

#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_gpio.h>
#include <stm32g0xx_ll_exti.h>

#include <drivers/peripheral.hpp>


struct Gpio: public Peripheral< GPIO_TypeDef > {
    Gpio( GPIO_TypeDef *_periph ) : Peripheral< GPIO_TypeDef >( _periph ) {}

    struct Pin {
        Gpio port() const {
            return Gpio( _periph );
        }

        void setupPPOutput() {
            port().enableClock();
            LL_GPIO_InitTypeDef cfg{};
            cfg.Pin = 1 << _pos;
            cfg.Mode = LL_GPIO_MODE_OUTPUT;
            cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            cfg.Pull = LL_GPIO_PULL_NO;
            LL_GPIO_Init( _periph, &cfg );
        }

        void setupInput( bool pull, bool pull_up = true ) {
            port().enableClock();
            LL_GPIO_InitTypeDef cfg{};
            cfg.Pin = 1 << _pos;
            cfg.Mode = LL_GPIO_MODE_INPUT;
            cfg.Pull = !pull ?
                LL_GPIO_PULL_NO :
                pull_up ? LL_GPIO_PULL_UP : LL_GPIO_PULL_DOWN;
            LL_GPIO_Init( _periph, &cfg );
        }

        void setupAnalog( bool pull, bool pull_up = true ) {
            port().enableClock();
            LL_GPIO_InitTypeDef cfg{};
            cfg.Pin = 1 << _pos;
            cfg.Mode = LL_GPIO_MODE_ANALOG;
            cfg.Pull = !pull ?
                LL_GPIO_PULL_NO :
                pull_up ? LL_GPIO_PULL_UP : LL_GPIO_PULL_DOWN;
            LL_GPIO_Init( _periph, &cfg );
        }

        template < typename Callback >
        void setupInterrupt( int edge, Callback c ) {
            port().enableClock();

            if ( _periph == GPIOA )
                LL_EXTI_SetEXTISource( LL_EXTI_CONFIG_PORTA, _extiConfigLine( _pos ) );
            else
                LL_EXTI_SetEXTISource( LL_EXTI_CONFIG_PORTB, _extiConfigLine( _pos ) );

            LL_EXTI_InitTypeDef EXTI_InitStruct{};
            EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_4;
            EXTI_InitStruct.LineCommand = ENABLE;
            EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
            EXTI_InitStruct.Trigger = edge;
            LL_EXTI_Init( &EXTI_InitStruct );

            LL_GPIO_SetPinPull( _periph, 1 << _pos, LL_GPIO_PULL_NO );
            LL_GPIO_SetPinMode( _periph, 1 << _pos, LL_GPIO_MODE_INPUT);

            Gpio::_lines[ _pos ] = c;
            IRQn_Type irq =
                  _pos == 1 ? EXTI0_1_IRQn
                : _pos <= 3 ? EXTI2_3_IRQn
                : EXTI4_15_IRQn;
            NVIC_SetPriority( irq, 2 );
            NVIC_EnableIRQ( irq );
        }

        bool read() {
            return LL_GPIO_IsInputPinSet( _periph, 1 << _pos );
        }

        void write( bool state ) {
            if ( state )
                LL_GPIO_SetOutputPin( _periph, 1 << _pos );
            else
                LL_GPIO_ResetOutputPin( _periph, 1 << _pos );
        }

        int _pos;
        GPIO_TypeDef *_periph;
    };

    void enableClock() {
        if ( _periph == GPIOA )
            LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOA );
        else if ( _periph == GPIOB )
            LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOB );
    }

    void disableClock() {
        if ( _periph == GPIOA )
            LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOA );
        else if ( _periph == GPIOB )
            LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOB );
    }

    Pin operator[]( int pin ) {
        return { pin, _periph };
    }

    static int _extiConfigLine( int pos ) {
        static const int table[ 16 ] = {
            LL_EXTI_CONFIG_LINE1,
            LL_EXTI_CONFIG_LINE2,
            LL_EXTI_CONFIG_LINE3,
            LL_EXTI_CONFIG_LINE4,
            LL_EXTI_CONFIG_LINE5,
            LL_EXTI_CONFIG_LINE6,
            LL_EXTI_CONFIG_LINE7,
            LL_EXTI_CONFIG_LINE8,
            LL_EXTI_CONFIG_LINE9,
            LL_EXTI_CONFIG_LINE10,
            LL_EXTI_CONFIG_LINE11,
            LL_EXTI_CONFIG_LINE12,
            LL_EXTI_CONFIG_LINE13,
            LL_EXTI_CONFIG_LINE14,
            LL_EXTI_CONFIG_LINE15
        };
        return table[ pos ];
    }

    static void _handleIrq( int line ) {
        int l = 1 << line;
        if ( !LL_EXTI_IsEnabledIT_0_31( l ) )
            return;
        if ( LL_EXTI_IsActiveFallingFlag_0_31( l ) ) {
            LL_EXTI_ClearFallingFlag_0_31( l );
            Gpio::_lines[ line ]( false );
        }
        if ( LL_EXTI_IsActiveRisingFlag_0_31( l ) ) {
            LL_EXTI_ClearRisingFlag_0_31( l );
            Gpio::_lines[ line ]( true );
        }
    }

    using Handler = std::function< void( bool /* rising */ ) >;
    static Handler _lines[ 16 ];
};


extern Gpio GpioA;
extern Gpio GpioB;