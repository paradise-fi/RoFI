#pragma once

#include <stm32f0xx_ll_adc.h>
#include <stm32f0xx_ll_bus.h>
#include <stm32f0xx_hal.h>
#include <cassert>

namespace detail {

template < typename Self >
class Adc {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    void enableClock() {
        if ( self()._periph == ADC1 ) {
            LL_APB1_GRP2_EnableClock( LL_APB1_GRP2_PERIPH_ADC1 );
            return;
        }
        assert( false && "Invalid peripheral specified" );
    }

    void setupDefaultSampling() {
        LL_ADC_SetSamplingTimeCommonChannels( self()._periph,
            LL_ADC_SAMPLINGTIME_239CYCLES_5 );
    }
};

} // namespace detail
