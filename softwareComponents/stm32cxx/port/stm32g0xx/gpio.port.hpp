#pragma once

#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_gpio.h>
#include <stm32g0xx_ll_exti.h>

#include <drivers/adc.hpp>
#include <system/util.hpp>
#include <system/assert.hpp>

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
            LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOA );
        else if ( self()._periph == GPIOB )
            LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOB );
        else if ( self()._periph == GPIOC )
            LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOC );
        else if ( self()._periph == GPIOD )
            LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOD );
        else
            assert( false && "Gpio enableclock not implemented" );
    }

    void disableClock() {
        if ( self()._periph == GPIOA )
            LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOA );
        else if ( self()._periph == GPIOB )
            LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOB );
        else if ( self()._periph == GPIOC )
            LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOC );
        else if ( self()._periph == GPIOD )
            LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOD );
        else
            assert( false && "Gpio disableclock not implemented" );
    }

    static IRQn_Type _positionToInterrupt( int pos ) {
        return    pos == 1 ? EXTI0_1_IRQn
                : pos <= 3 ? EXTI2_3_IRQn
                : EXTI4_15_IRQn;
    }

    static uint32_t _extiConfigLine( int pos ) {
        static const constexpr uint32_t table[ 16 ] = {
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

    static uint32_t _extiPort( int portIdx ) {
        static constexpr uint32_t table[] = {
            LL_EXTI_CONFIG_PORTA,
            LL_EXTI_CONFIG_PORTB,
            LL_EXTI_CONFIG_PORTC,
            #ifdef LL_EXTI_CONFIG_PORTD
                LL_EXTI_CONFIG_PORTD,
            #endif
            #ifdef LL_EXTI_CONFIG_PORTE
                LL_EXTI_CONFIG_PORTE,
            #endif
            #ifdef LL_EXTI_CONFIG_PORTF
                LL_EXTI_CONFIG_PORTF,
            #endif
        };
        return table[ portIdx ];
    }

    static void _handleIrq( int line ) {
        int l = 1 << line;
        if ( !LL_EXTI_IsEnabledIT_0_31( l ) )
            return;
        if ( LL_EXTI_IsActiveFallingFlag_0_31( l ) ) {
            LL_EXTI_ClearFallingFlag_0_31( l );
            Self::_lines[ line ]( false );
        }
        if ( LL_EXTI_IsActiveRisingFlag_0_31( l ) ) {
            LL_EXTI_ClearRisingFlag_0_31( l );
            Self::_lines[ line ]( true );
        }
    }

    void setExtiSource( int line ) {
        int portIdx = indexOf( self()._periph, self().availablePeripherals );
        LL_EXTI_SetEXTISource( _extiPort( portIdx ), _extiConfigLine( line ) );
    }

protected:
    static uint32_t _getAdcChannel( GPIO_TypeDef *port, int pos ) {
         if ( port == GPIOA ) {
            switch ( pos ) {
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
                case 2: return LL_ADC_CHANNEL_10;
                case 10: return LL_ADC_CHANNEL_11;
                case 11: return LL_ADC_CHANNEL_15;
                case 12: return LL_ADC_CHANNEL_16;
            }
        }
        if ( port == GPIOC ) {
            switch ( pos ) {
                case 4: LL_ADC_CHANNEL_17;
                case 5: LL_ADC_CHANNEL_18;
            }
        }
        assert( false && "Not implemented" );
        __builtin_trap();
    }

    static auto& _getAdc( GPIO_TypeDef */*port*/, int /*pos*/ ) {
        return Adc1;
        assert( false && "Not implemented" );
        __builtin_trap();
    }
};


} // namespace detail
