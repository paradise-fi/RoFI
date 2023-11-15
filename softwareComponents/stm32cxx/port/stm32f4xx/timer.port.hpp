#pragma once

#include <system/assert.hpp>
#include <stm32f4xx_ll_tim.h>
#include <stm32f4xx_ll_bus.h>

namespace detail {
template < typename Self >
class Timer {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    void enableClock() {
        if ( self()._periph == TIM1 )
            LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_TIM1 );
        else if ( self()._periph == TIM2 )
            LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_TIM2 );
        else {
            impossible( "Invalid timer specified " );
        }
    }

    static int alternativeFun( ::Gpio::Pin pin, TIM_TypeDef* periph, int channel ) {
        if ( periph == TIM1 ) {
            if ( pin._periph == GPIOA && pin._pos == 8 && channel == 1 )
                return LL_GPIO_AF_1;
            if ( pin._periph == GPIOB && pin._pos == 13 && channel == 1 )
                return LL_GPIO_AF_1;
        }
        impossible( "Uknown pin specified" );
    }

};

} // namespace detail
