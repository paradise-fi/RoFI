#include <drivers/spi.hpp>

std::array< Spi::Handlers, 2 > Spi::_spis;

extern "C" void SPI1_IRQHandler() {
    Spi::handlers( SPI1 )._handleIsr( SPI1 );
}

extern "C" void SPI2_IRQHandler() {
    Spi::handlers( SPI1 )._handleIsr( SPI2 );
}