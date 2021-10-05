#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_usart.h>
#include <stm32g0xx_ll_gpio.h>
#include <cassert>

namespace detail {
    int uartAlternateFunTX( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if defined(STM32G030C6Tx) || defined(STM32G030C8Tx) || defined(STM32G030F6Px) || \
            defined(STM32G030J6Mx) || defined(STM32G030K6Tx) || defined(STM32G030K8Tx) || \
            defined(STM32G031C4Tx) || defined(STM32G031C6Tx) || defined(STM32G031C8Tx) || \
            defined(STM32G031C4Ux) || defined(STM32G031C6Ux) || defined(STM32G031C8Ux) || \
            defined(STM32G031F4Px) || defined(STM32G031F6Px) || defined(STM32G031F8Px) || \
            defined(STM32G031G4Ux) || defined(STM32G031G6Ux) || defined(STM32G031G8Ux) || \
            defined(STM32G031J4Mx) || defined(STM32G031J6Mx) || defined(STM32G031K4Tx) || \
            defined(STM32G031K6Tx) || defined(STM32G031K8Tx) || defined(STM32G031K4Ux) || \
            defined(STM32G031K6Ux) || defined(STM32G031K8Ux) || defined(STM32G031Y8Yx) || \
            defined(STM32G041C6Tx) || defined(STM32G041C8Tx) || defined(STM32G041C6Ux) || \
            defined(STM32G041C8Ux) || defined(STM32G041F6Px) || defined(STM32G041F8Px) || \
            defined(STM32G041G6Ux) || defined(STM32G041G8Ux) || defined(STM32G041J6Mx) || \
            defined(STM32G041K6Tx) || defined(STM32G041K8Tx) || defined(STM32G041K6Ux) || \
            defined(STM32G041K8Ux) || defined(STM32G041Y8Yx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 1;
                if ( port == GPIOA && pos == 9 )
                    return 1;
                if ( port == GPIOB && pos == 6 )
                    return 0;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 1;
                if ( port == GPIOA && pos == 14 )
                    return 1;
            }
        #endif
        #if defined(STM32G070CBTx) || defined(STM32G070KBTx) || defined(STM32G070RBTx) || \
            defined(STM32G071C6Tx) || defined(STM32G071C8Tx) || defined(STM32G071CBTx) || \
            defined(STM32G071C6Ux) || defined(STM32G071C8Ux) || defined(STM32G071CBUx) || \
            defined(STM32G071EBYx) || defined(STM32G071G6Ux) || defined(STM32G071G8Ux) || \
            defined(STM32G071GBUx) || defined(STM32G071G8UxN) || defined(STM32G071GBUxN) || \
            defined(STM32G071K6Tx) || defined(STM32G071K8Tx) || defined(STM32G071KBTx) || \
            defined(STM32G071K6Ux) || defined(STM32G071K8Ux) || defined(STM32G071KBUx) || \
            defined(STM32G071K8TxN) || defined(STM32G071KBTxN) || defined(STM32G071K8UxN) || \
            defined(STM32G071KBUxN) || defined(STM32G071R6Tx) || defined(STM32G071R8Tx) || \
            defined(STM32G071RBTx) || defined(STM32G071RBIx) || defined(STM32G081CBTx) || \
            defined(STM32G081CBUx) || defined(STM32G081EBYx) || defined(STM32G081GBUx) || \
            defined(STM32G081GBUxN) || defined(STM32G081KBTx) || defined(STM32G081KBTxN) || \
            defined(STM32G081KBUx) || defined(STM32G081KBUxN) || defined(STM32G081RBIx) || \
            defined(STM32G081RBTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 1;
                if ( port == GPIOA && pos == 9 )
                    return 1;
                if ( port == GPIOB && pos == 6 )
                    return 0;
                if ( port == GPIOC && pos == 4 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 1;
                if ( port == GPIOA && pos == 14 )
                    return 1;
                if ( port == GPIOD && pos == 5 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOA && pos == 5 )
                    return 4;
                if ( port == GPIOB && pos == 2 )
                    return 4;
                if ( port == GPIOB && pos == 8 )
                    return 4;
                if ( port == GPIOB && pos == 10 )
                    return 4;
                if ( port == GPIOC && pos == 4 )
                    return 0;
                if ( port == GPIOC && pos == 10 )
                    return 0;
                if ( port == GPIOD && pos == 8 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 0 )
                    return 4;
                if ( port == GPIOC && pos == 10 )
                    return 1;
            }
        #endif
        #if defined(STM32G0B0CETx) || defined(STM32G0B0KETx) || defined(STM32G0B0RETx) || \
            defined(STM32G0B0VETx) || defined(STM32G0B1CCTx) || defined(STM32G0B1CETx) || \
            defined(STM32G0B1CCUx) || defined(STM32G0B1CEUx) || defined(STM32G0B1KCTx) || \
            defined(STM32G0B1KETx) || defined(STM32G0B1KCTxN) || defined(STM32G0B1KETxN) || \
            defined(STM32G0B1KCUx) || defined(STM32G0B1KEUx) || defined(STM32G0B1KCUxN) || \
            defined(STM32G0B1KEUxN) || defined(STM32G0B1MCTx) || defined(STM32G0B1METx) || \
            defined(STM32G0B1RCIx) || defined(STM32G0B1REIx) || defined(STM32G0B1RCTx) || \
            defined(STM32G0B1RETx) || defined(STM32G0B1VCIx) || defined(STM32G0B1VEIx) || \
            defined(STM32G0B1VCTx) || defined(STM32G0B1VETx) || defined(STM32G0C1CCTx) || \
            defined(STM32G0C1CETx) || defined(STM32G0C1CCUx) || defined(STM32G0C1CEUx) || \
            defined(STM32G0C1KCTx) || defined(STM32G0C1KETx) || defined(STM32G0C1KCTxN) || \
            defined(STM32G0C1KETxN) || defined(STM32G0C1KCUx) || defined(STM32G0C1KEUx) || \
            defined(STM32G0C1KCUxN) || defined(STM32G0C1KEUxN) || defined(STM32G0C1MCTx) || \
            defined(STM32G0C1METx) || defined(STM32G0C1RCIx) || defined(STM32G0C1REIx) || \
            defined(STM32G0C1RCTx) || defined(STM32G0C1RETx) || defined(STM32G0C1VCIx) || \
            defined(STM32G0C1VEIx) || defined(STM32G0C1VCTx) || defined(STM32G0C1VETx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 1;
                if ( port == GPIOA && pos == 9 )
                    return 1;
                if ( port == GPIOB && pos == 6 )
                    return 0;
                if ( port == GPIOC && pos == 4 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 1;
                if ( port == GPIOA && pos == 14 )
                    return 1;
                if ( port == GPIOD && pos == 5 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOA && pos == 5 )
                    return 4;
                if ( port == GPIOB && pos == 2 )
                    return 4;
                if ( port == GPIOB && pos == 8 )
                    return 4;
                if ( port == GPIOB && pos == 10 )
                    return 4;
                if ( port == GPIOC && pos == 4 )
                    return 0;
                if ( port == GPIOC && pos == 10 )
                    return 0;
                if ( port == GPIOD && pos == 8 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 0 )
                    return 4;
                if ( port == GPIOC && pos == 10 )
                    return 1;
                if ( port == GPIOE && pos == 8 )
                    return 0;
            }
            if ( periph == USART5 ) {
                if ( port == GPIOB && pos == 0 )
                    return 8;
                if ( port == GPIOB && pos == 3 )
                    return 3;
                if ( port == GPIOC && pos == 12 )
                    return 3;
                if ( port == GPIOD && pos == 3 )
                    return 3;
                if ( port == GPIOE && pos == 10 )
                    return 3;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 4 )
                    return 3;
                if ( port == GPIOB && pos == 8 )
                    return 8;
                if ( port == GPIOC && pos == 0 )
                    return 4;
                if ( port == GPIOF && pos == 9 )
                    return 3;
            }
        #endif
        assert( false && "Incorrect TX pin" );
        __builtin_trap();
    }

    int uartAlternateFunRX( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if defined(STM32G030C6Tx) || defined(STM32G030C8Tx) || defined(STM32G030F6Px) || \
            defined(STM32G030J6Mx) || defined(STM32G030K6Tx) || defined(STM32G030K8Tx) || \
            defined(STM32G031C4Tx) || defined(STM32G031C6Tx) || defined(STM32G031C8Tx) || \
            defined(STM32G031C4Ux) || defined(STM32G031C6Ux) || defined(STM32G031C8Ux) || \
            defined(STM32G031F4Px) || defined(STM32G031F6Px) || defined(STM32G031F8Px) || \
            defined(STM32G031G4Ux) || defined(STM32G031G6Ux) || defined(STM32G031G8Ux) || \
            defined(STM32G031J4Mx) || defined(STM32G031J6Mx) || defined(STM32G031K4Tx) || \
            defined(STM32G031K6Tx) || defined(STM32G031K8Tx) || defined(STM32G031K4Ux) || \
            defined(STM32G031K6Ux) || defined(STM32G031K8Ux) || defined(STM32G031Y8Yx) || \
            defined(STM32G041C6Tx) || defined(STM32G041C8Tx) || defined(STM32G041C6Ux) || \
            defined(STM32G041C8Ux) || defined(STM32G041F6Px) || defined(STM32G041F8Px) || \
            defined(STM32G041G6Ux) || defined(STM32G041G8Ux) || defined(STM32G041J6Mx) || \
            defined(STM32G041K6Tx) || defined(STM32G041K8Tx) || defined(STM32G041K6Ux) || \
            defined(STM32G041K8Ux) || defined(STM32G041Y8Yx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 1;
                if ( port == GPIOA && pos == 10 )
                    return 1;
                if ( port == GPIOB && pos == 7 )
                    return 0;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 1;
                if ( port == GPIOA && pos == 15 )
                    return 1;
            }
        #endif
        #if defined(STM32G070CBTx) || defined(STM32G070KBTx) || defined(STM32G070RBTx) || \
            defined(STM32G071C6Tx) || defined(STM32G071C8Tx) || defined(STM32G071CBTx) || \
            defined(STM32G071C6Ux) || defined(STM32G071C8Ux) || defined(STM32G071CBUx) || \
            defined(STM32G071EBYx) || defined(STM32G071G6Ux) || defined(STM32G071G8Ux) || \
            defined(STM32G071GBUx) || defined(STM32G071G8UxN) || defined(STM32G071GBUxN) || \
            defined(STM32G071K6Tx) || defined(STM32G071K8Tx) || defined(STM32G071KBTx) || \
            defined(STM32G071K6Ux) || defined(STM32G071K8Ux) || defined(STM32G071KBUx) || \
            defined(STM32G071K8TxN) || defined(STM32G071KBTxN) || defined(STM32G071K8UxN) || \
            defined(STM32G071KBUxN) || defined(STM32G071R6Tx) || defined(STM32G071R8Tx) || \
            defined(STM32G071RBTx) || defined(STM32G071RBIx) || defined(STM32G081CBTx) || \
            defined(STM32G081CBUx) || defined(STM32G081EBYx) || defined(STM32G081GBUx) || \
            defined(STM32G081GBUxN) || defined(STM32G081KBTx) || defined(STM32G081KBTxN) || \
            defined(STM32G081KBUx) || defined(STM32G081KBUxN) || defined(STM32G081RBIx) || \
            defined(STM32G081RBTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 1;
                if ( port == GPIOA && pos == 10 )
                    return 1;
                if ( port == GPIOB && pos == 7 )
                    return 0;
                if ( port == GPIOC && pos == 5 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 1;
                if ( port == GPIOA && pos == 15 )
                    return 1;
                if ( port == GPIOD && pos == 6 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 0 )
                    return 4;
                if ( port == GPIOB && pos == 9 )
                    return 4;
                if ( port == GPIOB && pos == 11 )
                    return 4;
                if ( port == GPIOC && pos == 5 )
                    return 0;
                if ( port == GPIOC && pos == 11 )
                    return 0;
                if ( port == GPIOD && pos == 9 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 1 )
                    return 4;
                if ( port == GPIOC && pos == 11 )
                    return 1;
            }
        #endif
        #if defined(STM32G0B0CETx) || defined(STM32G0B0KETx) || defined(STM32G0B0RETx) || \
            defined(STM32G0B0VETx) || defined(STM32G0B1CCTx) || defined(STM32G0B1CETx) || \
            defined(STM32G0B1CCUx) || defined(STM32G0B1CEUx) || defined(STM32G0B1KCTx) || \
            defined(STM32G0B1KETx) || defined(STM32G0B1KCTxN) || defined(STM32G0B1KETxN) || \
            defined(STM32G0B1KCUx) || defined(STM32G0B1KEUx) || defined(STM32G0B1KCUxN) || \
            defined(STM32G0B1KEUxN) || defined(STM32G0B1MCTx) || defined(STM32G0B1METx) || \
            defined(STM32G0B1RCIx) || defined(STM32G0B1REIx) || defined(STM32G0B1RCTx) || \
            defined(STM32G0B1RETx) || defined(STM32G0B1VCIx) || defined(STM32G0B1VEIx) || \
            defined(STM32G0B1VCTx) || defined(STM32G0B1VETx) || defined(STM32G0C1CCTx) || \
            defined(STM32G0C1CETx) || defined(STM32G0C1CCUx) || defined(STM32G0C1CEUx) || \
            defined(STM32G0C1KCTx) || defined(STM32G0C1KETx) || defined(STM32G0C1KCTxN) || \
            defined(STM32G0C1KETxN) || defined(STM32G0C1KCUx) || defined(STM32G0C1KEUx) || \
            defined(STM32G0C1KCUxN) || defined(STM32G0C1KEUxN) || defined(STM32G0C1MCTx) || \
            defined(STM32G0C1METx) || defined(STM32G0C1RCIx) || defined(STM32G0C1REIx) || \
            defined(STM32G0C1RCTx) || defined(STM32G0C1RETx) || defined(STM32G0C1VCIx) || \
            defined(STM32G0C1VEIx) || defined(STM32G0C1VCTx) || defined(STM32G0C1VETx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 1;
                if ( port == GPIOA && pos == 10 )
                    return 1;
                if ( port == GPIOB && pos == 7 )
                    return 0;
                if ( port == GPIOC && pos == 5 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 1;
                if ( port == GPIOA && pos == 15 )
                    return 1;
                if ( port == GPIOD && pos == 6 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 0 )
                    return 4;
                if ( port == GPIOB && pos == 9 )
                    return 4;
                if ( port == GPIOB && pos == 11 )
                    return 4;
                if ( port == GPIOC && pos == 5 )
                    return 0;
                if ( port == GPIOC && pos == 11 )
                    return 0;
                if ( port == GPIOD && pos == 9 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 1 )
                    return 4;
                if ( port == GPIOC && pos == 11 )
                    return 1;
                if ( port == GPIOE && pos == 9 )
                    return 0;
            }
            if ( periph == USART5 ) {
                if ( port == GPIOB && pos == 1 )
                    return 8;
                if ( port == GPIOB && pos == 4 )
                    return 3;
                if ( port == GPIOD && pos == 2 )
                    return 3;
                if ( port == GPIOE && pos == 11 )
                    return 3;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 5 )
                    return 3;
                if ( port == GPIOB && pos == 9 )
                    return 8;
                if ( port == GPIOC && pos == 1 )
                    return 4;
                if ( port == GPIOF && pos == 10 )
                    return 3;
            }
        #endif
        assert( false && "Incorrect RX pin" );
        __builtin_trap();
    }

    int uartAlternateFunCTS( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if defined(STM32G030C6Tx) || defined(STM32G030C8Tx) || defined(STM32G030F6Px) || \
            defined(STM32G030J6Mx) || defined(STM32G030K6Tx) || defined(STM32G030K8Tx) || \
            defined(STM32G031C4Tx) || defined(STM32G031C6Tx) || defined(STM32G031C8Tx) || \
            defined(STM32G031C4Ux) || defined(STM32G031C6Ux) || defined(STM32G031C8Ux) || \
            defined(STM32G031F4Px) || defined(STM32G031F6Px) || defined(STM32G031F8Px) || \
            defined(STM32G031G4Ux) || defined(STM32G031G6Ux) || defined(STM32G031G8Ux) || \
            defined(STM32G031J4Mx) || defined(STM32G031J6Mx) || defined(STM32G031K4Tx) || \
            defined(STM32G031K6Tx) || defined(STM32G031K8Tx) || defined(STM32G031K4Ux) || \
            defined(STM32G031K6Ux) || defined(STM32G031K8Ux) || defined(STM32G031Y8Yx) || \
            defined(STM32G041C6Tx) || defined(STM32G041C8Tx) || defined(STM32G041C6Ux) || \
            defined(STM32G041C8Ux) || defined(STM32G041F6Px) || defined(STM32G041F8Px) || \
            defined(STM32G041G6Ux) || defined(STM32G041G8Ux) || defined(STM32G041J6Mx) || \
            defined(STM32G041K6Tx) || defined(STM32G041K8Tx) || defined(STM32G041K6Ux) || \
            defined(STM32G041K8Ux) || defined(STM32G041Y8Yx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 11 )
                    return 1;
                if ( port == GPIOB && pos == 4 )
                    return 4;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 0 )
                    return 1;
                if ( port == GPIOD && pos == 3 )
                    return 0;
            }
        #endif
        #if defined(STM32G070CBTx) || defined(STM32G070KBTx) || defined(STM32G070RBTx) || \
            defined(STM32G071C6Tx) || defined(STM32G071C8Tx) || defined(STM32G071CBTx) || \
            defined(STM32G071C6Ux) || defined(STM32G071C8Ux) || defined(STM32G071CBUx) || \
            defined(STM32G071EBYx) || defined(STM32G071G6Ux) || defined(STM32G071G8Ux) || \
            defined(STM32G071GBUx) || defined(STM32G071G8UxN) || defined(STM32G071GBUxN) || \
            defined(STM32G071K6Tx) || defined(STM32G071K8Tx) || defined(STM32G071KBTx) || \
            defined(STM32G071K6Ux) || defined(STM32G071K8Ux) || defined(STM32G071KBUx) || \
            defined(STM32G071K8TxN) || defined(STM32G071KBTxN) || defined(STM32G071K8UxN) || \
            defined(STM32G071KBUxN) || defined(STM32G071R6Tx) || defined(STM32G071R8Tx) || \
            defined(STM32G071RBTx) || defined(STM32G071RBIx) || defined(STM32G081CBTx) || \
            defined(STM32G081CBUx) || defined(STM32G081EBYx) || defined(STM32G081GBUx) || \
            defined(STM32G081GBUxN) || defined(STM32G081KBTx) || defined(STM32G081KBTxN) || \
            defined(STM32G081KBUx) || defined(STM32G081KBUxN) || defined(STM32G081RBIx) || \
            defined(STM32G081RBTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 11 )
                    return 1;
                if ( port == GPIOB && pos == 4 )
                    return 4;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 0 )
                    return 1;
                if ( port == GPIOD && pos == 3 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOA && pos == 6 )
                    return 4;
                if ( port == GPIOB && pos == 13 )
                    return 4;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOB && pos == 7 )
                    return 4;
            }
        #endif
        #if defined(STM32G0B0CETx) || defined(STM32G0B0KETx) || defined(STM32G0B0RETx) || \
            defined(STM32G0B0VETx) || defined(STM32G0B1CCTx) || defined(STM32G0B1CETx) || \
            defined(STM32G0B1CCUx) || defined(STM32G0B1CEUx) || defined(STM32G0B1KCTx) || \
            defined(STM32G0B1KETx) || defined(STM32G0B1KCTxN) || defined(STM32G0B1KETxN) || \
            defined(STM32G0B1KCUx) || defined(STM32G0B1KEUx) || defined(STM32G0B1KCUxN) || \
            defined(STM32G0B1KEUxN) || defined(STM32G0B1MCTx) || defined(STM32G0B1METx) || \
            defined(STM32G0B1RCIx) || defined(STM32G0B1REIx) || defined(STM32G0B1RCTx) || \
            defined(STM32G0B1RETx) || defined(STM32G0B1VCIx) || defined(STM32G0B1VEIx) || \
            defined(STM32G0B1VCTx) || defined(STM32G0B1VETx) || defined(STM32G0C1CCTx) || \
            defined(STM32G0C1CETx) || defined(STM32G0C1CCUx) || defined(STM32G0C1CEUx) || \
            defined(STM32G0C1KCTx) || defined(STM32G0C1KETx) || defined(STM32G0C1KCTxN) || \
            defined(STM32G0C1KETxN) || defined(STM32G0C1KCUx) || defined(STM32G0C1KEUx) || \
            defined(STM32G0C1KCUxN) || defined(STM32G0C1KEUxN) || defined(STM32G0C1MCTx) || \
            defined(STM32G0C1METx) || defined(STM32G0C1RCIx) || defined(STM32G0C1REIx) || \
            defined(STM32G0C1RCTx) || defined(STM32G0C1RETx) || defined(STM32G0C1VCIx) || \
            defined(STM32G0C1VEIx) || defined(STM32G0C1VCTx) || defined(STM32G0C1VETx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 11 )
                    return 1;
                if ( port == GPIOB && pos == 4 )
                    return 4;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 0 )
                    return 1;
                if ( port == GPIOD && pos == 3 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOA && pos == 6 )
                    return 4;
                if ( port == GPIOB && pos == 13 )
                    return 4;
                if ( port == GPIOD && pos == 11 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOB && pos == 7 )
                    return 4;
            }
            if ( periph == USART5 ) {
                if ( port == GPIOB && pos == 6 )
                    return 8;
                if ( port == GPIOD && pos == 5 )
                    return 3;
                if ( port == GPIOF && pos == 7 )
                    return 3;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 6 )
                    return 3;
                if ( port == GPIOB && pos == 15 )
                    return 8;
                if ( port == GPIOF && pos == 12 )
                    return 3;
            }
        #endif
        assert( false && "Incorrect CTS pin" );
        __builtin_trap();
    }

    int uartAlternateFunRTS( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if defined(STM32G030C6Tx) || defined(STM32G030C8Tx) || defined(STM32G030F6Px) || \
            defined(STM32G030J6Mx) || defined(STM32G030K6Tx) || defined(STM32G030K8Tx) || \
            defined(STM32G031C4Tx) || defined(STM32G031C6Tx) || defined(STM32G031C8Tx) || \
            defined(STM32G031C4Ux) || defined(STM32G031C6Ux) || defined(STM32G031C8Ux) || \
            defined(STM32G031F4Px) || defined(STM32G031F6Px) || defined(STM32G031F8Px) || \
            defined(STM32G031G4Ux) || defined(STM32G031G6Ux) || defined(STM32G031G8Ux) || \
            defined(STM32G031J4Mx) || defined(STM32G031J6Mx) || defined(STM32G031K4Tx) || \
            defined(STM32G031K6Tx) || defined(STM32G031K8Tx) || defined(STM32G031K4Ux) || \
            defined(STM32G031K6Ux) || defined(STM32G031K8Ux) || defined(STM32G031Y8Yx) || \
            defined(STM32G041C6Tx) || defined(STM32G041C8Tx) || defined(STM32G041C6Ux) || \
            defined(STM32G041C8Ux) || defined(STM32G041F6Px) || defined(STM32G041F8Px) || \
            defined(STM32G041G6Ux) || defined(STM32G041G8Ux) || defined(STM32G041J6Mx) || \
            defined(STM32G041K6Tx) || defined(STM32G041K8Tx) || defined(STM32G041K6Ux) || \
            defined(STM32G041K8Ux) || defined(STM32G041Y8Yx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
                if ( port == GPIOB && pos == 3 )
                    return 4;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
            }
        #endif
        #if defined(STM32G070CBTx) || defined(STM32G070KBTx) || defined(STM32G070RBTx) || \
            defined(STM32G071C6Tx) || defined(STM32G071C8Tx) || defined(STM32G071CBTx) || \
            defined(STM32G071C6Ux) || defined(STM32G071C8Ux) || defined(STM32G071CBUx) || \
            defined(STM32G071EBYx) || defined(STM32G071G6Ux) || defined(STM32G071G8Ux) || \
            defined(STM32G071GBUx) || defined(STM32G071G8UxN) || defined(STM32G071GBUxN) || \
            defined(STM32G071K6Tx) || defined(STM32G071K8Tx) || defined(STM32G071KBTx) || \
            defined(STM32G071K6Ux) || defined(STM32G071K8Ux) || defined(STM32G071KBUx) || \
            defined(STM32G071K8TxN) || defined(STM32G071KBTxN) || defined(STM32G071K8UxN) || \
            defined(STM32G071KBUxN) || defined(STM32G071R6Tx) || defined(STM32G071R8Tx) || \
            defined(STM32G071RBTx) || defined(STM32G071RBIx) || defined(STM32G081CBTx) || \
            defined(STM32G081CBUx) || defined(STM32G081EBYx) || defined(STM32G081GBUx) || \
            defined(STM32G081GBUxN) || defined(STM32G081KBTx) || defined(STM32G081KBTxN) || \
            defined(STM32G081KBUx) || defined(STM32G081KBUxN) || defined(STM32G081RBIx) || \
            defined(STM32G081RBTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
                if ( port == GPIOB && pos == 3 )
                    return 4;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
                if ( port == GPIOD && pos == 4 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOA && pos == 15 )
                    return 5;
                if ( port == GPIOB && pos == 1 )
                    return 4;
                if ( port == GPIOB && pos == 14 )
                    return 4;
                if ( port == GPIOD && pos == 2 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 4;
            }
        #endif
        #if defined(STM32G0B0CETx) || defined(STM32G0B0KETx) || defined(STM32G0B0RETx) || \
            defined(STM32G0B0VETx) || defined(STM32G0B1CCTx) || defined(STM32G0B1CETx) || \
            defined(STM32G0B1CCUx) || defined(STM32G0B1CEUx) || defined(STM32G0B1KCTx) || \
            defined(STM32G0B1KETx) || defined(STM32G0B1KCTxN) || defined(STM32G0B1KETxN) || \
            defined(STM32G0B1KCUx) || defined(STM32G0B1KEUx) || defined(STM32G0B1KCUxN) || \
            defined(STM32G0B1KEUxN) || defined(STM32G0B1MCTx) || defined(STM32G0B1METx) || \
            defined(STM32G0B1RCIx) || defined(STM32G0B1REIx) || defined(STM32G0B1RCTx) || \
            defined(STM32G0B1RETx) || defined(STM32G0B1VCIx) || defined(STM32G0B1VEIx) || \
            defined(STM32G0B1VCTx) || defined(STM32G0B1VETx) || defined(STM32G0C1CCTx) || \
            defined(STM32G0C1CETx) || defined(STM32G0C1CCUx) || defined(STM32G0C1CEUx) || \
            defined(STM32G0C1KCTx) || defined(STM32G0C1KETx) || defined(STM32G0C1KCTxN) || \
            defined(STM32G0C1KETxN) || defined(STM32G0C1KCUx) || defined(STM32G0C1KEUx) || \
            defined(STM32G0C1KCUxN) || defined(STM32G0C1KEUxN) || defined(STM32G0C1MCTx) || \
            defined(STM32G0C1METx) || defined(STM32G0C1RCIx) || defined(STM32G0C1REIx) || \
            defined(STM32G0C1RCTx) || defined(STM32G0C1RETx) || defined(STM32G0C1VCIx) || \
            defined(STM32G0C1VEIx) || defined(STM32G0C1VCTx) || defined(STM32G0C1VETx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
                if ( port == GPIOB && pos == 3 )
                    return 4;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
                if ( port == GPIOD && pos == 4 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOA && pos == 15 )
                    return 5;
                if ( port == GPIOB && pos == 1 )
                    return 4;
                if ( port == GPIOB && pos == 14 )
                    return 4;
                if ( port == GPIOD && pos == 2 )
                    return 0;
                if ( port == GPIOD && pos == 12 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 4;
            }
            if ( periph == USART5 ) {
                if ( port == GPIOB && pos == 5 )
                    return 8;
                if ( port == GPIOD && pos == 4 )
                    return 3;
                if ( port == GPIOE && pos == 7 )
                    return 3;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 7 )
                    return 3;
                if ( port == GPIOB && pos == 14 )
                    return 8;
                if ( port == GPIOF && pos == 3 )
                    return 3;
                if ( port == GPIOF && pos == 11 )
                    return 3;
            }
        #endif
        assert( false && "Incorrect RTS pin" );
        __builtin_trap();
    }

    int uartAlternateFunDE( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if defined(STM32G030C6Tx) || defined(STM32G030C8Tx) || defined(STM32G030F6Px) || \
            defined(STM32G030J6Mx) || defined(STM32G030K6Tx) || defined(STM32G030K8Tx) || \
            defined(STM32G031C4Tx) || defined(STM32G031C6Tx) || defined(STM32G031C8Tx) || \
            defined(STM32G031C4Ux) || defined(STM32G031C6Ux) || defined(STM32G031C8Ux) || \
            defined(STM32G031F4Px) || defined(STM32G031F6Px) || defined(STM32G031F8Px) || \
            defined(STM32G031G4Ux) || defined(STM32G031G6Ux) || defined(STM32G031G8Ux) || \
            defined(STM32G031J4Mx) || defined(STM32G031J6Mx) || defined(STM32G031K4Tx) || \
            defined(STM32G031K6Tx) || defined(STM32G031K8Tx) || defined(STM32G031K4Ux) || \
            defined(STM32G031K6Ux) || defined(STM32G031K8Ux) || defined(STM32G031Y8Yx) || \
            defined(STM32G041C6Tx) || defined(STM32G041C8Tx) || defined(STM32G041C6Ux) || \
            defined(STM32G041C8Ux) || defined(STM32G041F6Px) || defined(STM32G041F8Px) || \
            defined(STM32G041G6Ux) || defined(STM32G041G8Ux) || defined(STM32G041J6Mx) || \
            defined(STM32G041K6Tx) || defined(STM32G041K8Tx) || defined(STM32G041K6Ux) || \
            defined(STM32G041K8Ux) || defined(STM32G041Y8Yx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
                if ( port == GPIOB && pos == 3 )
                    return 4;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
            }
        #endif
        #if defined(STM32G070CBTx) || defined(STM32G070KBTx) || defined(STM32G070RBTx) || \
            defined(STM32G071C6Tx) || defined(STM32G071C8Tx) || defined(STM32G071CBTx) || \
            defined(STM32G071C6Ux) || defined(STM32G071C8Ux) || defined(STM32G071CBUx) || \
            defined(STM32G071EBYx) || defined(STM32G071G6Ux) || defined(STM32G071G8Ux) || \
            defined(STM32G071GBUx) || defined(STM32G071G8UxN) || defined(STM32G071GBUxN) || \
            defined(STM32G071K6Tx) || defined(STM32G071K8Tx) || defined(STM32G071KBTx) || \
            defined(STM32G071K6Ux) || defined(STM32G071K8Ux) || defined(STM32G071KBUx) || \
            defined(STM32G071K8TxN) || defined(STM32G071KBTxN) || defined(STM32G071K8UxN) || \
            defined(STM32G071KBUxN) || defined(STM32G071R6Tx) || defined(STM32G071R8Tx) || \
            defined(STM32G071RBTx) || defined(STM32G071RBIx) || defined(STM32G081CBTx) || \
            defined(STM32G081CBUx) || defined(STM32G081EBYx) || defined(STM32G081GBUx) || \
            defined(STM32G081GBUxN) || defined(STM32G081KBTx) || defined(STM32G081KBTxN) || \
            defined(STM32G081KBUx) || defined(STM32G081KBUxN) || defined(STM32G081RBIx) || \
            defined(STM32G081RBTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
                if ( port == GPIOB && pos == 3 )
                    return 4;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
                if ( port == GPIOD && pos == 4 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOA && pos == 15 )
                    return 5;
                if ( port == GPIOB && pos == 1 )
                    return 4;
                if ( port == GPIOB && pos == 14 )
                    return 4;
                if ( port == GPIOD && pos == 2 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 4;
            }
        #endif
        #if defined(STM32G0B0CETx) || defined(STM32G0B0KETx) || defined(STM32G0B0RETx) || \
            defined(STM32G0B0VETx) || defined(STM32G0B1CCTx) || defined(STM32G0B1CETx) || \
            defined(STM32G0B1CCUx) || defined(STM32G0B1CEUx) || defined(STM32G0B1KCTx) || \
            defined(STM32G0B1KETx) || defined(STM32G0B1KCTxN) || defined(STM32G0B1KETxN) || \
            defined(STM32G0B1KCUx) || defined(STM32G0B1KEUx) || defined(STM32G0B1KCUxN) || \
            defined(STM32G0B1KEUxN) || defined(STM32G0B1MCTx) || defined(STM32G0B1METx) || \
            defined(STM32G0B1RCIx) || defined(STM32G0B1REIx) || defined(STM32G0B1RCTx) || \
            defined(STM32G0B1RETx) || defined(STM32G0B1VCIx) || defined(STM32G0B1VEIx) || \
            defined(STM32G0B1VCTx) || defined(STM32G0B1VETx) || defined(STM32G0C1CCTx) || \
            defined(STM32G0C1CETx) || defined(STM32G0C1CCUx) || defined(STM32G0C1CEUx) || \
            defined(STM32G0C1KCTx) || defined(STM32G0C1KETx) || defined(STM32G0C1KCTxN) || \
            defined(STM32G0C1KETxN) || defined(STM32G0C1KCUx) || defined(STM32G0C1KEUx) || \
            defined(STM32G0C1KCUxN) || defined(STM32G0C1KEUxN) || defined(STM32G0C1MCTx) || \
            defined(STM32G0C1METx) || defined(STM32G0C1RCIx) || defined(STM32G0C1REIx) || \
            defined(STM32G0C1RCTx) || defined(STM32G0C1RETx) || defined(STM32G0C1VCIx) || \
            defined(STM32G0C1VEIx) || defined(STM32G0C1VCTx) || defined(STM32G0C1VETx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
                if ( port == GPIOB && pos == 3 )
                    return 4;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
                if ( port == GPIOD && pos == 4 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOA && pos == 15 )
                    return 5;
                if ( port == GPIOB && pos == 1 )
                    return 4;
                if ( port == GPIOB && pos == 14 )
                    return 4;
                if ( port == GPIOD && pos == 2 )
                    return 0;
                if ( port == GPIOD && pos == 12 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 4;
            }
            if ( periph == USART5 ) {
                if ( port == GPIOB && pos == 5 )
                    return 8;
                if ( port == GPIOD && pos == 4 )
                    return 3;
                if ( port == GPIOE && pos == 7 )
                    return 3;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 7 )
                    return 3;
                if ( port == GPIOB && pos == 14 )
                    return 8;
                if ( port == GPIOF && pos == 3 )
                    return 3;
                if ( port == GPIOF && pos == 11 )
                    return 3;
            }
        #endif
        assert( false && "Incorrect DE pin" );
        __builtin_trap();
    }

} // namespace detail

