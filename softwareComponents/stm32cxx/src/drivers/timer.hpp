#pragma once

#include <cassert>
#include <utility>

#include <stm32g0xx_ll_tim.h>
#include <stm32g0xx_ll_bus.h>

#include <drivers/peripheral.hpp>
#include <drivers/gpio.hpp>

class Timer: public Peripheral< TIM_TypeDef > {
public:
    template < typename... Configs >
    Timer( TIM_TypeDef *periph = nullptr, Configs... configs )
        : Peripheral< TIM_TypeDef >( periph )
    {
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

    void enableClock() {
        if ( _periph == TIM1 )
            LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
        else if ( _periph == TIM2 )
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
        else {
            // ToDo: missing timers
            assert( false && "Invalid timer specified " );
        }
    }

    class Pwm {
    public:
        void attachPin( Gpio::Pin pin ) {
            pin.port().enableClock();

            LL_GPIO_InitTypeDef GPIO_InitStruct{};
            GPIO_InitStruct.Pin = 1 << pin._pos;
            GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
            GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
            GPIO_InitStruct.Alternate = alternativeFun( pin );

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
            LL_TIM_OC_DisableFast( _periph, _channel );
            LL_TIM_SetTriggerOutput( _periph, LL_TIM_TRGO_RESET );
            LL_TIM_SetTriggerOutput2( _periph, LL_TIM_TRGO2_RESET );
            LL_TIM_DisableMasterSlaveMode( _periph );
            LL_TIM_EnableAllOutputs( _periph );
        }

        int alternativeFun( Gpio::Pin pin ) {
            if ( _periph == TIM1 && _channel == LL_TIM_CHANNEL_CH1 ) {
                if ( pin._periph == GPIOA && pin._pos == 8 )
                    return LL_GPIO_AF_2;
                assert( false && "Incorrect Output pin pin" );
            }
            // ToDo: More configurations
            assert( false && "Incorrect Output pin pin" );
            __builtin_trap();
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