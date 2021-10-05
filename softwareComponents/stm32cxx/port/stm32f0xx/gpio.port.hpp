#pragma once

#include <stm32f0xx_ll_bus.h>
#include <stm32f0xx_ll_gpio.h>
#include <stm32f0xx_ll_exti.h>
#include <stm32f0xx_ll_adc.h>

#include <cassert>

namespace detail {

template < typename Self >
class Gpio {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    void enableClock() {
        if ( self()._periph == GPIOA )
            LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_GPIOA );
        else if ( self()._periph == GPIOB )
            LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_GPIOB );
        else if ( self()._periph == GPIOC )
            LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_GPIOC );
        else
            assert( false && "Uknown clock" );
    }

    void disableClock() {
        if ( self()._periph == GPIOA )
            LL_AHB1_GRP1_DisableClock( LL_AHB1_GRP1_PERIPH_GPIOA );
        else if ( self()._periph == GPIOB )
            LL_AHB1_GRP1_DisableClock( LL_AHB1_GRP1_PERIPH_GPIOB );
        else if ( self()._periph == GPIOC )
            LL_AHB1_GRP1_DisableClock( LL_AHB1_GRP1_PERIPH_GPIOC );
    }

    static int _extiConfigLine( int pos ) {
        assert( false && "Not implemented" );
        __builtin_trap();
    }

    static void _handleIrq( int line ) {
        assert( false && "Not implemented" );
    }

    void setExtiSource( int line ) {
        assert( false && "Not implemented" );
    }

protected:
    static uint32_t _getAdcChannel( GPIO_TypeDef *port, int pos ) {
        if ( port == GPIOA ) {
            // LL_AD_CHANNEL are magic constants, so we (unfortunatelly) have to switch...
            switch ( pos) {
                case 0: return LL_ADC_CHANNEL_0;
                case 1: return LL_ADC_CHANNEL_1;
                case 2: return LL_ADC_CHANNEL_2;
                case 3: return LL_ADC_CHANNEL_3;
                case 4: return LL_ADC_CHANNEL_4;
                case 5: return LL_ADC_CHANNEL_5;
                case 6: return LL_ADC_CHANNEL_6;
                case 7: return LL_ADC_CHANNEL_7;
            }
        }
        if ( port == GPIOB ) {
            switch ( pos ) {
                case 0: return LL_ADC_CHANNEL_8;
                case 1: return LL_ADC_CHANNEL_9;
            }
        }
        assert( false && "Invalid pin specified" );
        __builtin_trap();
    }

    static ADC_TypeDef *_getAdcPeriph( GPIO_TypeDef *port, int pos ) {
        if ( port == GPIOA && pos >= 0 && pos <= 7 )
            return ADC1;
        if ( port == GPIOB && pos >= 0 && pos <= 1 )
            return ADC1;
        assert( false && "Invalid pin specified" );
        __builtin_trap();
    }
};

} // namespace detail