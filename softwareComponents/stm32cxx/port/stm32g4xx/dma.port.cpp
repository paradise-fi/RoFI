#include <drivers/dma.hpp>

extern "C" void DMA1_Channel1_IRQHandler() {
    Dma::channelData( DMA1, 1 )._handleIsr( DMA1, 1 );
}

extern "C" void DMA1_Channel2_IRQHandler() {
    Dma::channelData( DMA1, 2 )._handleIsr( DMA1, 2 );
}

extern "C" void DMA1_Channel3_IRQHandler() {
    Dma::channelData( DMA1, 3 )._handleIsr( DMA1, 3 );
}

extern "C" void DMA1_Channel4_IRQHandler() {
    Dma::channelData( DMA1, 4 )._handleIsr( DMA1, 4 );
}

extern "C" void DMA1_Channel5_IRQHandler() {
    Dma::channelData( DMA1, 5 )._handleIsr( DMA1, 5 );
}

extern "C" void DMA1_Channel6_IRQHandler() {
    Dma::channelData( DMA1, 6 )._handleIsr( DMA1, 6 );
}
