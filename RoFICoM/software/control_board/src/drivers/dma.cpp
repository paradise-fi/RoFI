#include "dma.hpp"

std::array< Dma::Handlers, 7 > Dma::_channels;

extern "C" void DMA1_Channel1_IRQHandler() {
    Dma::channel( 1 )._handleIsr( 1 );
}

extern "C" void DMA1_Channel2_3_IRQHandler() {
    Dma::channel( 2 )._handleIsr( 2 );
}

extern "C" void DMA1_Ch4_7_DMAMUX1_OVR_IRQHandler() {
    for ( int i = 4; i <= 7; i++ )
        Dma::channel( i )._handleIsr( i );
}