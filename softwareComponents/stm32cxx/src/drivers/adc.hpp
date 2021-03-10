#pragma once

#if defined(STM32G0xx)
    #include <drivers/stm32g0xx/adc.hpp>
#elif defined(STM32F0xx)
    #include <drivers/stm32f0xx/adc.hpp>
#else
    #error "Unsuported MCU family"
#endif

#include <drivers/peripheral.hpp>

struct Adc: public Peripheral< ADC_TypeDef >, public detail::Adc< Adc > {
    Adc( ADC_TypeDef *_periph ) : Peripheral< ADC_TypeDef >( _periph ) {}

    template < typename... Configs >
    void setup( Configs... configs ) {
        LL_ADC_InitTypeDef initStruct{};
        LL_ADC_REG_InitTypeDef regInitStruct{};

        enableClock();

        initStruct.Clock = LL_ADC_CLOCK_ASYNC;
        initStruct.Resolution = LL_ADC_RESOLUTION_12B;
        initStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
        initStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
        ( configs.pre( initStruct ), ... );
        LL_ADC_Init( _periph, &initStruct );

        regInitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
        regInitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
        regInitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
        regInitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_LIMITED;
        regInitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;
        ( configs.pre( regInitStruct ), ... );
        LL_ADC_REG_Init( _periph, &regInitStruct );

        LL_ADC_REG_SetSequencerScanDirection( _periph,
            LL_ADC_REG_SEQ_SCAN_DIR_FORWARD );
        setupDefaultSampling();
        LL_ADC_DisableIT_EOC( _periph );
        LL_ADC_DisableIT_EOS( _periph );

        ( configs.post( _periph ), ... );
    }

    void calibrate() {
        assert( !LL_ADC_IsEnabled( _periph ) && "Cannot calibrate on enabled ADC" );
        LL_ADC_StartCalibration( _periph );
        while( LL_ADC_IsCalibrationOnGoing( _periph ) );
        // At least LL_ADC_DELAY_CALIB_ENABLE_ADC_CYCLES must pass
        // ToDo: We act safe & slow here, update if necessary
        HAL_Delay( 1 );
    }

    void enable() {
        LL_ADC_Enable( _periph );
    }

    void disable() {
        LL_ADC_Disable( _periph );
    }
};

#ifdef ADC1
    extern Adc Adc1;
#endif
#ifdef ADC2
    extern Adc Adc2;
#endif