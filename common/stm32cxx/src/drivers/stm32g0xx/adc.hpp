#pragma once

#include <stm32g0xx_ll_adc.h>
#include <stm32g0xx_hal.h>
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
        assert( false && "Not implemented" );
    }

    void setupDefaultSampling() {
        LL_ADC_SetSamplingTimeCommonChannels( self()._periph,
            LL_ADC_SAMPLINGTIME_160CYCLES_5 );
    }
};

} // namespace detail
