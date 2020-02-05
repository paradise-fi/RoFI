#pragma once

#include <stm32f0xx_ll_bus.h>
#include <stm32f0xx_ll_gpio.h>
#include <stm32f0xx_ll_exti.h>

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
        else
            assert( false && "Uknown clock" );
    }

    void disableClock() {
        if ( self()._periph == GPIOA )
            LL_AHB1_GRP1_DisableClock( LL_AHB1_GRP1_PERIPH_GPIOA );
        else if ( self()._periph == GPIOB )
            LL_AHB1_GRP1_DisableClock( LL_AHB1_GRP1_PERIPH_GPIOB );
    }

    static int _extiConfigLine( int pos ) {
        assert( false && "Not implemented" );
    }

    static void _handleIrq( int line ) {
        assert( false && "Not implemented" );
    }

    void setExtiSource( int line ) {
        assert( false && "Not implemented" );
    }
};

} // namespace detail