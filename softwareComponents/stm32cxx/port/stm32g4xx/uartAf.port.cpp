#include <stm32g4xx_ll_bus.h>
#include <stm32g4xx_ll_usart.h>
#include <stm32g4xx_ll_gpio.h>
#include <system/assert.hpp>

namespace detail {
    int uartAlternateFunTX( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if !defined(STM32G431C6Tx) && !defined(STM32G431C8Tx) && !defined(STM32G431CBTx) && \
            !defined(STM32G431C6Ux) && !defined(STM32G431C8Ux) && !defined(STM32G431CBUx) && \
            !defined(STM32G431CBYx) && !defined(STM32G431K6Tx) && !defined(STM32G431K8Tx) && \
            !defined(STM32G431KBTx) && !defined(STM32G431K6Ux) && !defined(STM32G431K8Ux) && \
            !defined(STM32G431KBUx) && !defined(STM32G431M6Tx) && !defined(STM32G431M8Tx) && \
            !defined(STM32G431MBTx) && !defined(STM32G431R6Ix) && !defined(STM32G431R8Ix) && \
            !defined(STM32G431RBIx) && !defined(STM32G431R6Tx) && !defined(STM32G431R8Tx) && \
            !defined(STM32G431RBTx) && !defined(STM32G431V6Tx) && !defined(STM32G431V8Tx) && \
            !defined(STM32G431VBTx) && !defined(STM32G441CBTx) && !defined(STM32G441CBUx) && \
            !defined(STM32G441CBYx) && !defined(STM32G441KBTx) && !defined(STM32G441KBUx) && \
            !defined(STM32G441MBTx) && !defined(STM32G441RBIx) && !defined(STM32G441RBTx) && \
            !defined(STM32G441VBTx) && !defined(STM32G471CCTx) && !defined(STM32G471CETx) && \
            !defined(STM32G471CCUx) && !defined(STM32G471CEUx) && !defined(STM32G471MCTx) && \
            !defined(STM32G471METx) && !defined(STM32G471MEYx) && !defined(STM32G471QCTx) && \
            !defined(STM32G471QETx) && !defined(STM32G471RCTx) && !defined(STM32G471RETx) && \
            !defined(STM32G471VCHx) && !defined(STM32G471VEHx) && !defined(STM32G471VCIx) && \
            !defined(STM32G471VEIx) && !defined(STM32G471VCTx) && !defined(STM32G471VETx) && \
            !defined(STM32G491CCTx) && !defined(STM32G491CETx) && !defined(STM32G491CCUx) && \
            !defined(STM32G491CEUx) && !defined(STM32G491KCUx) && !defined(STM32G491KEUx) && \
            !defined(STM32G491MCSx) && !defined(STM32G491MESx) && !defined(STM32G491MCTx) && \
            !defined(STM32G491METx) && !defined(STM32G491RCIx) && !defined(STM32G491REIx) && \
            !defined(STM32G491RCTx) && !defined(STM32G491RETx) && !defined(STM32G491REYx) && \
            !defined(STM32G491VCTx) && !defined(STM32G491VETx) && !defined(STM32G4A1CETx) && \
            !defined(STM32G4A1CEUx) && !defined(STM32G4A1KEUx) && !defined(STM32G4A1MESx) && \
            !defined(STM32G4A1METx) && !defined(STM32G4A1REIx) && !defined(STM32G4A1RETx) && \
            !defined(STM32G4A1REYx) && !defined(STM32G4A1VETx) && !defined(STM32G473CBTx) && \
            !defined(STM32G473CCTx) && !defined(STM32G473CETx) && !defined(STM32G473CBUx) && \
            !defined(STM32G473CCUx) && !defined(STM32G473CEUx) && !defined(STM32G473MBTx) && \
            !defined(STM32G473MCTx) && !defined(STM32G473METx) && !defined(STM32G473MEYx) && \
            !defined(STM32G473PBIx) && !defined(STM32G473PCIx) && !defined(STM32G473PEIx) && \
            !defined(STM32G473QBTx) && !defined(STM32G473QCTx) && !defined(STM32G473QETx) && \
            !defined(STM32G473RBTx) && !defined(STM32G473RCTx) && !defined(STM32G473RETx) && \
            !defined(STM32G473VBHx) && !defined(STM32G473VCHx) && !defined(STM32G473VEHx) && \
            !defined(STM32G473VBIx) && !defined(STM32G473VCIx) && !defined(STM32G473VEIx) && \
            !defined(STM32G473VBTx) && !defined(STM32G473VCTx) && !defined(STM32G473VETx) && \
            !defined(STM32G483CETx) && !defined(STM32G483CEUx) && !defined(STM32G483METx) && \
            !defined(STM32G483MEYx) && !defined(STM32G483PEIx) && !defined(STM32G483QETx) && \
            !defined(STM32G483RETx) && !defined(STM32G483VEHx) && !defined(STM32G483VEIx) && \
            !defined(STM32G483VETx) && !defined(STM32G474CBTx) && !defined(STM32G474CCTx) && \
            !defined(STM32G474CETx) && !defined(STM32G474CBUx) && !defined(STM32G474CCUx) && \
            !defined(STM32G474CEUx) && !defined(STM32G474MBTx) && !defined(STM32G474MCTx) && \
            !defined(STM32G474METx) && !defined(STM32G474MEYx) && !defined(STM32G474PBIx) && \
            !defined(STM32G474PCIx) && !defined(STM32G474PEIx) && !defined(STM32G474QBTx) && \
            !defined(STM32G474QCTx) && !defined(STM32G474QETx) && !defined(STM32G474RBTx) && \
            !defined(STM32G474RCTx) && !defined(STM32G474RETx) && !defined(STM32G474VBHx) && \
            !defined(STM32G474VCHx) && !defined(STM32G474VEHx) && !defined(STM32G474VBIx) && \
            !defined(STM32G474VCIx) && !defined(STM32G474VEIx) && !defined(STM32G474VBTx) && \
            !defined(STM32G474VCTx) && !defined(STM32G474VETx) && !defined(STM32G484CETx) && \
            !defined(STM32G484CEUx) && !defined(STM32G484METx) && !defined(STM32G484MEYx) && \
            !defined(STM32G484PEIx) && !defined(STM32G484QETx) && !defined(STM32G484RETx) && \
            !defined(STM32G484VEHx) && !defined(STM32G484VEIx) && !defined(STM32G484VETx)
                #error Invalid MCU specified
        #endif
        #if defined(STM32G431C6Tx) || defined(STM32G431C8Tx) || defined(STM32G431CBTx) || \
            defined(STM32G431C6Ux) || defined(STM32G431C8Ux) || defined(STM32G431CBUx) || \
            defined(STM32G431CBYx) || defined(STM32G431K6Tx) || defined(STM32G431K8Tx) || \
            defined(STM32G431KBTx) || defined(STM32G431K6Ux) || defined(STM32G431K8Ux) || \
            defined(STM32G431KBUx) || defined(STM32G431M6Tx) || defined(STM32G431M8Tx) || \
            defined(STM32G431MBTx) || defined(STM32G431R6Ix) || defined(STM32G431R8Ix) || \
            defined(STM32G431RBIx) || defined(STM32G431R6Tx) || defined(STM32G431R8Tx) || \
            defined(STM32G431RBTx) || defined(STM32G431V6Tx) || defined(STM32G431V8Tx) || \
            defined(STM32G431VBTx) || defined(STM32G441CBTx) || defined(STM32G441CBUx) || \
            defined(STM32G441CBYx) || defined(STM32G441KBTx) || defined(STM32G441KBUx) || \
            defined(STM32G441MBTx) || defined(STM32G441RBIx) || defined(STM32G441RBTx) || \
            defined(STM32G441VBTx)
            if ( periph == UART4 ) {
                if ( port == GPIOC && pos == 10 )
                    return 5;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 7;
                if ( port == GPIOB && pos == 6 )
                    return 7;
                if ( port == GPIOC && pos == 4 )
                    return 7;
                if ( port == GPIOE && pos == 0 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 7;
                if ( port == GPIOA && pos == 14 )
                    return 7;
                if ( port == GPIOB && pos == 3 )
                    return 7;
                if ( port == GPIOD && pos == 5 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 9 )
                    return 7;
                if ( port == GPIOB && pos == 10 )
                    return 7;
                if ( port == GPIOC && pos == 10 )
                    return 7;
                if ( port == GPIOD && pos == 8 )
                    return 7;
            }
        #endif
        #if defined(STM32G471CCTx) || defined(STM32G471CETx) || defined(STM32G471CCUx) || \
            defined(STM32G471CEUx) || defined(STM32G471MCTx) || defined(STM32G471METx) || \
            defined(STM32G471MEYx) || defined(STM32G471QCTx) || defined(STM32G471QETx) || \
            defined(STM32G471RCTx) || defined(STM32G471RETx) || defined(STM32G471VCHx) || \
            defined(STM32G471VEHx) || defined(STM32G471VCIx) || defined(STM32G471VEIx) || \
            defined(STM32G471VCTx) || defined(STM32G471VETx) || defined(STM32G473CBTx) || \
            defined(STM32G473CCTx) || defined(STM32G473CETx) || defined(STM32G473CBUx) || \
            defined(STM32G473CCUx) || defined(STM32G473CEUx) || defined(STM32G473MBTx) || \
            defined(STM32G473MCTx) || defined(STM32G473METx) || defined(STM32G473MEYx) || \
            defined(STM32G473PBIx) || defined(STM32G473PCIx) || defined(STM32G473PEIx) || \
            defined(STM32G473QBTx) || defined(STM32G473QCTx) || defined(STM32G473QETx) || \
            defined(STM32G473RBTx) || defined(STM32G473RCTx) || defined(STM32G473RETx) || \
            defined(STM32G473VBHx) || defined(STM32G473VCHx) || defined(STM32G473VEHx) || \
            defined(STM32G473VBIx) || defined(STM32G473VCIx) || defined(STM32G473VEIx) || \
            defined(STM32G473VBTx) || defined(STM32G473VCTx) || defined(STM32G473VETx) || \
            defined(STM32G483CETx) || defined(STM32G483CEUx) || defined(STM32G483METx) || \
            defined(STM32G483MEYx) || defined(STM32G483PEIx) || defined(STM32G483QETx) || \
            defined(STM32G483RETx) || defined(STM32G483VEHx) || defined(STM32G483VEIx) || \
            defined(STM32G483VETx) || defined(STM32G474CBTx) || defined(STM32G474CCTx) || \
            defined(STM32G474CETx) || defined(STM32G474CBUx) || defined(STM32G474CCUx) || \
            defined(STM32G474CEUx) || defined(STM32G474MBTx) || defined(STM32G474MCTx) || \
            defined(STM32G474METx) || defined(STM32G474MEYx) || defined(STM32G474PBIx) || \
            defined(STM32G474PCIx) || defined(STM32G474PEIx) || defined(STM32G474QBTx) || \
            defined(STM32G474QCTx) || defined(STM32G474QETx) || defined(STM32G474RBTx) || \
            defined(STM32G474RCTx) || defined(STM32G474RETx) || defined(STM32G474VBHx) || \
            defined(STM32G474VCHx) || defined(STM32G474VEHx) || defined(STM32G474VBIx) || \
            defined(STM32G474VCIx) || defined(STM32G474VEIx) || defined(STM32G474VBTx) || \
            defined(STM32G474VCTx) || defined(STM32G474VETx) || defined(STM32G484CETx) || \
            defined(STM32G484CEUx) || defined(STM32G484METx) || defined(STM32G484MEYx) || \
            defined(STM32G484PEIx) || defined(STM32G484QETx) || defined(STM32G484RETx) || \
            defined(STM32G484VEHx) || defined(STM32G484VEIx) || defined(STM32G484VETx)
            if ( periph == UART4 ) {
                if ( port == GPIOC && pos == 10 )
                    return 5;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOC && pos == 12 )
                    return 5;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 7;
                if ( port == GPIOB && pos == 6 )
                    return 7;
                if ( port == GPIOC && pos == 4 )
                    return 7;
                if ( port == GPIOE && pos == 0 )
                    return 7;
                if ( port == GPIOG && pos == 9 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 7;
                if ( port == GPIOA && pos == 14 )
                    return 7;
                if ( port == GPIOB && pos == 3 )
                    return 7;
                if ( port == GPIOD && pos == 5 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 9 )
                    return 7;
                if ( port == GPIOB && pos == 10 )
                    return 7;
                if ( port == GPIOC && pos == 10 )
                    return 7;
                if ( port == GPIOD && pos == 8 )
                    return 7;
            }
        #endif
        #if defined(STM32G491CCTx) || defined(STM32G491CETx) || defined(STM32G491CCUx) || \
            defined(STM32G491CEUx) || defined(STM32G491KCUx) || defined(STM32G491KEUx) || \
            defined(STM32G491MCSx) || defined(STM32G491MESx) || defined(STM32G491MCTx) || \
            defined(STM32G491METx) || defined(STM32G491RCIx) || defined(STM32G491REIx) || \
            defined(STM32G491RCTx) || defined(STM32G491RETx) || defined(STM32G491REYx) || \
            defined(STM32G491VCTx) || defined(STM32G491VETx) || defined(STM32G4A1CETx) || \
            defined(STM32G4A1CEUx) || defined(STM32G4A1KEUx) || defined(STM32G4A1MESx) || \
            defined(STM32G4A1METx) || defined(STM32G4A1REIx) || defined(STM32G4A1RETx) || \
            defined(STM32G4A1REYx) || defined(STM32G4A1VETx)
            if ( periph == UART4 ) {
                if ( port == GPIOC && pos == 10 )
                    return 5;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOC && pos == 12 )
                    return 5;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 7;
                if ( port == GPIOB && pos == 6 )
                    return 7;
                if ( port == GPIOC && pos == 4 )
                    return 7;
                if ( port == GPIOE && pos == 0 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 7;
                if ( port == GPIOA && pos == 14 )
                    return 7;
                if ( port == GPIOB && pos == 3 )
                    return 7;
                if ( port == GPIOD && pos == 5 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 9 )
                    return 7;
                if ( port == GPIOB && pos == 10 )
                    return 7;
                if ( port == GPIOC && pos == 10 )
                    return 7;
                if ( port == GPIOD && pos == 8 )
                    return 7;
            }
        #endif
        impossible( "Incorrect TX pin" );
    }

    int uartAlternateFunRX( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if !defined(STM32G431C6Tx) && !defined(STM32G431C8Tx) && !defined(STM32G431CBTx) && \
            !defined(STM32G431C6Ux) && !defined(STM32G431C8Ux) && !defined(STM32G431CBUx) && \
            !defined(STM32G431CBYx) && !defined(STM32G431K6Tx) && !defined(STM32G431K8Tx) && \
            !defined(STM32G431KBTx) && !defined(STM32G431K6Ux) && !defined(STM32G431K8Ux) && \
            !defined(STM32G431KBUx) && !defined(STM32G431M6Tx) && !defined(STM32G431M8Tx) && \
            !defined(STM32G431MBTx) && !defined(STM32G431R6Ix) && !defined(STM32G431R8Ix) && \
            !defined(STM32G431RBIx) && !defined(STM32G431R6Tx) && !defined(STM32G431R8Tx) && \
            !defined(STM32G431RBTx) && !defined(STM32G431V6Tx) && !defined(STM32G431V8Tx) && \
            !defined(STM32G431VBTx) && !defined(STM32G441CBTx) && !defined(STM32G441CBUx) && \
            !defined(STM32G441CBYx) && !defined(STM32G441KBTx) && !defined(STM32G441KBUx) && \
            !defined(STM32G441MBTx) && !defined(STM32G441RBIx) && !defined(STM32G441RBTx) && \
            !defined(STM32G441VBTx) && !defined(STM32G471CCTx) && !defined(STM32G471CETx) && \
            !defined(STM32G471CCUx) && !defined(STM32G471CEUx) && !defined(STM32G471MCTx) && \
            !defined(STM32G471METx) && !defined(STM32G471MEYx) && !defined(STM32G471QCTx) && \
            !defined(STM32G471QETx) && !defined(STM32G471RCTx) && !defined(STM32G471RETx) && \
            !defined(STM32G471VCHx) && !defined(STM32G471VEHx) && !defined(STM32G471VCIx) && \
            !defined(STM32G471VEIx) && !defined(STM32G471VCTx) && !defined(STM32G471VETx) && \
            !defined(STM32G491CCTx) && !defined(STM32G491CETx) && !defined(STM32G491CCUx) && \
            !defined(STM32G491CEUx) && !defined(STM32G491KCUx) && !defined(STM32G491KEUx) && \
            !defined(STM32G491MCSx) && !defined(STM32G491MESx) && !defined(STM32G491MCTx) && \
            !defined(STM32G491METx) && !defined(STM32G491RCIx) && !defined(STM32G491REIx) && \
            !defined(STM32G491RCTx) && !defined(STM32G491RETx) && !defined(STM32G491REYx) && \
            !defined(STM32G491VCTx) && !defined(STM32G491VETx) && !defined(STM32G4A1CETx) && \
            !defined(STM32G4A1CEUx) && !defined(STM32G4A1KEUx) && !defined(STM32G4A1MESx) && \
            !defined(STM32G4A1METx) && !defined(STM32G4A1REIx) && !defined(STM32G4A1RETx) && \
            !defined(STM32G4A1REYx) && !defined(STM32G4A1VETx) && !defined(STM32G473CBTx) && \
            !defined(STM32G473CCTx) && !defined(STM32G473CETx) && !defined(STM32G473CBUx) && \
            !defined(STM32G473CCUx) && !defined(STM32G473CEUx) && !defined(STM32G473MBTx) && \
            !defined(STM32G473MCTx) && !defined(STM32G473METx) && !defined(STM32G473MEYx) && \
            !defined(STM32G473PBIx) && !defined(STM32G473PCIx) && !defined(STM32G473PEIx) && \
            !defined(STM32G473QBTx) && !defined(STM32G473QCTx) && !defined(STM32G473QETx) && \
            !defined(STM32G473RBTx) && !defined(STM32G473RCTx) && !defined(STM32G473RETx) && \
            !defined(STM32G473VBHx) && !defined(STM32G473VCHx) && !defined(STM32G473VEHx) && \
            !defined(STM32G473VBIx) && !defined(STM32G473VCIx) && !defined(STM32G473VEIx) && \
            !defined(STM32G473VBTx) && !defined(STM32G473VCTx) && !defined(STM32G473VETx) && \
            !defined(STM32G483CETx) && !defined(STM32G483CEUx) && !defined(STM32G483METx) && \
            !defined(STM32G483MEYx) && !defined(STM32G483PEIx) && !defined(STM32G483QETx) && \
            !defined(STM32G483RETx) && !defined(STM32G483VEHx) && !defined(STM32G483VEIx) && \
            !defined(STM32G483VETx) && !defined(STM32G474CBTx) && !defined(STM32G474CCTx) && \
            !defined(STM32G474CETx) && !defined(STM32G474CBUx) && !defined(STM32G474CCUx) && \
            !defined(STM32G474CEUx) && !defined(STM32G474MBTx) && !defined(STM32G474MCTx) && \
            !defined(STM32G474METx) && !defined(STM32G474MEYx) && !defined(STM32G474PBIx) && \
            !defined(STM32G474PCIx) && !defined(STM32G474PEIx) && !defined(STM32G474QBTx) && \
            !defined(STM32G474QCTx) && !defined(STM32G474QETx) && !defined(STM32G474RBTx) && \
            !defined(STM32G474RCTx) && !defined(STM32G474RETx) && !defined(STM32G474VBHx) && \
            !defined(STM32G474VCHx) && !defined(STM32G474VEHx) && !defined(STM32G474VBIx) && \
            !defined(STM32G474VCIx) && !defined(STM32G474VEIx) && !defined(STM32G474VBTx) && \
            !defined(STM32G474VCTx) && !defined(STM32G474VETx) && !defined(STM32G484CETx) && \
            !defined(STM32G484CEUx) && !defined(STM32G484METx) && !defined(STM32G484MEYx) && \
            !defined(STM32G484PEIx) && !defined(STM32G484QETx) && !defined(STM32G484RETx) && \
            !defined(STM32G484VEHx) && !defined(STM32G484VEIx) && !defined(STM32G484VETx)
                #error Invalid MCU specified
        #endif
        #if defined(STM32G431C6Tx) || defined(STM32G431C8Tx) || defined(STM32G431CBTx) || \
            defined(STM32G431C6Ux) || defined(STM32G431C8Ux) || defined(STM32G431CBUx) || \
            defined(STM32G431CBYx) || defined(STM32G431K6Tx) || defined(STM32G431K8Tx) || \
            defined(STM32G431KBTx) || defined(STM32G431K6Ux) || defined(STM32G431K8Ux) || \
            defined(STM32G431KBUx) || defined(STM32G431M6Tx) || defined(STM32G431M8Tx) || \
            defined(STM32G431MBTx) || defined(STM32G431R6Ix) || defined(STM32G431R8Ix) || \
            defined(STM32G431RBIx) || defined(STM32G431R6Tx) || defined(STM32G431R8Tx) || \
            defined(STM32G431RBTx) || defined(STM32G431V6Tx) || defined(STM32G431V8Tx) || \
            defined(STM32G431VBTx) || defined(STM32G441CBTx) || defined(STM32G441CBUx) || \
            defined(STM32G441CBYx) || defined(STM32G441KBTx) || defined(STM32G441KBUx) || \
            defined(STM32G441MBTx) || defined(STM32G441RBIx) || defined(STM32G441RBTx) || \
            defined(STM32G441VBTx)
            if ( periph == UART4 ) {
                if ( port == GPIOC && pos == 11 )
                    return 5;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 7;
                if ( port == GPIOB && pos == 7 )
                    return 7;
                if ( port == GPIOC && pos == 5 )
                    return 7;
                if ( port == GPIOE && pos == 1 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 7;
                if ( port == GPIOA && pos == 15 )
                    return 7;
                if ( port == GPIOB && pos == 4 )
                    return 7;
                if ( port == GPIOD && pos == 6 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 8 )
                    return 7;
                if ( port == GPIOB && pos == 11 )
                    return 7;
                if ( port == GPIOC && pos == 11 )
                    return 7;
                if ( port == GPIOD && pos == 9 )
                    return 7;
                if ( port == GPIOE && pos == 15 )
                    return 7;
            }
        #endif
        #if defined(STM32G471CCTx) || defined(STM32G471CETx) || defined(STM32G471CCUx) || \
            defined(STM32G471CEUx) || defined(STM32G471MCTx) || defined(STM32G471METx) || \
            defined(STM32G471MEYx) || defined(STM32G471QCTx) || defined(STM32G471QETx) || \
            defined(STM32G471RCTx) || defined(STM32G471RETx) || defined(STM32G471VCHx) || \
            defined(STM32G471VEHx) || defined(STM32G471VCIx) || defined(STM32G471VEIx) || \
            defined(STM32G471VCTx) || defined(STM32G471VETx) || defined(STM32G491CCTx) || \
            defined(STM32G491CETx) || defined(STM32G491CCUx) || defined(STM32G491CEUx) || \
            defined(STM32G491KCUx) || defined(STM32G491KEUx) || defined(STM32G491MCSx) || \
            defined(STM32G491MESx) || defined(STM32G491MCTx) || defined(STM32G491METx) || \
            defined(STM32G491RCIx) || defined(STM32G491REIx) || defined(STM32G491RCTx) || \
            defined(STM32G491RETx) || defined(STM32G491REYx) || defined(STM32G491VCTx) || \
            defined(STM32G491VETx) || defined(STM32G4A1CETx) || defined(STM32G4A1CEUx) || \
            defined(STM32G4A1KEUx) || defined(STM32G4A1MESx) || defined(STM32G4A1METx) || \
            defined(STM32G4A1REIx) || defined(STM32G4A1RETx) || defined(STM32G4A1REYx) || \
            defined(STM32G4A1VETx) || defined(STM32G473CBTx) || defined(STM32G473CCTx) || \
            defined(STM32G473CETx) || defined(STM32G473CBUx) || defined(STM32G473CCUx) || \
            defined(STM32G473CEUx) || defined(STM32G473MBTx) || defined(STM32G473MCTx) || \
            defined(STM32G473METx) || defined(STM32G473MEYx) || defined(STM32G473PBIx) || \
            defined(STM32G473PCIx) || defined(STM32G473PEIx) || defined(STM32G473QBTx) || \
            defined(STM32G473QCTx) || defined(STM32G473QETx) || defined(STM32G473RBTx) || \
            defined(STM32G473RCTx) || defined(STM32G473RETx) || defined(STM32G473VBHx) || \
            defined(STM32G473VCHx) || defined(STM32G473VEHx) || defined(STM32G473VBIx) || \
            defined(STM32G473VCIx) || defined(STM32G473VEIx) || defined(STM32G473VBTx) || \
            defined(STM32G473VCTx) || defined(STM32G473VETx) || defined(STM32G483CETx) || \
            defined(STM32G483CEUx) || defined(STM32G483METx) || defined(STM32G483MEYx) || \
            defined(STM32G483PEIx) || defined(STM32G483QETx) || defined(STM32G483RETx) || \
            defined(STM32G483VEHx) || defined(STM32G483VEIx) || defined(STM32G483VETx) || \
            defined(STM32G474CBTx) || defined(STM32G474CCTx) || defined(STM32G474CETx) || \
            defined(STM32G474CBUx) || defined(STM32G474CCUx) || defined(STM32G474CEUx) || \
            defined(STM32G474MBTx) || defined(STM32G474MCTx) || defined(STM32G474METx) || \
            defined(STM32G474MEYx) || defined(STM32G474PBIx) || defined(STM32G474PCIx) || \
            defined(STM32G474PEIx) || defined(STM32G474QBTx) || defined(STM32G474QCTx) || \
            defined(STM32G474QETx) || defined(STM32G474RBTx) || defined(STM32G474RCTx) || \
            defined(STM32G474RETx) || defined(STM32G474VBHx) || defined(STM32G474VCHx) || \
            defined(STM32G474VEHx) || defined(STM32G474VBIx) || defined(STM32G474VCIx) || \
            defined(STM32G474VEIx) || defined(STM32G474VBTx) || defined(STM32G474VCTx) || \
            defined(STM32G474VETx) || defined(STM32G484CETx) || defined(STM32G484CEUx) || \
            defined(STM32G484METx) || defined(STM32G484MEYx) || defined(STM32G484PEIx) || \
            defined(STM32G484QETx) || defined(STM32G484RETx) || defined(STM32G484VEHx) || \
            defined(STM32G484VEIx) || defined(STM32G484VETx)
            if ( periph == UART4 ) {
                if ( port == GPIOC && pos == 11 )
                    return 5;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOD && pos == 2 )
                    return 5;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 7;
                if ( port == GPIOB && pos == 7 )
                    return 7;
                if ( port == GPIOC && pos == 5 )
                    return 7;
                if ( port == GPIOE && pos == 1 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 7;
                if ( port == GPIOA && pos == 15 )
                    return 7;
                if ( port == GPIOB && pos == 4 )
                    return 7;
                if ( port == GPIOD && pos == 6 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 8 )
                    return 7;
                if ( port == GPIOB && pos == 11 )
                    return 7;
                if ( port == GPIOC && pos == 11 )
                    return 7;
                if ( port == GPIOD && pos == 9 )
                    return 7;
                if ( port == GPIOE && pos == 15 )
                    return 7;
            }
        #endif
        impossible( "Incorrect RX pin" );
    }

    int uartAlternateFunCTS( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if !defined(STM32G431C6Tx) && !defined(STM32G431C8Tx) && !defined(STM32G431CBTx) && \
            !defined(STM32G431C6Ux) && !defined(STM32G431C8Ux) && !defined(STM32G431CBUx) && \
            !defined(STM32G431CBYx) && !defined(STM32G431K6Tx) && !defined(STM32G431K8Tx) && \
            !defined(STM32G431KBTx) && !defined(STM32G431K6Ux) && !defined(STM32G431K8Ux) && \
            !defined(STM32G431KBUx) && !defined(STM32G431M6Tx) && !defined(STM32G431M8Tx) && \
            !defined(STM32G431MBTx) && !defined(STM32G431R6Ix) && !defined(STM32G431R8Ix) && \
            !defined(STM32G431RBIx) && !defined(STM32G431R6Tx) && !defined(STM32G431R8Tx) && \
            !defined(STM32G431RBTx) && !defined(STM32G431V6Tx) && !defined(STM32G431V8Tx) && \
            !defined(STM32G431VBTx) && !defined(STM32G441CBTx) && !defined(STM32G441CBUx) && \
            !defined(STM32G441CBYx) && !defined(STM32G441KBTx) && !defined(STM32G441KBUx) && \
            !defined(STM32G441MBTx) && !defined(STM32G441RBIx) && !defined(STM32G441RBTx) && \
            !defined(STM32G441VBTx) && !defined(STM32G471CCTx) && !defined(STM32G471CETx) && \
            !defined(STM32G471CCUx) && !defined(STM32G471CEUx) && !defined(STM32G471MCTx) && \
            !defined(STM32G471METx) && !defined(STM32G471MEYx) && !defined(STM32G471QCTx) && \
            !defined(STM32G471QETx) && !defined(STM32G471RCTx) && !defined(STM32G471RETx) && \
            !defined(STM32G471VCHx) && !defined(STM32G471VEHx) && !defined(STM32G471VCIx) && \
            !defined(STM32G471VEIx) && !defined(STM32G471VCTx) && !defined(STM32G471VETx) && \
            !defined(STM32G491CCTx) && !defined(STM32G491CETx) && !defined(STM32G491CCUx) && \
            !defined(STM32G491CEUx) && !defined(STM32G491KCUx) && !defined(STM32G491KEUx) && \
            !defined(STM32G491MCSx) && !defined(STM32G491MESx) && !defined(STM32G491MCTx) && \
            !defined(STM32G491METx) && !defined(STM32G491RCIx) && !defined(STM32G491REIx) && \
            !defined(STM32G491RCTx) && !defined(STM32G491RETx) && !defined(STM32G491REYx) && \
            !defined(STM32G491VCTx) && !defined(STM32G491VETx) && !defined(STM32G4A1CETx) && \
            !defined(STM32G4A1CEUx) && !defined(STM32G4A1KEUx) && !defined(STM32G4A1MESx) && \
            !defined(STM32G4A1METx) && !defined(STM32G4A1REIx) && !defined(STM32G4A1RETx) && \
            !defined(STM32G4A1REYx) && !defined(STM32G4A1VETx) && !defined(STM32G473CBTx) && \
            !defined(STM32G473CCTx) && !defined(STM32G473CETx) && !defined(STM32G473CBUx) && \
            !defined(STM32G473CCUx) && !defined(STM32G473CEUx) && !defined(STM32G473MBTx) && \
            !defined(STM32G473MCTx) && !defined(STM32G473METx) && !defined(STM32G473MEYx) && \
            !defined(STM32G473PBIx) && !defined(STM32G473PCIx) && !defined(STM32G473PEIx) && \
            !defined(STM32G473QBTx) && !defined(STM32G473QCTx) && !defined(STM32G473QETx) && \
            !defined(STM32G473RBTx) && !defined(STM32G473RCTx) && !defined(STM32G473RETx) && \
            !defined(STM32G473VBHx) && !defined(STM32G473VCHx) && !defined(STM32G473VEHx) && \
            !defined(STM32G473VBIx) && !defined(STM32G473VCIx) && !defined(STM32G473VEIx) && \
            !defined(STM32G473VBTx) && !defined(STM32G473VCTx) && !defined(STM32G473VETx) && \
            !defined(STM32G483CETx) && !defined(STM32G483CEUx) && !defined(STM32G483METx) && \
            !defined(STM32G483MEYx) && !defined(STM32G483PEIx) && !defined(STM32G483QETx) && \
            !defined(STM32G483RETx) && !defined(STM32G483VEHx) && !defined(STM32G483VEIx) && \
            !defined(STM32G483VETx) && !defined(STM32G474CBTx) && !defined(STM32G474CCTx) && \
            !defined(STM32G474CETx) && !defined(STM32G474CBUx) && !defined(STM32G474CCUx) && \
            !defined(STM32G474CEUx) && !defined(STM32G474MBTx) && !defined(STM32G474MCTx) && \
            !defined(STM32G474METx) && !defined(STM32G474MEYx) && !defined(STM32G474PBIx) && \
            !defined(STM32G474PCIx) && !defined(STM32G474PEIx) && !defined(STM32G474QBTx) && \
            !defined(STM32G474QCTx) && !defined(STM32G474QETx) && !defined(STM32G474RBTx) && \
            !defined(STM32G474RCTx) && !defined(STM32G474RETx) && !defined(STM32G474VBHx) && \
            !defined(STM32G474VCHx) && !defined(STM32G474VEHx) && !defined(STM32G474VBIx) && \
            !defined(STM32G474VCIx) && !defined(STM32G474VEIx) && !defined(STM32G474VBTx) && \
            !defined(STM32G474VCTx) && !defined(STM32G474VETx) && !defined(STM32G484CETx) && \
            !defined(STM32G484CEUx) && !defined(STM32G484METx) && !defined(STM32G484MEYx) && \
            !defined(STM32G484PEIx) && !defined(STM32G484QETx) && !defined(STM32G484RETx) && \
            !defined(STM32G484VEHx) && !defined(STM32G484VEIx) && !defined(STM32G484VETx)
                #error Invalid MCU specified
        #endif
        #if defined(STM32G431C6Tx) || defined(STM32G431C8Tx) || defined(STM32G431CBTx) || \
            defined(STM32G431C6Ux) || defined(STM32G431C8Ux) || defined(STM32G431CBUx) || \
            defined(STM32G431CBYx) || defined(STM32G431K6Tx) || defined(STM32G431K8Tx) || \
            defined(STM32G431KBTx) || defined(STM32G431K6Ux) || defined(STM32G431K8Ux) || \
            defined(STM32G431KBUx) || defined(STM32G431M6Tx) || defined(STM32G431M8Tx) || \
            defined(STM32G431MBTx) || defined(STM32G431R6Ix) || defined(STM32G431R8Ix) || \
            defined(STM32G431RBIx) || defined(STM32G431R6Tx) || defined(STM32G431R8Tx) || \
            defined(STM32G431RBTx) || defined(STM32G431V6Tx) || defined(STM32G431V8Tx) || \
            defined(STM32G431VBTx) || defined(STM32G441CBTx) || defined(STM32G441CBUx) || \
            defined(STM32G441CBYx) || defined(STM32G441KBTx) || defined(STM32G441KBUx) || \
            defined(STM32G441MBTx) || defined(STM32G441RBIx) || defined(STM32G441RBTx) || \
            defined(STM32G441VBTx)
            if ( periph == UART4 ) {
                if ( port == GPIOB && pos == 7 )
                    return 14;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 11 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 0 )
                    return 7;
                if ( port == GPIOD && pos == 3 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOA && pos == 13 )
                    return 7;
                if ( port == GPIOB && pos == 13 )
                    return 7;
                if ( port == GPIOD && pos == 11 )
                    return 7;
            }
        #endif
        #if defined(STM32G471CCTx) || defined(STM32G471CETx) || defined(STM32G471CCUx) || \
            defined(STM32G471CEUx) || defined(STM32G471MCTx) || defined(STM32G471METx) || \
            defined(STM32G471MEYx) || defined(STM32G471QCTx) || defined(STM32G471QETx) || \
            defined(STM32G471RCTx) || defined(STM32G471RETx) || defined(STM32G471VCHx) || \
            defined(STM32G471VEHx) || defined(STM32G471VCIx) || defined(STM32G471VEIx) || \
            defined(STM32G471VCTx) || defined(STM32G471VETx) || defined(STM32G491CCTx) || \
            defined(STM32G491CETx) || defined(STM32G491CCUx) || defined(STM32G491CEUx) || \
            defined(STM32G491KCUx) || defined(STM32G491KEUx) || defined(STM32G491MCSx) || \
            defined(STM32G491MESx) || defined(STM32G491MCTx) || defined(STM32G491METx) || \
            defined(STM32G491RCIx) || defined(STM32G491REIx) || defined(STM32G491RCTx) || \
            defined(STM32G491RETx) || defined(STM32G491REYx) || defined(STM32G491VCTx) || \
            defined(STM32G491VETx) || defined(STM32G4A1CETx) || defined(STM32G4A1CEUx) || \
            defined(STM32G4A1KEUx) || defined(STM32G4A1MESx) || defined(STM32G4A1METx) || \
            defined(STM32G4A1REIx) || defined(STM32G4A1RETx) || defined(STM32G4A1REYx) || \
            defined(STM32G4A1VETx) || defined(STM32G473CBTx) || defined(STM32G473CCTx) || \
            defined(STM32G473CETx) || defined(STM32G473CBUx) || defined(STM32G473CCUx) || \
            defined(STM32G473CEUx) || defined(STM32G473MBTx) || defined(STM32G473MCTx) || \
            defined(STM32G473METx) || defined(STM32G473MEYx) || defined(STM32G473PBIx) || \
            defined(STM32G473PCIx) || defined(STM32G473PEIx) || defined(STM32G473QBTx) || \
            defined(STM32G473QCTx) || defined(STM32G473QETx) || defined(STM32G473RBTx) || \
            defined(STM32G473RCTx) || defined(STM32G473RETx) || defined(STM32G473VBHx) || \
            defined(STM32G473VCHx) || defined(STM32G473VEHx) || defined(STM32G473VBIx) || \
            defined(STM32G473VCIx) || defined(STM32G473VEIx) || defined(STM32G473VBTx) || \
            defined(STM32G473VCTx) || defined(STM32G473VETx) || defined(STM32G483CETx) || \
            defined(STM32G483CEUx) || defined(STM32G483METx) || defined(STM32G483MEYx) || \
            defined(STM32G483PEIx) || defined(STM32G483QETx) || defined(STM32G483RETx) || \
            defined(STM32G483VEHx) || defined(STM32G483VEIx) || defined(STM32G483VETx) || \
            defined(STM32G474CBTx) || defined(STM32G474CCTx) || defined(STM32G474CETx) || \
            defined(STM32G474CBUx) || defined(STM32G474CCUx) || defined(STM32G474CEUx) || \
            defined(STM32G474MBTx) || defined(STM32G474MCTx) || defined(STM32G474METx) || \
            defined(STM32G474MEYx) || defined(STM32G474PBIx) || defined(STM32G474PCIx) || \
            defined(STM32G474PEIx) || defined(STM32G474QBTx) || defined(STM32G474QCTx) || \
            defined(STM32G474QETx) || defined(STM32G474RBTx) || defined(STM32G474RCTx) || \
            defined(STM32G474RETx) || defined(STM32G474VBHx) || defined(STM32G474VCHx) || \
            defined(STM32G474VEHx) || defined(STM32G474VBIx) || defined(STM32G474VCIx) || \
            defined(STM32G474VEIx) || defined(STM32G474VBTx) || defined(STM32G474VCTx) || \
            defined(STM32G474VETx) || defined(STM32G484CETx) || defined(STM32G484CEUx) || \
            defined(STM32G484METx) || defined(STM32G484MEYx) || defined(STM32G484PEIx) || \
            defined(STM32G484QETx) || defined(STM32G484RETx) || defined(STM32G484VEHx) || \
            defined(STM32G484VEIx) || defined(STM32G484VETx)
            if ( periph == UART4 ) {
                if ( port == GPIOB && pos == 7 )
                    return 14;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOB && pos == 5 )
                    return 14;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 11 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 0 )
                    return 7;
                if ( port == GPIOD && pos == 3 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOA && pos == 13 )
                    return 7;
                if ( port == GPIOB && pos == 13 )
                    return 7;
                if ( port == GPIOD && pos == 11 )
                    return 7;
            }
        #endif
        impossible( "Incorrect CTS pin" );
    }

    int uartAlternateFunRTS( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if !defined(STM32G431C6Tx) && !defined(STM32G431C8Tx) && !defined(STM32G431CBTx) && \
            !defined(STM32G431C6Ux) && !defined(STM32G431C8Ux) && !defined(STM32G431CBUx) && \
            !defined(STM32G431CBYx) && !defined(STM32G431K6Tx) && !defined(STM32G431K8Tx) && \
            !defined(STM32G431KBTx) && !defined(STM32G431K6Ux) && !defined(STM32G431K8Ux) && \
            !defined(STM32G431KBUx) && !defined(STM32G431M6Tx) && !defined(STM32G431M8Tx) && \
            !defined(STM32G431MBTx) && !defined(STM32G431R6Ix) && !defined(STM32G431R8Ix) && \
            !defined(STM32G431RBIx) && !defined(STM32G431R6Tx) && !defined(STM32G431R8Tx) && \
            !defined(STM32G431RBTx) && !defined(STM32G431V6Tx) && !defined(STM32G431V8Tx) && \
            !defined(STM32G431VBTx) && !defined(STM32G441CBTx) && !defined(STM32G441CBUx) && \
            !defined(STM32G441CBYx) && !defined(STM32G441KBTx) && !defined(STM32G441KBUx) && \
            !defined(STM32G441MBTx) && !defined(STM32G441RBIx) && !defined(STM32G441RBTx) && \
            !defined(STM32G441VBTx) && !defined(STM32G471CCTx) && !defined(STM32G471CETx) && \
            !defined(STM32G471CCUx) && !defined(STM32G471CEUx) && !defined(STM32G471MCTx) && \
            !defined(STM32G471METx) && !defined(STM32G471MEYx) && !defined(STM32G471QCTx) && \
            !defined(STM32G471QETx) && !defined(STM32G471RCTx) && !defined(STM32G471RETx) && \
            !defined(STM32G471VCHx) && !defined(STM32G471VEHx) && !defined(STM32G471VCIx) && \
            !defined(STM32G471VEIx) && !defined(STM32G471VCTx) && !defined(STM32G471VETx) && \
            !defined(STM32G491CCTx) && !defined(STM32G491CETx) && !defined(STM32G491CCUx) && \
            !defined(STM32G491CEUx) && !defined(STM32G491KCUx) && !defined(STM32G491KEUx) && \
            !defined(STM32G491MCSx) && !defined(STM32G491MESx) && !defined(STM32G491MCTx) && \
            !defined(STM32G491METx) && !defined(STM32G491RCIx) && !defined(STM32G491REIx) && \
            !defined(STM32G491RCTx) && !defined(STM32G491RETx) && !defined(STM32G491REYx) && \
            !defined(STM32G491VCTx) && !defined(STM32G491VETx) && !defined(STM32G4A1CETx) && \
            !defined(STM32G4A1CEUx) && !defined(STM32G4A1KEUx) && !defined(STM32G4A1MESx) && \
            !defined(STM32G4A1METx) && !defined(STM32G4A1REIx) && !defined(STM32G4A1RETx) && \
            !defined(STM32G4A1REYx) && !defined(STM32G4A1VETx) && !defined(STM32G473CBTx) && \
            !defined(STM32G473CCTx) && !defined(STM32G473CETx) && !defined(STM32G473CBUx) && \
            !defined(STM32G473CCUx) && !defined(STM32G473CEUx) && !defined(STM32G473MBTx) && \
            !defined(STM32G473MCTx) && !defined(STM32G473METx) && !defined(STM32G473MEYx) && \
            !defined(STM32G473PBIx) && !defined(STM32G473PCIx) && !defined(STM32G473PEIx) && \
            !defined(STM32G473QBTx) && !defined(STM32G473QCTx) && !defined(STM32G473QETx) && \
            !defined(STM32G473RBTx) && !defined(STM32G473RCTx) && !defined(STM32G473RETx) && \
            !defined(STM32G473VBHx) && !defined(STM32G473VCHx) && !defined(STM32G473VEHx) && \
            !defined(STM32G473VBIx) && !defined(STM32G473VCIx) && !defined(STM32G473VEIx) && \
            !defined(STM32G473VBTx) && !defined(STM32G473VCTx) && !defined(STM32G473VETx) && \
            !defined(STM32G483CETx) && !defined(STM32G483CEUx) && !defined(STM32G483METx) && \
            !defined(STM32G483MEYx) && !defined(STM32G483PEIx) && !defined(STM32G483QETx) && \
            !defined(STM32G483RETx) && !defined(STM32G483VEHx) && !defined(STM32G483VEIx) && \
            !defined(STM32G483VETx) && !defined(STM32G474CBTx) && !defined(STM32G474CCTx) && \
            !defined(STM32G474CETx) && !defined(STM32G474CBUx) && !defined(STM32G474CCUx) && \
            !defined(STM32G474CEUx) && !defined(STM32G474MBTx) && !defined(STM32G474MCTx) && \
            !defined(STM32G474METx) && !defined(STM32G474MEYx) && !defined(STM32G474PBIx) && \
            !defined(STM32G474PCIx) && !defined(STM32G474PEIx) && !defined(STM32G474QBTx) && \
            !defined(STM32G474QCTx) && !defined(STM32G474QETx) && !defined(STM32G474RBTx) && \
            !defined(STM32G474RCTx) && !defined(STM32G474RETx) && !defined(STM32G474VBHx) && \
            !defined(STM32G474VCHx) && !defined(STM32G474VEHx) && !defined(STM32G474VBIx) && \
            !defined(STM32G474VCIx) && !defined(STM32G474VEIx) && !defined(STM32G474VBTx) && \
            !defined(STM32G474VCTx) && !defined(STM32G474VETx) && !defined(STM32G484CETx) && \
            !defined(STM32G484CEUx) && !defined(STM32G484METx) && !defined(STM32G484MEYx) && \
            !defined(STM32G484PEIx) && !defined(STM32G484QETx) && !defined(STM32G484RETx) && \
            !defined(STM32G484VEHx) && !defined(STM32G484VEIx) && !defined(STM32G484VETx)
                #error Invalid MCU specified
        #endif
        #if defined(STM32G431C6Tx) || defined(STM32G431C8Tx) || defined(STM32G431CBTx) || \
            defined(STM32G431C6Ux) || defined(STM32G431C8Ux) || defined(STM32G431CBUx) || \
            defined(STM32G431CBYx) || defined(STM32G431K6Tx) || defined(STM32G431K8Tx) || \
            defined(STM32G431KBTx) || defined(STM32G431K6Ux) || defined(STM32G431K8Ux) || \
            defined(STM32G431KBUx) || defined(STM32G431M6Tx) || defined(STM32G431M8Tx) || \
            defined(STM32G431MBTx) || defined(STM32G431R6Ix) || defined(STM32G431R8Ix) || \
            defined(STM32G431RBIx) || defined(STM32G431R6Tx) || defined(STM32G431R8Tx) || \
            defined(STM32G431RBTx) || defined(STM32G431V6Tx) || defined(STM32G431V8Tx) || \
            defined(STM32G431VBTx) || defined(STM32G441CBTx) || defined(STM32G441CBUx) || \
            defined(STM32G441CBYx) || defined(STM32G441KBTx) || defined(STM32G441KBUx) || \
            defined(STM32G441MBTx) || defined(STM32G441RBIx) || defined(STM32G441RBTx) || \
            defined(STM32G441VBTx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 7;
                if ( port == GPIOD && pos == 4 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 14 )
                    return 7;
                if ( port == GPIOD && pos == 12 )
                    return 7;
            }
        #endif
        #if defined(STM32G471CCTx) || defined(STM32G471CETx) || defined(STM32G471CCUx) || \
            defined(STM32G471CEUx) || defined(STM32G471MCTx) || defined(STM32G471METx) || \
            defined(STM32G471MEYx) || defined(STM32G471QCTx) || defined(STM32G471QETx) || \
            defined(STM32G471RCTx) || defined(STM32G471RETx) || defined(STM32G471VCHx) || \
            defined(STM32G471VEHx) || defined(STM32G471VCIx) || defined(STM32G471VEIx) || \
            defined(STM32G471VCTx) || defined(STM32G471VETx) || defined(STM32G473CBTx) || \
            defined(STM32G473CCTx) || defined(STM32G473CETx) || defined(STM32G473CBUx) || \
            defined(STM32G473CCUx) || defined(STM32G473CEUx) || defined(STM32G473MBTx) || \
            defined(STM32G473MCTx) || defined(STM32G473METx) || defined(STM32G473MEYx) || \
            defined(STM32G473PBIx) || defined(STM32G473PCIx) || defined(STM32G473PEIx) || \
            defined(STM32G473QBTx) || defined(STM32G473QCTx) || defined(STM32G473QETx) || \
            defined(STM32G473RBTx) || defined(STM32G473RCTx) || defined(STM32G473RETx) || \
            defined(STM32G473VBHx) || defined(STM32G473VCHx) || defined(STM32G473VEHx) || \
            defined(STM32G473VBIx) || defined(STM32G473VCIx) || defined(STM32G473VEIx) || \
            defined(STM32G473VBTx) || defined(STM32G473VCTx) || defined(STM32G473VETx) || \
            defined(STM32G483CETx) || defined(STM32G483CEUx) || defined(STM32G483METx) || \
            defined(STM32G483MEYx) || defined(STM32G483PEIx) || defined(STM32G483QETx) || \
            defined(STM32G483RETx) || defined(STM32G483VEHx) || defined(STM32G483VEIx) || \
            defined(STM32G483VETx) || defined(STM32G474CBTx) || defined(STM32G474CCTx) || \
            defined(STM32G474CETx) || defined(STM32G474CBUx) || defined(STM32G474CCUx) || \
            defined(STM32G474CEUx) || defined(STM32G474MBTx) || defined(STM32G474MCTx) || \
            defined(STM32G474METx) || defined(STM32G474MEYx) || defined(STM32G474PBIx) || \
            defined(STM32G474PCIx) || defined(STM32G474PEIx) || defined(STM32G474QBTx) || \
            defined(STM32G474QCTx) || defined(STM32G474QETx) || defined(STM32G474RBTx) || \
            defined(STM32G474RCTx) || defined(STM32G474RETx) || defined(STM32G474VBHx) || \
            defined(STM32G474VCHx) || defined(STM32G474VEHx) || defined(STM32G474VBIx) || \
            defined(STM32G474VCIx) || defined(STM32G474VEIx) || defined(STM32G474VBTx) || \
            defined(STM32G474VCTx) || defined(STM32G474VETx) || defined(STM32G484CETx) || \
            defined(STM32G484CEUx) || defined(STM32G484METx) || defined(STM32G484MEYx) || \
            defined(STM32G484PEIx) || defined(STM32G484QETx) || defined(STM32G484RETx) || \
            defined(STM32G484VEHx) || defined(STM32G484VEIx) || defined(STM32G484VETx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOB && pos == 4 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 7;
                if ( port == GPIOD && pos == 4 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 14 )
                    return 7;
                if ( port == GPIOD && pos == 12 )
                    return 7;
                if ( port == GPIOF && pos == 6 )
                    return 7;
            }
        #endif
        #if defined(STM32G491CCTx) || defined(STM32G491CETx) || defined(STM32G491CCUx) || \
            defined(STM32G491CEUx) || defined(STM32G491KCUx) || defined(STM32G491KEUx) || \
            defined(STM32G491MCSx) || defined(STM32G491MESx) || defined(STM32G491MCTx) || \
            defined(STM32G491METx) || defined(STM32G491RCIx) || defined(STM32G491REIx) || \
            defined(STM32G491RCTx) || defined(STM32G491RETx) || defined(STM32G491REYx) || \
            defined(STM32G491VCTx) || defined(STM32G491VETx) || defined(STM32G4A1CETx) || \
            defined(STM32G4A1CEUx) || defined(STM32G4A1KEUx) || defined(STM32G4A1MESx) || \
            defined(STM32G4A1METx) || defined(STM32G4A1REIx) || defined(STM32G4A1RETx) || \
            defined(STM32G4A1REYx) || defined(STM32G4A1VETx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOB && pos == 4 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 7;
                if ( port == GPIOD && pos == 4 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 14 )
                    return 7;
                if ( port == GPIOD && pos == 12 )
                    return 7;
            }
        #endif
        impossible( "Incorrect RTS pin" );
    }

    int uartAlternateFunDE( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if !defined(STM32G431C6Tx) && !defined(STM32G431C8Tx) && !defined(STM32G431CBTx) && \
            !defined(STM32G431C6Ux) && !defined(STM32G431C8Ux) && !defined(STM32G431CBUx) && \
            !defined(STM32G431CBYx) && !defined(STM32G431K6Tx) && !defined(STM32G431K8Tx) && \
            !defined(STM32G431KBTx) && !defined(STM32G431K6Ux) && !defined(STM32G431K8Ux) && \
            !defined(STM32G431KBUx) && !defined(STM32G431M6Tx) && !defined(STM32G431M8Tx) && \
            !defined(STM32G431MBTx) && !defined(STM32G431R6Ix) && !defined(STM32G431R8Ix) && \
            !defined(STM32G431RBIx) && !defined(STM32G431R6Tx) && !defined(STM32G431R8Tx) && \
            !defined(STM32G431RBTx) && !defined(STM32G431V6Tx) && !defined(STM32G431V8Tx) && \
            !defined(STM32G431VBTx) && !defined(STM32G441CBTx) && !defined(STM32G441CBUx) && \
            !defined(STM32G441CBYx) && !defined(STM32G441KBTx) && !defined(STM32G441KBUx) && \
            !defined(STM32G441MBTx) && !defined(STM32G441RBIx) && !defined(STM32G441RBTx) && \
            !defined(STM32G441VBTx) && !defined(STM32G471CCTx) && !defined(STM32G471CETx) && \
            !defined(STM32G471CCUx) && !defined(STM32G471CEUx) && !defined(STM32G471MCTx) && \
            !defined(STM32G471METx) && !defined(STM32G471MEYx) && !defined(STM32G471QCTx) && \
            !defined(STM32G471QETx) && !defined(STM32G471RCTx) && !defined(STM32G471RETx) && \
            !defined(STM32G471VCHx) && !defined(STM32G471VEHx) && !defined(STM32G471VCIx) && \
            !defined(STM32G471VEIx) && !defined(STM32G471VCTx) && !defined(STM32G471VETx) && \
            !defined(STM32G491CCTx) && !defined(STM32G491CETx) && !defined(STM32G491CCUx) && \
            !defined(STM32G491CEUx) && !defined(STM32G491KCUx) && !defined(STM32G491KEUx) && \
            !defined(STM32G491MCSx) && !defined(STM32G491MESx) && !defined(STM32G491MCTx) && \
            !defined(STM32G491METx) && !defined(STM32G491RCIx) && !defined(STM32G491REIx) && \
            !defined(STM32G491RCTx) && !defined(STM32G491RETx) && !defined(STM32G491REYx) && \
            !defined(STM32G491VCTx) && !defined(STM32G491VETx) && !defined(STM32G4A1CETx) && \
            !defined(STM32G4A1CEUx) && !defined(STM32G4A1KEUx) && !defined(STM32G4A1MESx) && \
            !defined(STM32G4A1METx) && !defined(STM32G4A1REIx) && !defined(STM32G4A1RETx) && \
            !defined(STM32G4A1REYx) && !defined(STM32G4A1VETx) && !defined(STM32G473CBTx) && \
            !defined(STM32G473CCTx) && !defined(STM32G473CETx) && !defined(STM32G473CBUx) && \
            !defined(STM32G473CCUx) && !defined(STM32G473CEUx) && !defined(STM32G473MBTx) && \
            !defined(STM32G473MCTx) && !defined(STM32G473METx) && !defined(STM32G473MEYx) && \
            !defined(STM32G473PBIx) && !defined(STM32G473PCIx) && !defined(STM32G473PEIx) && \
            !defined(STM32G473QBTx) && !defined(STM32G473QCTx) && !defined(STM32G473QETx) && \
            !defined(STM32G473RBTx) && !defined(STM32G473RCTx) && !defined(STM32G473RETx) && \
            !defined(STM32G473VBHx) && !defined(STM32G473VCHx) && !defined(STM32G473VEHx) && \
            !defined(STM32G473VBIx) && !defined(STM32G473VCIx) && !defined(STM32G473VEIx) && \
            !defined(STM32G473VBTx) && !defined(STM32G473VCTx) && !defined(STM32G473VETx) && \
            !defined(STM32G483CETx) && !defined(STM32G483CEUx) && !defined(STM32G483METx) && \
            !defined(STM32G483MEYx) && !defined(STM32G483PEIx) && !defined(STM32G483QETx) && \
            !defined(STM32G483RETx) && !defined(STM32G483VEHx) && !defined(STM32G483VEIx) && \
            !defined(STM32G483VETx) && !defined(STM32G474CBTx) && !defined(STM32G474CCTx) && \
            !defined(STM32G474CETx) && !defined(STM32G474CBUx) && !defined(STM32G474CCUx) && \
            !defined(STM32G474CEUx) && !defined(STM32G474MBTx) && !defined(STM32G474MCTx) && \
            !defined(STM32G474METx) && !defined(STM32G474MEYx) && !defined(STM32G474PBIx) && \
            !defined(STM32G474PCIx) && !defined(STM32G474PEIx) && !defined(STM32G474QBTx) && \
            !defined(STM32G474QCTx) && !defined(STM32G474QETx) && !defined(STM32G474RBTx) && \
            !defined(STM32G474RCTx) && !defined(STM32G474RETx) && !defined(STM32G474VBHx) && \
            !defined(STM32G474VCHx) && !defined(STM32G474VEHx) && !defined(STM32G474VBIx) && \
            !defined(STM32G474VCIx) && !defined(STM32G474VEIx) && !defined(STM32G474VBTx) && \
            !defined(STM32G474VCTx) && !defined(STM32G474VETx) && !defined(STM32G484CETx) && \
            !defined(STM32G484CEUx) && !defined(STM32G484METx) && !defined(STM32G484MEYx) && \
            !defined(STM32G484PEIx) && !defined(STM32G484QETx) && !defined(STM32G484RETx) && \
            !defined(STM32G484VEHx) && !defined(STM32G484VEIx) && !defined(STM32G484VETx)
                #error Invalid MCU specified
        #endif
        #if defined(STM32G431C6Tx) || defined(STM32G431C8Tx) || defined(STM32G431CBTx) || \
            defined(STM32G431C6Ux) || defined(STM32G431C8Ux) || defined(STM32G431CBUx) || \
            defined(STM32G431CBYx) || defined(STM32G431K6Tx) || defined(STM32G431K8Tx) || \
            defined(STM32G431KBTx) || defined(STM32G431K6Ux) || defined(STM32G431K8Ux) || \
            defined(STM32G431KBUx) || defined(STM32G431M6Tx) || defined(STM32G431M8Tx) || \
            defined(STM32G431MBTx) || defined(STM32G431R6Ix) || defined(STM32G431R8Ix) || \
            defined(STM32G431RBIx) || defined(STM32G431R6Tx) || defined(STM32G431R8Tx) || \
            defined(STM32G431RBTx) || defined(STM32G431V6Tx) || defined(STM32G431V8Tx) || \
            defined(STM32G431VBTx) || defined(STM32G441CBTx) || defined(STM32G441CBUx) || \
            defined(STM32G441CBYx) || defined(STM32G441KBTx) || defined(STM32G441KBUx) || \
            defined(STM32G441MBTx) || defined(STM32G441RBIx) || defined(STM32G441RBTx) || \
            defined(STM32G441VBTx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 7;
                if ( port == GPIOD && pos == 4 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 14 )
                    return 7;
                if ( port == GPIOD && pos == 12 )
                    return 7;
            }
        #endif
        #if defined(STM32G471CCTx) || defined(STM32G471CETx) || defined(STM32G471CCUx) || \
            defined(STM32G471CEUx) || defined(STM32G471MCTx) || defined(STM32G471METx) || \
            defined(STM32G471MEYx) || defined(STM32G471QCTx) || defined(STM32G471QETx) || \
            defined(STM32G471RCTx) || defined(STM32G471RETx) || defined(STM32G471VCHx) || \
            defined(STM32G471VEHx) || defined(STM32G471VCIx) || defined(STM32G471VEIx) || \
            defined(STM32G471VCTx) || defined(STM32G471VETx) || defined(STM32G473CBTx) || \
            defined(STM32G473CCTx) || defined(STM32G473CETx) || defined(STM32G473CBUx) || \
            defined(STM32G473CCUx) || defined(STM32G473CEUx) || defined(STM32G473MBTx) || \
            defined(STM32G473MCTx) || defined(STM32G473METx) || defined(STM32G473MEYx) || \
            defined(STM32G473PBIx) || defined(STM32G473PCIx) || defined(STM32G473PEIx) || \
            defined(STM32G473QBTx) || defined(STM32G473QCTx) || defined(STM32G473QETx) || \
            defined(STM32G473RBTx) || defined(STM32G473RCTx) || defined(STM32G473RETx) || \
            defined(STM32G473VBHx) || defined(STM32G473VCHx) || defined(STM32G473VEHx) || \
            defined(STM32G473VBIx) || defined(STM32G473VCIx) || defined(STM32G473VEIx) || \
            defined(STM32G473VBTx) || defined(STM32G473VCTx) || defined(STM32G473VETx) || \
            defined(STM32G483CETx) || defined(STM32G483CEUx) || defined(STM32G483METx) || \
            defined(STM32G483MEYx) || defined(STM32G483PEIx) || defined(STM32G483QETx) || \
            defined(STM32G483RETx) || defined(STM32G483VEHx) || defined(STM32G483VEIx) || \
            defined(STM32G483VETx) || defined(STM32G474CBTx) || defined(STM32G474CCTx) || \
            defined(STM32G474CETx) || defined(STM32G474CBUx) || defined(STM32G474CCUx) || \
            defined(STM32G474CEUx) || defined(STM32G474MBTx) || defined(STM32G474MCTx) || \
            defined(STM32G474METx) || defined(STM32G474MEYx) || defined(STM32G474PBIx) || \
            defined(STM32G474PCIx) || defined(STM32G474PEIx) || defined(STM32G474QBTx) || \
            defined(STM32G474QCTx) || defined(STM32G474QETx) || defined(STM32G474RBTx) || \
            defined(STM32G474RCTx) || defined(STM32G474RETx) || defined(STM32G474VBHx) || \
            defined(STM32G474VCHx) || defined(STM32G474VEHx) || defined(STM32G474VBIx) || \
            defined(STM32G474VCIx) || defined(STM32G474VEIx) || defined(STM32G474VBTx) || \
            defined(STM32G474VCTx) || defined(STM32G474VETx) || defined(STM32G484CETx) || \
            defined(STM32G484CEUx) || defined(STM32G484METx) || defined(STM32G484MEYx) || \
            defined(STM32G484PEIx) || defined(STM32G484QETx) || defined(STM32G484RETx) || \
            defined(STM32G484VEHx) || defined(STM32G484VEIx) || defined(STM32G484VETx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOB && pos == 4 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 7;
                if ( port == GPIOD && pos == 4 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 14 )
                    return 7;
                if ( port == GPIOD && pos == 12 )
                    return 7;
                if ( port == GPIOF && pos == 6 )
                    return 7;
            }
        #endif
        #if defined(STM32G491CCTx) || defined(STM32G491CETx) || defined(STM32G491CCUx) || \
            defined(STM32G491CEUx) || defined(STM32G491KCUx) || defined(STM32G491KEUx) || \
            defined(STM32G491MCSx) || defined(STM32G491MESx) || defined(STM32G491MCTx) || \
            defined(STM32G491METx) || defined(STM32G491RCIx) || defined(STM32G491REIx) || \
            defined(STM32G491RCTx) || defined(STM32G491RETx) || defined(STM32G491REYx) || \
            defined(STM32G491VCTx) || defined(STM32G491VETx) || defined(STM32G4A1CETx) || \
            defined(STM32G4A1CEUx) || defined(STM32G4A1KEUx) || defined(STM32G4A1MESx) || \
            defined(STM32G4A1METx) || defined(STM32G4A1REIx) || defined(STM32G4A1RETx) || \
            defined(STM32G4A1REYx) || defined(STM32G4A1VETx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOB && pos == 4 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 7;
                if ( port == GPIOD && pos == 4 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 14 )
                    return 7;
                if ( port == GPIOD && pos == 12 )
                    return 7;
            }
        #endif
        impossible( "Incorrect DE pin" );
    }

} // namespace detail

