#pragma once

#include <system/assert.hpp>
#include <stm32g0xx_ll_tim.h>
#include <stm32g0xx_ll_bus.h>

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
        if ( periph == TIM1 && channel == LL_TIM_CHANNEL_CH1 ) {
            if ( pin._periph == GPIOA && pin._pos == 8 )
                return LL_GPIO_AF_2;
            impossible( "Incorrect Output pin pin" );
        }
        // ToDo: More configurations
        impossible( "Incorrect Output pin pin" );
    }


};

} // namespace detail
