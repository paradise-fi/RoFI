#include <drivers/adc.hpp>

#ifdef ADC1
    Adc Adc1( ADC1 );
#endif
#ifdef ADC2
    Adc Adc2( ADC2 );
#endif
