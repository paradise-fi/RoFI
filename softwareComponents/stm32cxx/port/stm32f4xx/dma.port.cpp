#include <drivers/dma.hpp>

extern "C" void DMA1_Stream0_IRQHandler() {
    Dma::channelData( DMA1, 0 )._handleIsr( DMA1, 0 );
}

extern "C" void DMA1_Stream1_IRQHandler() {
    Dma::channelData( DMA1, 1 )._handleIsr( DMA1, 1 );
}

extern "C" void DMA1_Stream2_IRQHandler() {
    Dma::channelData( DMA1, 2 )._handleIsr( DMA1, 2 );
}

extern "C" void DMA1_Stream3_IRQHandler() {
    Dma::channelData( DMA1, 3 )._handleIsr( DMA1, 3 );
}

extern "C" void DMA1_Stream4_IRQHandler() {
    Dma::channelData( DMA1, 4 )._handleIsr( DMA1, 4 );
}

extern "C" void DMA1_Stream5_IRQHandler() {
    Dma::channelData( DMA1, 5 )._handleIsr( DMA1, 5 );
}

extern "C" void DMA1_Stream6_IRQHandler() {
    Dma::channelData( DMA1, 6 )._handleIsr( DMA1, 6 );
}

extern "C" void DMA1_Stream7_IRQHandler() {
    Dma::channelData( DMA1, 7 )._handleIsr( DMA1, 7 );
}

extern "C" void DMA2_Stream0_IRQHandler() {
    Dma::channelData( DMA2, 0 )._handleIsr( DMA2, 0 );
}

extern "C" void DMA2_Stream1_IRQHandler() {
    Dma::channelData( DMA2, 1 )._handleIsr( DMA2, 1 );
}

extern "C" void DMA2_Stream2_IRQHandler() {
    Dma::channelData( DMA2, 2 )._handleIsr( DMA2, 2 );
}

extern "C" void DMA2_Stream3_IRQHandler() {
    Dma::channelData( DMA2, 3 )._handleIsr( DMA2, 3 );
}

extern "C" void DMA2_Stream4_IRQHandler() {
    Dma::channelData( DMA2, 4 )._handleIsr( DMA2, 4 );
}

extern "C" void DMA2_Stream5_IRQHandler() {
    Dma::channelData( DMA2, 5 )._handleIsr( DMA2, 5 );
}

extern "C" void DMA2_Stream6_IRQHandler() {
    Dma::channelData( DMA2, 6 )._handleIsr( DMA2, 6 );
}

extern "C" void DMA2_Stream7_IRQHandler() {
    Dma::channelData( DMA2, 7 )._handleIsr( DMA2, 7 );
}
