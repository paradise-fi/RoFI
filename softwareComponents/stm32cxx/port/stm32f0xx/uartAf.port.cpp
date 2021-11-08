#include <stm32f0xx_ll_bus.h>
#include <stm32f0xx_ll_usart.h>
#include <stm32f0xx_ll_gpio.h>
#include <system/assert.hpp>

namespace detail {
    int uartAlternateFunTX( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if defined(STM32F030C6Tx) || defined(STM32F030F4Px) || defined(STM32F030K6Tx) || \
            defined(STM32F031C4Tx) || defined(STM32F031C6Tx) || defined(STM32F031E6Yx) || \
            defined(STM32F031F4Px) || defined(STM32F031F6Px) || defined(STM32F031G4Ux) || \
            defined(STM32F031G6Ux) || defined(STM32F031K4Ux) || defined(STM32F031K6Ux) || \
            defined(STM32F031K6Tx) || defined(STM32F038C6Tx) || defined(STM32F038E6Yx) || \
            defined(STM32F038F6Px) || defined(STM32F038G6Ux) || defined(STM32F038K6Ux)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 2 )
                    return 1;
                if ( port == GPIOA && pos == 9 )
                    return 1;
                if ( port == GPIOA && pos == 14 )
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
        #if defined(STM32F030C8Tx) || defined(STM32F030R8Tx) || defined(STM32F070C6Tx) || \
            defined(STM32F070F6Px) || defined(STM32F051C4Tx) || defined(STM32F051C4Ux) || \
            defined(STM32F051C6Tx) || defined(STM32F051C6Ux) || defined(STM32F051C8Tx) || \
            defined(STM32F051C8Ux) || defined(STM32F051K4Tx) || defined(STM32F051K4Ux) || \
            defined(STM32F051K6Tx) || defined(STM32F051K6Ux) || defined(STM32F051K8Tx) || \
            defined(STM32F051K8Ux) || defined(STM32F051R4Tx) || defined(STM32F051R6Tx) || \
            defined(STM32F051R8Hx) || defined(STM32F051R8Tx) || defined(STM32F051T8Yx) || \
            defined(STM32F042C4Tx) || defined(STM32F042C6Tx) || defined(STM32F042C4Ux) || \
            defined(STM32F042C6Ux) || defined(STM32F042F4Px) || defined(STM32F042F6Px) || \
            defined(STM32F042G4Ux) || defined(STM32F042G6Ux) || defined(STM32F042K4Tx) || \
            defined(STM32F042K6Tx) || defined(STM32F042K4Ux) || defined(STM32F042K6Ux) || \
            defined(STM32F042T6Yx) || defined(STM32F048C6Ux) || defined(STM32F048G6Ux) || \
            defined(STM32F048T6Yx) || defined(STM32F058C8Ux) || defined(STM32F058R8Hx) || \
            defined(STM32F058R8Tx) || defined(STM32F058T8Yx)
            if ( periph == USART1 ) {
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
        #if defined(STM32F030CCTx) || defined(STM32F030RCTx) || defined(STM32F091CBTx) || \
            defined(STM32F091CCTx) || defined(STM32F091CBUx) || defined(STM32F091CCUx) || \
            defined(STM32F091RBTx) || defined(STM32F091RCTx) || defined(STM32F091RCHx) || \
            defined(STM32F091RCYx) || defined(STM32F091VBTx) || defined(STM32F091VCTx) || \
            defined(STM32F091VCHx) || defined(STM32F098CCTx) || defined(STM32F098CCUx) || \
            defined(STM32F098RCHx) || defined(STM32F098RCTx) || defined(STM32F098RCYx) || \
            defined(STM32F098VCHx) || defined(STM32F098VCTx)
            if ( periph == USART1 ) {
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
                if ( port == GPIOD && pos == 5 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 10 )
                    return 4;
                if ( port == GPIOC && pos == 4 )
                    return 1;
                if ( port == GPIOC && pos == 10 )
                    return 1;
                if ( port == GPIOD && pos == 8 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 0 )
                    return 4;
                if ( port == GPIOC && pos == 10 )
                    return 0;
                if ( port == GPIOE && pos == 8 )
                    return 1;
            }
            if ( periph == USART5 ) {
                if ( port == GPIOB && pos == 3 )
                    return 4;
                if ( port == GPIOC && pos == 12 )
                    return 2;
                if ( port == GPIOE && pos == 10 )
                    return 1;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 4 )
                    return 5;
                if ( port == GPIOC && pos == 0 )
                    return 2;
                if ( port == GPIOF && pos == 9 )
                    return 1;
            }
            if ( periph == USART7 ) {
                if ( port == GPIOC && pos == 0 )
                    return 1;
                if ( port == GPIOC && pos == 6 )
                    return 1;
                if ( port == GPIOF && pos == 2 )
                    return 1;
            }
            if ( periph == USART8 ) {
                if ( port == GPIOC && pos == 2 )
                    return 2;
                if ( port == GPIOC && pos == 8 )
                    return 1;
                if ( port == GPIOD && pos == 13 )
                    return 0;
            }
        #endif
        #if defined(STM32F070CBTx) || defined(STM32F070RBTx) || defined(STM32F071C8Tx) || \
            defined(STM32F071CBTx) || defined(STM32F071C8Ux) || defined(STM32F071CBUx) || \
            defined(STM32F071CBYx) || defined(STM32F071RBTx) || defined(STM32F071V8Hx) || \
            defined(STM32F071VBHx) || defined(STM32F071V8Tx) || defined(STM32F071VBTx) || \
            defined(STM32F072C8Tx) || defined(STM32F072CBTx) || defined(STM32F072C8Ux) || \
            defined(STM32F072CBUx) || defined(STM32F072CBYx) || defined(STM32F072R8Tx) || \
            defined(STM32F072RBTx) || defined(STM32F072RBHx) || defined(STM32F072RBIx) || \
            defined(STM32F072V8Hx) || defined(STM32F072VBHx) || defined(STM32F072V8Tx) || \
            defined(STM32F072VBTx) || defined(STM32F078CBTx) || defined(STM32F078CBUx) || \
            defined(STM32F078CBYx) || defined(STM32F078RBHx) || defined(STM32F078RBTx) || \
            defined(STM32F078VBHx) || defined(STM32F078VBTx)
            if ( periph == USART1 ) {
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
                if ( port == GPIOD && pos == 5 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 10 )
                    return 4;
                if ( port == GPIOC && pos == 4 )
                    return 1;
                if ( port == GPIOC && pos == 10 )
                    return 1;
                if ( port == GPIOD && pos == 8 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 0 )
                    return 4;
                if ( port == GPIOC && pos == 10 )
                    return 0;
            }
        #endif
        assert( false && "Incorrect TX pin" );
        __builtin_trap();
    }

    int uartAlternateFunRX( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if defined(STM32F030C6Tx) || defined(STM32F030F4Px) || defined(STM32F030K6Tx) || \
            defined(STM32F031C4Tx) || defined(STM32F031C6Tx) || defined(STM32F031E6Yx) || \
            defined(STM32F031F4Px) || defined(STM32F031F6Px) || defined(STM32F031G4Ux) || \
            defined(STM32F031G6Ux) || defined(STM32F031K4Ux) || defined(STM32F031K6Ux) || \
            defined(STM32F031K6Tx) || defined(STM32F038C6Tx) || defined(STM32F038E6Yx) || \
            defined(STM32F038F6Px) || defined(STM32F038G6Ux) || defined(STM32F038K6Ux)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 3 )
                    return 1;
                if ( port == GPIOA && pos == 10 )
                    return 1;
                if ( port == GPIOA && pos == 15 )
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
        #if defined(STM32F030C8Tx) || defined(STM32F030R8Tx) || defined(STM32F070C6Tx) || \
            defined(STM32F070F6Px) || defined(STM32F051C4Tx) || defined(STM32F051C4Ux) || \
            defined(STM32F051C6Tx) || defined(STM32F051C6Ux) || defined(STM32F051C8Tx) || \
            defined(STM32F051C8Ux) || defined(STM32F051K4Tx) || defined(STM32F051K4Ux) || \
            defined(STM32F051K6Tx) || defined(STM32F051K6Ux) || defined(STM32F051K8Tx) || \
            defined(STM32F051K8Ux) || defined(STM32F051R4Tx) || defined(STM32F051R6Tx) || \
            defined(STM32F051R8Hx) || defined(STM32F051R8Tx) || defined(STM32F051T8Yx) || \
            defined(STM32F042C4Tx) || defined(STM32F042C6Tx) || defined(STM32F042C4Ux) || \
            defined(STM32F042C6Ux) || defined(STM32F042F4Px) || defined(STM32F042F6Px) || \
            defined(STM32F042G4Ux) || defined(STM32F042G6Ux) || defined(STM32F042K4Tx) || \
            defined(STM32F042K6Tx) || defined(STM32F042K4Ux) || defined(STM32F042K6Ux) || \
            defined(STM32F042T6Yx) || defined(STM32F048C6Ux) || defined(STM32F048G6Ux) || \
            defined(STM32F048T6Yx) || defined(STM32F058C8Ux) || defined(STM32F058R8Hx) || \
            defined(STM32F058R8Tx) || defined(STM32F058T8Yx)
            if ( periph == USART1 ) {
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
        #if defined(STM32F030CCTx) || defined(STM32F030RCTx) || defined(STM32F091CBTx) || \
            defined(STM32F091CCTx) || defined(STM32F091CBUx) || defined(STM32F091CCUx) || \
            defined(STM32F091RBTx) || defined(STM32F091RCTx) || defined(STM32F091RCHx) || \
            defined(STM32F091RCYx) || defined(STM32F091VBTx) || defined(STM32F091VCTx) || \
            defined(STM32F091VCHx) || defined(STM32F098CCTx) || defined(STM32F098CCUx) || \
            defined(STM32F098RCHx) || defined(STM32F098RCTx) || defined(STM32F098RCYx) || \
            defined(STM32F098VCHx) || defined(STM32F098VCTx)
            if ( periph == USART1 ) {
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
                if ( port == GPIOD && pos == 6 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 11 )
                    return 4;
                if ( port == GPIOC && pos == 5 )
                    return 1;
                if ( port == GPIOC && pos == 11 )
                    return 1;
                if ( port == GPIOD && pos == 9 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 1 )
                    return 4;
                if ( port == GPIOC && pos == 11 )
                    return 0;
                if ( port == GPIOE && pos == 9 )
                    return 1;
            }
            if ( periph == USART5 ) {
                if ( port == GPIOB && pos == 4 )
                    return 4;
                if ( port == GPIOD && pos == 2 )
                    return 2;
                if ( port == GPIOE && pos == 11 )
                    return 1;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 5 )
                    return 5;
                if ( port == GPIOC && pos == 1 )
                    return 2;
                if ( port == GPIOF && pos == 10 )
                    return 1;
            }
            if ( periph == USART7 ) {
                if ( port == GPIOC && pos == 1 )
                    return 1;
                if ( port == GPIOC && pos == 7 )
                    return 1;
                if ( port == GPIOF && pos == 3 )
                    return 1;
            }
            if ( periph == USART8 ) {
                if ( port == GPIOC && pos == 3 )
                    return 2;
                if ( port == GPIOC && pos == 9 )
                    return 1;
                if ( port == GPIOD && pos == 14 )
                    return 0;
            }
        #endif
        #if defined(STM32F070CBTx) || defined(STM32F070RBTx) || defined(STM32F071C8Tx) || \
            defined(STM32F071CBTx) || defined(STM32F071C8Ux) || defined(STM32F071CBUx) || \
            defined(STM32F071CBYx) || defined(STM32F071RBTx) || defined(STM32F071V8Hx) || \
            defined(STM32F071VBHx) || defined(STM32F071V8Tx) || defined(STM32F071VBTx) || \
            defined(STM32F072C8Tx) || defined(STM32F072CBTx) || defined(STM32F072C8Ux) || \
            defined(STM32F072CBUx) || defined(STM32F072CBYx) || defined(STM32F072R8Tx) || \
            defined(STM32F072RBTx) || defined(STM32F072RBHx) || defined(STM32F072RBIx) || \
            defined(STM32F072V8Hx) || defined(STM32F072VBHx) || defined(STM32F072V8Tx) || \
            defined(STM32F072VBTx) || defined(STM32F078CBTx) || defined(STM32F078CBUx) || \
            defined(STM32F078CBYx) || defined(STM32F078RBHx) || defined(STM32F078RBTx) || \
            defined(STM32F078VBHx) || defined(STM32F078VBTx)
            if ( periph == USART1 ) {
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
                if ( port == GPIOD && pos == 6 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 11 )
                    return 4;
                if ( port == GPIOC && pos == 5 )
                    return 1;
                if ( port == GPIOC && pos == 11 )
                    return 1;
                if ( port == GPIOD && pos == 9 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 1 )
                    return 4;
                if ( port == GPIOC && pos == 11 )
                    return 0;
            }
        #endif
        assert( false && "Incorrect RX pin" );
        __builtin_trap();
    }

    int uartAlternateFunCTS( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if defined(STM32F030C6Tx) || defined(STM32F030F4Px) || defined(STM32F030K6Tx) || \
            defined(STM32F031C4Tx) || defined(STM32F031C6Tx) || defined(STM32F031E6Yx) || \
            defined(STM32F031F4Px) || defined(STM32F031F6Px) || defined(STM32F031G4Ux) || \
            defined(STM32F031G6Ux) || defined(STM32F031K4Ux) || defined(STM32F031K6Ux) || \
            defined(STM32F031K6Tx) || defined(STM32F038C6Tx) || defined(STM32F038E6Yx) || \
            defined(STM32F038F6Px) || defined(STM32F038G6Ux) || defined(STM32F038K6Ux)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 0 )
                    return 1;
                if ( port == GPIOA && pos == 11 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 0 )
                    return 1;
            }
        #endif
        #if defined(STM32F030C8Tx) || defined(STM32F030R8Tx) || defined(STM32F070C6Tx) || \
            defined(STM32F070F6Px) || defined(STM32F051C4Tx) || defined(STM32F051C4Ux) || \
            defined(STM32F051C6Tx) || defined(STM32F051C6Ux) || defined(STM32F051C8Tx) || \
            defined(STM32F051C8Ux) || defined(STM32F051K4Tx) || defined(STM32F051K4Ux) || \
            defined(STM32F051K6Tx) || defined(STM32F051K6Ux) || defined(STM32F051K8Tx) || \
            defined(STM32F051K8Ux) || defined(STM32F051R4Tx) || defined(STM32F051R6Tx) || \
            defined(STM32F051R8Hx) || defined(STM32F051R8Tx) || defined(STM32F051T8Yx) || \
            defined(STM32F042C4Tx) || defined(STM32F042C6Tx) || defined(STM32F042C4Ux) || \
            defined(STM32F042C6Ux) || defined(STM32F042F4Px) || defined(STM32F042F6Px) || \
            defined(STM32F042G4Ux) || defined(STM32F042G6Ux) || defined(STM32F042K4Tx) || \
            defined(STM32F042K6Tx) || defined(STM32F042K4Ux) || defined(STM32F042K6Ux) || \
            defined(STM32F042T6Yx) || defined(STM32F048C6Ux) || defined(STM32F048G6Ux) || \
            defined(STM32F048T6Yx) || defined(STM32F058C8Ux) || defined(STM32F058R8Hx) || \
            defined(STM32F058R8Tx) || defined(STM32F058T8Yx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 11 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 0 )
                    return 1;
            }
        #endif
        #if defined(STM32F030CCTx) || defined(STM32F030RCTx) || defined(STM32F070CBTx) || \
            defined(STM32F070RBTx) || defined(STM32F071C8Tx) || defined(STM32F071CBTx) || \
            defined(STM32F071C8Ux) || defined(STM32F071CBUx) || defined(STM32F071CBYx) || \
            defined(STM32F071RBTx) || defined(STM32F071V8Hx) || defined(STM32F071VBHx) || \
            defined(STM32F071V8Tx) || defined(STM32F071VBTx) || defined(STM32F091CBTx) || \
            defined(STM32F091CCTx) || defined(STM32F091CBUx) || defined(STM32F091CCUx) || \
            defined(STM32F091RBTx) || defined(STM32F091RCTx) || defined(STM32F091RCHx) || \
            defined(STM32F091RCYx) || defined(STM32F091VBTx) || defined(STM32F091VCTx) || \
            defined(STM32F091VCHx) || defined(STM32F072C8Tx) || defined(STM32F072CBTx) || \
            defined(STM32F072C8Ux) || defined(STM32F072CBUx) || defined(STM32F072CBYx) || \
            defined(STM32F072R8Tx) || defined(STM32F072RBTx) || defined(STM32F072RBHx) || \
            defined(STM32F072RBIx) || defined(STM32F072V8Hx) || defined(STM32F072VBHx) || \
            defined(STM32F072V8Tx) || defined(STM32F072VBTx) || defined(STM32F078CBTx) || \
            defined(STM32F078CBUx) || defined(STM32F078CBYx) || defined(STM32F078RBHx) || \
            defined(STM32F078RBTx) || defined(STM32F078VBHx) || defined(STM32F078VBTx) || \
            defined(STM32F098CCTx) || defined(STM32F098CCUx) || defined(STM32F098RCHx) || \
            defined(STM32F098RCTx) || defined(STM32F098RCYx) || defined(STM32F098VCHx) || \
            defined(STM32F098VCTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 11 )
                    return 1;
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
        #endif
        assert( false && "Incorrect CTS pin" );
        __builtin_trap();
    }

    int uartAlternateFunRTS( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if defined(STM32F030C6Tx) || defined(STM32F030F4Px) || defined(STM32F030K6Tx) || \
            defined(STM32F031C4Tx) || defined(STM32F031C6Tx) || defined(STM32F031E6Yx) || \
            defined(STM32F031F4Px) || defined(STM32F031F6Px) || defined(STM32F031G4Ux) || \
            defined(STM32F031G6Ux) || defined(STM32F031K4Ux) || defined(STM32F031K6Ux) || \
            defined(STM32F031K6Tx) || defined(STM32F038C6Tx) || defined(STM32F038E6Yx) || \
            defined(STM32F038F6Px) || defined(STM32F038G6Ux) || defined(STM32F038K6Ux)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
                if ( port == GPIOA && pos == 12 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
            }
        #endif
        #if defined(STM32F030C8Tx) || defined(STM32F030R8Tx) || defined(STM32F070C6Tx) || \
            defined(STM32F070F6Px) || defined(STM32F051C4Tx) || defined(STM32F051C4Ux) || \
            defined(STM32F051C6Tx) || defined(STM32F051C6Ux) || defined(STM32F051C8Tx) || \
            defined(STM32F051C8Ux) || defined(STM32F051K4Tx) || defined(STM32F051K4Ux) || \
            defined(STM32F051K6Tx) || defined(STM32F051K6Ux) || defined(STM32F051K8Tx) || \
            defined(STM32F051K8Ux) || defined(STM32F051R4Tx) || defined(STM32F051R6Tx) || \
            defined(STM32F051R8Hx) || defined(STM32F051R8Tx) || defined(STM32F051T8Yx) || \
            defined(STM32F042C4Tx) || defined(STM32F042C6Tx) || defined(STM32F042C4Ux) || \
            defined(STM32F042C6Ux) || defined(STM32F042F4Px) || defined(STM32F042F6Px) || \
            defined(STM32F042G4Ux) || defined(STM32F042G6Ux) || defined(STM32F042K4Tx) || \
            defined(STM32F042K6Tx) || defined(STM32F042K4Ux) || defined(STM32F042K6Ux) || \
            defined(STM32F042T6Yx) || defined(STM32F048C6Ux) || defined(STM32F048G6Ux) || \
            defined(STM32F048T6Yx) || defined(STM32F058C8Ux) || defined(STM32F058R8Hx) || \
            defined(STM32F058R8Tx) || defined(STM32F058T8Yx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
            }
        #endif
        #if defined(STM32F030CCTx) || defined(STM32F030RCTx) || defined(STM32F091CBTx) || \
            defined(STM32F091CCTx) || defined(STM32F091CBUx) || defined(STM32F091CCUx) || \
            defined(STM32F091RBTx) || defined(STM32F091RCTx) || defined(STM32F091RCHx) || \
            defined(STM32F091RCYx) || defined(STM32F091VBTx) || defined(STM32F091VCTx) || \
            defined(STM32F091VCHx) || defined(STM32F098CCTx) || defined(STM32F098CCUx) || \
            defined(STM32F098RCHx) || defined(STM32F098RCTx) || defined(STM32F098RCYx) || \
            defined(STM32F098VCHx) || defined(STM32F098VCTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
                if ( port == GPIOD && pos == 4 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 1 )
                    return 4;
                if ( port == GPIOB && pos == 14 )
                    return 4;
                if ( port == GPIOD && pos == 2 )
                    return 1;
                if ( port == GPIOD && pos == 12 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 4;
            }
            if ( periph == USART5 ) {
                if ( port == GPIOB && pos == 5 )
                    return 4;
                if ( port == GPIOE && pos == 7 )
                    return 1;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOF && pos == 3 )
                    return 2;
            }
            if ( periph == USART7 ) {
                if ( port == GPIOD && pos == 15 )
                    return 2;
                if ( port == GPIOF && pos == 2 )
                    return 2;
            }
            if ( periph == USART8 ) {
                if ( port == GPIOD && pos == 12 )
                    return 2;
            }
        #endif
        #if defined(STM32F070CBTx) || defined(STM32F070RBTx) || defined(STM32F071C8Tx) || \
            defined(STM32F071CBTx) || defined(STM32F071C8Ux) || defined(STM32F071CBUx) || \
            defined(STM32F071CBYx) || defined(STM32F071RBTx) || defined(STM32F071V8Hx) || \
            defined(STM32F071VBHx) || defined(STM32F071V8Tx) || defined(STM32F071VBTx) || \
            defined(STM32F072C8Tx) || defined(STM32F072CBTx) || defined(STM32F072C8Ux) || \
            defined(STM32F072CBUx) || defined(STM32F072CBYx) || defined(STM32F072R8Tx) || \
            defined(STM32F072RBTx) || defined(STM32F072RBHx) || defined(STM32F072RBIx) || \
            defined(STM32F072V8Hx) || defined(STM32F072VBHx) || defined(STM32F072V8Tx) || \
            defined(STM32F072VBTx) || defined(STM32F078CBTx) || defined(STM32F078CBUx) || \
            defined(STM32F078CBYx) || defined(STM32F078RBHx) || defined(STM32F078RBTx) || \
            defined(STM32F078VBHx) || defined(STM32F078VBTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
                if ( port == GPIOD && pos == 4 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 1 )
                    return 4;
                if ( port == GPIOB && pos == 14 )
                    return 4;
                if ( port == GPIOD && pos == 2 )
                    return 1;
                if ( port == GPIOD && pos == 12 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 4;
            }
        #endif
        assert( false && "Incorrect RTS pin" );
        __builtin_trap();
    }

    int uartAlternateFunDE( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if defined(STM32F030C6Tx) || defined(STM32F030F4Px) || defined(STM32F030K6Tx) || \
            defined(STM32F031C4Tx) || defined(STM32F031C6Tx) || defined(STM32F031E6Yx) || \
            defined(STM32F031F4Px) || defined(STM32F031F6Px) || defined(STM32F031G4Ux) || \
            defined(STM32F031G6Ux) || defined(STM32F031K4Ux) || defined(STM32F031K6Ux) || \
            defined(STM32F031K6Tx) || defined(STM32F038C6Tx) || defined(STM32F038E6Yx) || \
            defined(STM32F038F6Px) || defined(STM32F038G6Ux) || defined(STM32F038K6Ux)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
                if ( port == GPIOA && pos == 12 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
            }
        #endif
        #if defined(STM32F030C8Tx) || defined(STM32F030R8Tx) || defined(STM32F070C6Tx) || \
            defined(STM32F070F6Px) || defined(STM32F051C4Tx) || defined(STM32F051C4Ux) || \
            defined(STM32F051C6Tx) || defined(STM32F051C6Ux) || defined(STM32F051C8Tx) || \
            defined(STM32F051C8Ux) || defined(STM32F051K4Tx) || defined(STM32F051K4Ux) || \
            defined(STM32F051K6Tx) || defined(STM32F051K6Ux) || defined(STM32F051K8Tx) || \
            defined(STM32F051K8Ux) || defined(STM32F051R4Tx) || defined(STM32F051R6Tx) || \
            defined(STM32F051R8Hx) || defined(STM32F051R8Tx) || defined(STM32F051T8Yx) || \
            defined(STM32F042C4Tx) || defined(STM32F042C6Tx) || defined(STM32F042C4Ux) || \
            defined(STM32F042C6Ux) || defined(STM32F042F4Px) || defined(STM32F042F6Px) || \
            defined(STM32F042G4Ux) || defined(STM32F042G6Ux) || defined(STM32F042K4Tx) || \
            defined(STM32F042K6Tx) || defined(STM32F042K4Ux) || defined(STM32F042K6Ux) || \
            defined(STM32F042T6Yx) || defined(STM32F048C6Ux) || defined(STM32F048G6Ux) || \
            defined(STM32F048T6Yx) || defined(STM32F058C8Ux) || defined(STM32F058R8Hx) || \
            defined(STM32F058R8Tx) || defined(STM32F058T8Yx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
            }
        #endif
        #if defined(STM32F030CCTx) || defined(STM32F030RCTx) || defined(STM32F091CBTx) || \
            defined(STM32F091CCTx) || defined(STM32F091CBUx) || defined(STM32F091CCUx) || \
            defined(STM32F091RBTx) || defined(STM32F091RCTx) || defined(STM32F091RCHx) || \
            defined(STM32F091RCYx) || defined(STM32F091VBTx) || defined(STM32F091VCTx) || \
            defined(STM32F091VCHx) || defined(STM32F098CCTx) || defined(STM32F098CCUx) || \
            defined(STM32F098RCHx) || defined(STM32F098RCTx) || defined(STM32F098RCYx) || \
            defined(STM32F098VCHx) || defined(STM32F098VCTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
                if ( port == GPIOD && pos == 4 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 1 )
                    return 4;
                if ( port == GPIOB && pos == 14 )
                    return 4;
                if ( port == GPIOD && pos == 2 )
                    return 1;
                if ( port == GPIOD && pos == 12 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 4;
            }
            if ( periph == USART5 ) {
                if ( port == GPIOB && pos == 5 )
                    return 4;
                if ( port == GPIOE && pos == 7 )
                    return 1;
            }
        #endif
        #if defined(STM32F070CBTx) || defined(STM32F070RBTx) || defined(STM32F071C8Tx) || \
            defined(STM32F071CBTx) || defined(STM32F071C8Ux) || defined(STM32F071CBUx) || \
            defined(STM32F071CBYx) || defined(STM32F071RBTx) || defined(STM32F071V8Hx) || \
            defined(STM32F071VBHx) || defined(STM32F071V8Tx) || defined(STM32F071VBTx) || \
            defined(STM32F072C8Tx) || defined(STM32F072CBTx) || defined(STM32F072C8Ux) || \
            defined(STM32F072CBUx) || defined(STM32F072CBYx) || defined(STM32F072R8Tx) || \
            defined(STM32F072RBTx) || defined(STM32F072RBHx) || defined(STM32F072RBIx) || \
            defined(STM32F072V8Hx) || defined(STM32F072VBHx) || defined(STM32F072V8Tx) || \
            defined(STM32F072VBTx) || defined(STM32F078CBTx) || defined(STM32F078CBUx) || \
            defined(STM32F078CBYx) || defined(STM32F078RBHx) || defined(STM32F078RBTx) || \
            defined(STM32F078VBHx) || defined(STM32F078VBTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 1;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 1;
                if ( port == GPIOD && pos == 4 )
                    return 0;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 1 )
                    return 4;
                if ( port == GPIOB && pos == 14 )
                    return 4;
                if ( port == GPIOD && pos == 2 )
                    return 1;
                if ( port == GPIOD && pos == 12 )
                    return 0;
            }
            if ( periph == USART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 4;
            }
        #endif
        assert( false && "Incorrect DE pin" );
        __builtin_trap();
    }

} // namespace detail

