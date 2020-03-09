#pragma once

#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_gpio.h>
#include <stm32g0xx_ll_exti.h>

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
            LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOA );
        else if ( self()._periph == GPIOB )
            LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOB );
    }

    void disableClock() {
        if ( self()._periph == GPIOA )
            LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOA );
        else if ( self()._periph == GPIOB )
            LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOB );
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
            Self::_lines[ line ]( false );
        }
        if ( LL_EXTI_IsActiveRisingFlag_0_31( l ) ) {
            LL_EXTI_ClearRisingFlag_0_31( l );
            Self::_lines[ line ]( true );
        }
    }

    void setExtiSource( int line ) {
        if ( self()._periph == GPIOA )
            LL_EXTI_SetEXTISource( LL_EXTI_CONFIG_PORTA, _extiConfigLine( line ) );
        else
            LL_EXTI_SetEXTISource( LL_EXTI_CONFIG_PORTB, _extiConfigLine( line ) );
    }

protected:
    static uint32_t _getAdcChannel( GPIO_TypeDef */*port*/, int /*pos*/ ) {
        assert( false && "Not implemented" );
        __builtin_trap();
    }

    static ADC_TypeDef *_getAdcPeriph( GPIO_TypeDef */*port*/, int /*pos*/ ) {
        assert( false && "Not implemented" );
        __builtin_trap();
    }
};


} // namespace detail