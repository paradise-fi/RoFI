#pragma once

#pragma once

#include <stm32f4xx_ll_adc.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_ll_bus.h>
#include <system/assert.hpp>

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
        assert( false );
        // TBA
        LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_ADC );
    }

    static LL_ADC_InitTypeDef _defaultInitStruct() {
        LL_ADC_InitTypeDef initStruct{};
        initStruct.Resolution = LL_ADC_RESOLUTION_12B;
        initStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
        initStruct.SequencersScanMode = LL_ADC_SEQ_SCAN_DISABLE;
        return initStruct;
    }

    static LL_ADC_REG_InitTypeDef _defaultRegInitStruct() {
        LL_ADC_REG_InitTypeDef regInitStruct{};
        regInitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
        regInitStruct.SequencerLength = 1;
        regInitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
        regInitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
        regInitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_LIMITED;
        return regInitStruct;
    }

    void setupDefaultScanDirection() {
        // Not available on this family
    }

    void setupDefaultSampling() {
        // Not available on this family
    }

    void calibrate() {
        // Not available on this family
    }

    void disableConversionInterrupts() {
        LL_ADC_DisableIT_EOCS( self()._periph );
    }
};

} // namespace detail
