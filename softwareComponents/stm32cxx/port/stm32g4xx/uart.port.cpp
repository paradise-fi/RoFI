#include <drivers/uart.hpp>

extern "C" void USART1_IRQHandler() {
    Uart::handlers( USART1 )._handleIsr( USART1 );
}

extern "C" void USART2_IRQHandler() {
    Uart::handlers( USART2 )._handleIsr( USART2 );
}
