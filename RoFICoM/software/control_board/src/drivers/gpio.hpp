#pragma once

#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_gpio.h>


struct Gpio {
    Gpio( GPIO_TypeDef *_periph ) : _periph( _periph ) {}

    struct Pin {
        Gpio port() const {
            return Gpio( _periph );
        }

        int _pos;
        GPIO_TypeDef *_periph;
    };

    void enableClock() {
        if ( _periph == GPIOA )
            LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOA );
        else if ( _periph == GPIOB )
            LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOB );
    }

    void disableClock() {
        if ( _periph == GPIOA )
            LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOA );
        else if ( _periph == GPIOB )
            LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOB );
    }

    Pin operator[]( int pin ) {
        return { pin, _periph };
    }

    GPIO_TypeDef *_periph;
};


extern Gpio GpioA;
extern Gpio GpioB;