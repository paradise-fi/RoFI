#pragma once

#include <initializer_list>

#include <gpio.port.hpp>

#include <functional>
#include <drivers/peripheral.hpp>


struct Gpio: public Peripheral< GPIO_TypeDef >, public detail::Gpio< Gpio > {
    static const constexpr std::initializer_list< GPIO_TypeDef * > availablePeripherals = {
        #ifdef GPIOA
            GPIOA,
        #endif
        #ifdef GPIOB
            GPIOB,
        #endif
        #ifdef GPIOC
            GPIOC,
        #endif
        #ifdef GPIOD
            GPIOD,
        #endif
        #ifdef GPIOE
            GPIOE,
        #endif
        #ifdef GPIOF
            GPIOF,
        #endif
        #ifdef GPIOG
            GPIOG,
        #endif
        #ifdef GPIOH
            GPIOH,
        #endif
        #ifdef GPIOI
            GPIOI,
        #endif
    };

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

        Pin& setupAnalog( bool pull = false, bool pull_up = true ) {
            port().enableClock();
            LL_GPIO_InitTypeDef cfg{};
            cfg.Pin = 1 << _pos;
            cfg.Mode = LL_GPIO_MODE_ANALOG;
            cfg.Pull = !pull ?
                LL_GPIO_PULL_NO :
                pull_up ? LL_GPIO_PULL_UP : LL_GPIO_PULL_DOWN;
            LL_GPIO_Init( _periph, &cfg );
            LL_ADC_Enable( Gpio::_getAdcPeriph( _periph, _pos ) );
            return *this;
        }

        template < typename Callback >
        Pin& setupInterrupt( int edge, Callback c ) {
            port().enableClock();
            port().setExtiSource( _pos );

            LL_EXTI_InitTypeDef EXTI_InitStruct{};
            EXTI_InitStruct.Line_0_31 = 1 << _pos;
            EXTI_InitStruct.LineCommand = ENABLE;
            EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
            EXTI_InitStruct.Trigger = edge;
            LL_EXTI_Init( &EXTI_InitStruct );

            Gpio::_lines[ _pos ] = c;
            IRQn_Type irq = _positionToInterrupt( _pos );
            NVIC_SetPriority( irq, 1 );
            NVIC_EnableIRQ( irq );
            return *this;
        }

        Pin& disableInterrupt() {
            LL_EXTI_DisableIT_0_31( 1 << 1 << _pos );
            return *this;
        }

        Pin& invert() {
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

        uint32_t readAnalog() {
            assert( false );
            // TBA
            // auto adc = Gpio::_getAdcPeriph( _periph, _pos );
            // auto channel = Gpio::_getAdcChannel( _periph, _pos );
            // LL_ADC_REG_SetSequencerChannels( adc, channel );
            // LL_ADC_REG_StartConversion( adc );
            // while ( LL_ADC_REG_IsConversionOngoing( adc ) );
            // return LL_ADC_REG_ReadConversionData32( adc );
            __builtin_unreachable();
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

#ifdef GPIOC
    extern Gpio GpioC;
#endif