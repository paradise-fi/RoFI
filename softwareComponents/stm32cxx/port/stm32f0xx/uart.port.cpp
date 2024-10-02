#include "../uart.hpp"

#ifdef STM32F0xx

extern "C" void USART1_IRQHandler() {
    Uart::handlers( USART1 )._handleIsr( USART1 );
}

#endif
