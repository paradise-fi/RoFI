#include "uart.hpp"

std::array< Uart::Handlers, 4 > Uart::_uarts;

extern "C" void USART1_IRQHandler() {
    Uart::handlers( USART1 )._handleIsr( USART1 );
}

extern "C" void USART2_IRQHandler() {
    Uart::handlers( USART2 )._handleIsr( USART2 );
}

extern "C" void USART3_4_LPUART1_IRQHandler() {
    Uart::handlers( USART3 )._handleIsr( USART3 );
    Uart::handlers( USART4 )._handleIsr( USART4 );
}