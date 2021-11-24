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
        assert( self()._periph == ADC1 );
        LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_ADC1 );
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
        regInitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_NONE;
        return regInitStruct;
    }

    void setupDefaultScanDirection() {
        // Not available on this family
    }

    void setupDefaultSampling() {
        _samplingRate = LL_ADC_SAMPLINGTIME_144CYCLES;
    }

    void calibrate() {
        // Not available on this family
    }

    void disableConversionInterrupts() {
        LL_ADC_DisableIT_EOCS( self()._periph );
    }

    void platformPostConfiguration() {
        LL_ADC_CommonInitTypeDef commonInit = { 0 };
        commonInit.CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV2;
        LL_ADC_CommonInit( __LL_ADC_COMMON_INSTANCE( self().periph ), &commonInit );
    }

    void startSingleConversion( int channelMask ) {
        LL_ADC_REG_SetSequencerRanks( self()._periph,
            LL_ADC_REG_RANK_1, channelMask );
        LL_ADC_SetChannelSamplingTime( self()._periph,
            channelMask, _samplingRate );
        LL_ADC_REG_StartConversionSWStart( self()._periph );
    }

    void waitForConversion() {
        while ( !LL_ADC_IsActiveFlag_EOCS( self()._periph ) );
        LL_ADC_ClearFlag_EOCS( self()._periph );
    }

private:
    uint32_t _samplingRate;
};

} // namespace detail
