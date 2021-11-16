#pragma once

#include <drivers/hal.hpp>

struct IrqGuard {
    IrqGuard( int priority ): _originalPri( __get_BASEPRI() ) {
        __set_BASEPRI( priority << ( 8 - __NVIC_PRIO_BITS ) );
    }
    ~IrqGuard() {
        give();
    }

    void give() {
        if ( _originalPri < 0 )
            return;
        __set_BASEPRI( _originalPri );
        _originalPri = -1;
    }

    int _originalPri;
};

struct IrqMask {
    IrqMask() {
        __disable_irq();
    }

    ~IrqMask() {
        __enable_irq();
    }

    void give() {
        __enable_irq();
    }
};
