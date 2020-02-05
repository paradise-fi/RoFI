#pragma once

#if defined(STM32G0xx)
    #include <drivers/stm32g0xx/gpio.hpp>
#elif defined(STM32F0xx)
    #include <drivers/stm32f0xx/gpio.hpp>
#else
    #error "Unsuported MCU family"
#endif

#include <functional>
#include <drivers/peripheral.hpp>


struct Gpio: public Peripheral< GPIO_TypeDef >, public detail::Gpio< Gpio > {
    Gpio( GPIO_TypeDef *_periph ) : Peripheral< GPIO_TypeDef >( _periph ) {}

    struct Pin {
        Gpio port() const {
            return Gpio( _periph );
        }

        Pin& setupPPOutput() {
            port().enableClock();
            LL_GPIO_InitTypeDef cfg{};
            cfg.Pin = 1 << _pos;
            cfg.Mode = LL_GPIO_MODE_OUTPUT;
            cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            cfg.Pull = LL_GPIO_PULL_NO;
            LL_GPIO_Init( _periph, &cfg );
            return *this;
        }

        Pin& setupInput( bool pull, bool pull_up = true ) {
            port().enableClock();
            LL_GPIO_InitTypeDef cfg{};
            cfg.Pin = 1 << _pos;
            cfg.Mode = LL_GPIO_MODE_INPUT;
            cfg.Pull = !pull ?
                LL_GPIO_PULL_NO :
                pull_up ? LL_GPIO_PULL_UP : LL_GPIO_PULL_DOWN;
            LL_GPIO_Init( _periph, &cfg );
            return *this;
        }

        Pin& setupAnalog( bool pull, bool pull_up = true ) {
            port().enableClock();
            LL_GPIO_InitTypeDef cfg{};
            cfg.Pin = 1 << _pos;
            cfg.Mode = LL_GPIO_MODE_ANALOG;
            cfg.Pull = !pull ?
                LL_GPIO_PULL_NO :
                pull_up ? LL_GPIO_PULL_UP : LL_GPIO_PULL_DOWN;
            LL_GPIO_Init( _periph, &cfg );
            return *this;
        }

        template < typename Callback >
        Pin setupInterrupt( int edge, Callback c ) {
            port().enableClock();
            port().setExtiSource( _pos );

            LL_EXTI_InitTypeDef EXTI_InitStruct{};
            EXTI_InitStruct.Line_0_31 = 1 << _pos;
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
            NVIC_SetPriority( irq, 1 );
            NVIC_EnableIRQ( irq );
            return *this;
        }

        Pin invert() {
            _invert = true;
            return *this;
        }

        bool read() {
            if ( _invert )
                return !LL_GPIO_IsInputPinSet( _periph, 1 << _pos );
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
        bool _invert = false;
    };

    Pin operator[]( int pin ) {
        return { pin, _periph };
    }

    using Handler = std::function< void( bool /* rising */ ) >;
    static Handler _lines[ 16 ];
};


extern Gpio GpioA;
extern Gpio GpioB;