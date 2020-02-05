#include "../dma.hpp"

#ifdef STM32F0xx

extern "C" void DMA1_Channel2_3_IRQHandler() {
    Dma::channel( 2 )._handleIsr( 2 );
    Dma::channel( 3 )._handleIsr( 3 );
}

// ToDo: Add handlers

#endif