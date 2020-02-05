#include "../dma.hpp"

#ifdef STM32G0xx

extern "C" void DMA1_Channel1_IRQHandler() {
    Dma::channel( 1 )._handleIsr( 1 );
}

extern "C" void DMA1_Channel2_3_IRQHandler() {
    Dma::channel( 2 )._handleIsr( 2 );
    Dma::channel( 3 )._handleIsr( 3 );
}

extern "C" void DMA1_Ch4_7_DMAMUX1_OVR_IRQHandler() {
    for ( int i = 4; i <= 7; i++ )
        Dma::channel( i )._handleIsr( i );
}

#endif