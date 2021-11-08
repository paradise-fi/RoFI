#pragma once

#include <stm32g4xx_ll_bus.h>
#include <stm32g4xx_ll_gpio.h>
#include <stm32g4xx_ll_exti.h>
#include <stm32g4xx_ll_system.h>

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
            return LL_AHB2_GRP1_EnableClock( LL_AHB2_GRP1_PERIPH_GPIOA );
        #ifdef GPIOB
            else if ( self()._periph == GPIOB )
                return LL_AHB2_GRP1_EnableClock( LL_AHB2_GRP1_PERIPH_GPIOB );
        #endif
        #ifdef GPIOC
            else if ( self()._periph == GPIOC )
                return LL_AHB2_GRP1_EnableClock( LL_AHB2_GRP1_PERIPH_GPIOC );
        #endif
        #ifdef GPIOD
            else if ( self()._periph == GPIOD )
                return LL_AHB2_GRP1_EnableClock( LL_AHB2_GRP1_PERIPH_GPIOC );
        #endif
        #ifdef GPIOE
            else if ( self()._periph == GPIOE )
                return LL_AHB2_GRP1_EnableClock( LL_AHB2_GRP1_PERIPH_GPIOC );
        #endif
        #ifdef GPIOF
            else if ( self()._periph == GPIOF )
                return LL_AHB2_GRP1_EnableClock( LL_AHB2_GRP1_PERIPH_GPIOC );
        #endif
        #ifdef GPIOG
            else if ( self()._periph == GPIOG )
                return LL_AHB2_GRP1_EnableClock( LL_AHB2_GRP1_PERIPH_GPIOC );
        #endif
        assert( false && "Invalid peripheral specified" );
    }


    void disableClock() {
        if ( self()._periph == GPIOA )
            return LL_AHB2_GRP1_DisableClock( LL_AHB2_GRP1_PERIPH_GPIOA );
        #ifdef GPIOB
            else if ( self()._periph == GPIOB )
                return LL_AHB2_GRP1_DisableClock( LL_AHB2_GRP1_PERIPH_GPIOB );
        #endif
        #ifdef GPIOC
            else if ( self()._periph == GPIOC )
                return LL_AHB2_GRP1_DisableClock( LL_AHB2_GRP1_PERIPH_GPIOC );
        #endif
        #ifdef GPIOD
            else if ( self()._periph == GPIOD )
                return LL_AHB2_GRP1_DisableClock( LL_AHB2_GRP1_PERIPH_GPIOC );
        #endif
        #ifdef GPIOE
            else if ( self()._periph == GPIOE )
                return LL_AHB2_GRP1_DisableClock( LL_AHB2_GRP1_PERIPH_GPIOC );
        #endif
        #ifdef GPIOF
            else if ( self()._periph == GPIOF )
                return LL_AHB2_GRP1_DisableClock( LL_AHB2_GRP1_PERIPH_GPIOC );
        #endif
        #ifdef GPIOG
            else if ( self()._periph == GPIOG )
                return LL_AHB2_GRP1_DisableClock( LL_AHB2_GRP1_PERIPH_GPIOC );
        #endif
        assert( false && "Invalid peripheral specified" );
    }

    static IRQn_Type _positionToInterrupt( int pos ) {
        return    pos == 0 ? EXTI0_IRQn
                : pos == 1 ? EXTI1_IRQn
                : pos == 2 ? EXTI2_IRQn
                : pos == 3 ? EXTI3_IRQn
                : pos == 4 ? EXTI4_IRQn
                : pos <= 9 ? EXTI9_5_IRQn
                : EXTI15_10_IRQn;
    }

    static uint32_t _extiConfigLine( int pos ) {
        static const constexpr uint32_t table[ 16 ] = {
            LL_SYSCFG_EXTI_LINE1,
            LL_SYSCFG_EXTI_LINE2,
            LL_SYSCFG_EXTI_LINE3,
            LL_SYSCFG_EXTI_LINE4,
            LL_SYSCFG_EXTI_LINE5,
            LL_SYSCFG_EXTI_LINE6,
            LL_SYSCFG_EXTI_LINE7,
            LL_SYSCFG_EXTI_LINE8,
            LL_SYSCFG_EXTI_LINE9,
            LL_SYSCFG_EXTI_LINE10,
            LL_SYSCFG_EXTI_LINE11,
            LL_SYSCFG_EXTI_LINE12,
            LL_SYSCFG_EXTI_LINE13,
            LL_SYSCFG_EXTI_LINE14,
            LL_SYSCFG_EXTI_LINE15
        };
        return table[ pos ];
    }

    static uint32_t _extiPort( int portIdx ) {
        static constexpr uint32_t table[] = {
            LL_SYSCFG_EXTI_PORTA,
            LL_SYSCFG_EXTI_PORTB,
            LL_SYSCFG_EXTI_PORTC,
            #ifdef LL_SYSCFG_EXTI_PORTD
                LL_SYSCFG_EXTI_PORTD,
            #endif
            #ifdef LL_SYSCFG_EXTI_PORTE
                LL_SYSCFG_EXTI_PORTE,
            #endif
            #ifdef LL_SYSCFG_EXTI_PORTF
                LL_SYSCFG_EXTI_PORTF,
            #endif
        };
        return table[ portIdx ];
    }

    static void _handleIrq( int /*line*/ ) {
        // int l = 1 << line;
        // TBA
        assert( false );
        // if ( !LL_EXTI_IsEnabledIT_0_31( l ) )
        //     return;
        // if ( LL_EXTI_IsActiveFallingFlag_0_31( l ) ) {
        //     LL_EXTI_ClearFallingFlag_0_31( l );
        //     Self::_lines[ line ]( false );
        // }
        // if ( LL_EXTI_IsActiveRisingFlag_0_31( l ) ) {
        //     LL_EXTI_ClearRisingFlag_0_31( l );
        //     Self::_lines[ line ]( true );
        // }
    }

    void setExtiSource( int line ) {
        int portIdx = indexOf( self()._periph, self().availablePeripherals );
        LL_SYSCFG_SetEXTISource( _extiPort( portIdx ), _extiConfigLine( line ) );
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