#pragma once

#include <adc.port.hpp>
#include <drivers/peripheral.hpp>

struct Adc: public Peripheral< ADC_TypeDef >, public detail::Adc< Adc > {
    Adc( ADC_TypeDef *_periph ) : Peripheral< ADC_TypeDef >( _periph ) {}

    template < typename... Configs >
    void setup( Configs... configs ) {
        LL_ADC_InitTypeDef initStruct = _defaultInitStruct();
        LL_ADC_REG_InitTypeDef regInitStruct = _defaultRegInitStruct();

        enableClock();

        ( configs.pre( initStruct ), ... );
        LL_ADC_Init( _periph, &initStruct );

        ( configs.pre( regInitStruct ), ... );
        LL_ADC_REG_Init( _periph, &regInitStruct );

        setupDefaultScanDirection();
        setupDefaultSampling();
        disableConversionInterrupts();

        ( configs.post( _periph ), ... );
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