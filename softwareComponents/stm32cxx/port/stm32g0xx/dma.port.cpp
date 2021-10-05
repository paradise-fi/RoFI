#include <drivers/dma.hpp>

extern "C" void DMA1_Channel1_IRQHandler() {
    Dma::channelData( DMA1, 1 )._handleIsr( DMA1, 1 );
}

extern "C" void DMA1_Channel2_3_IRQHandler() {
    Dma::channelData( DMA1, 2 )._handleIsr( DMA1, 2 );
    Dma::channelData( DMA1, 3 )._handleIsr( DMA1, 3 );
}

extern "C" void DMA1_Ch4_7_DMAMUX1_OVR_IRQHandler() {
    for ( int i = 4; i <= 7; i++ )
        Dma::channelData( DMA1, i )._handleIsr( DMA1, i );
}
