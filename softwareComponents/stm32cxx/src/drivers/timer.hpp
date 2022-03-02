#pragma once

#include <system/assert.hpp>
#include <utility>
#include <drivers/peripheral.hpp>
#include <drivers/gpio.hpp>

#include <timer.port.hpp>

class Timer: public Peripheral< TIM_TypeDef >, public detail::Timer< Timer > {
public:
    template < typename... Configs >
    Timer( TIM_TypeDef *periph = nullptr, Configs... configs )
        : Peripheral< TIM_TypeDef >( periph )
    {
        setup(std::forward< Configs >( configs )... );
    }

    template < typename... Configs >
    void setup( Configs... configs ) {
        enableClock();
        LL_TIM_EnableARRPreload( _periph );
        ( configs.post( _periph ), ... );
    }

    void enable() {
        LL_TIM_EnableCounter( _periph );
    }

    void disable() {
        LL_TIM_DisableCounter( _periph );
    }

    class Pwm {
    public:
        Pwm(): _periph( nullptr ), _channel( 0 ) {};

        void attachPin( Gpio::Pin pin ) {
            pin.port().enableClock();

            LL_GPIO_InitTypeDef GPIO_InitStruct{};
            GPIO_InitStruct.Pin = 1 << pin._pos;
            GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
            GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
            GPIO_InitStruct.Alternate = alternativeFun( pin, _periph, _channel );

            LL_GPIO_Init( pin._periph, &GPIO_InitStruct );
        }

        void enable() {
            LL_TIM_CC_EnableChannel( _periph, _channel );
        }

        void disable() {
            LL_TIM_CC_DisableChannel( _periph, _channel );
        }

        void set( int val ) {
            if ( _channel == LL_TIM_CHANNEL_CH1 )
                LL_TIM_OC_SetCompareCH1( _periph, val );
            else {
                // ToDo: missing timers
                assert( false && "Invalid channel specified " );
            }
        }

        int top() {
            return LL_TIM_GetAutoReload( _periph );
        }
    private:
        Pwm( TIM_TypeDef *periph, int channel )
            : _periph( periph ), _channel( channel )
        {
            LL_TIM_OC_InitTypeDef cfg{};
            cfg.OCMode = LL_TIM_OCMODE_PWM1;
            cfg.OCState = LL_TIM_OCSTATE_DISABLE;
            cfg.OCNState = LL_TIM_OCSTATE_DISABLE;
            cfg.CompareValue = 0;
            cfg.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
            cfg.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
            cfg.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
            cfg.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;

            LL_TIM_OC_Init( _periph, _channel, &cfg );
            LL_TIM_OC_EnablePreload( _periph, _channel );
            LL_TIM_OC_DisableFast( _periph, _channel );
            LL_TIM_SetTriggerOutput( _periph, LL_TIM_TRGO_RESET );
            // LL_TIM_SetTriggerOutput2( _periph, LL_TIM_TRGO2_RESET );
            LL_TIM_DisableMasterSlaveMode( _periph );
            LL_TIM_EnableAllOutputs( _periph );
        }

        TIM_TypeDef *_periph;
        int _channel;
        friend class Timer;
    };

    Pwm pwmChannel( int channel ) {
        return Pwm( _periph, channel );
    }
};

struct FreqAndRes {
    FreqAndRes( int frequency, int resolution ): _freq( frequency ), _res( resolution ) {}
    void post( TIM_TypeDef *periph ) {
        LL_TIM_SetPrescaler( periph,
            __LL_TIM_CALC_PSC( SystemCoreClock, _freq * _res ) );
        LL_TIM_EnableARRPreload( periph );
        LL_TIM_SetAutoReload( periph,
            __LL_TIM_CALC_ARR( SystemCoreClock, LL_TIM_GetPrescaler( periph ), _freq ) );
    }
    int _freq;
    int _res;
};