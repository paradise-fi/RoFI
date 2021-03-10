#include <drivers/spi.hpp>

extern "C" void SPI1_IRQHandler() {
    Spi::handlers( SPI1 )._handleIsr( SPI1 );
}

extern "C" void SPI2_IRQHandler() {
    Spi::handlers( SPI1 )._handleIsr( SPI2 );
}