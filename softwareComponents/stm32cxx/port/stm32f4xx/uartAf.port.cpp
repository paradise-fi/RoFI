#include <stm32f4xx_ll_bus.h>
#include <stm32f4xx_ll_usart.h>
#include <stm32f4xx_ll_gpio.h>
#include <system/assert.hpp>

namespace detail {
    int uartAlternateFunTX( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if !defined(STM32F401CBUx) && !defined(STM32F401CCUx) && !defined(STM32F401CBYx) && \
            !defined(STM32F401CCYx) && !defined(STM32F401CDUx) && !defined(STM32F401CEUx) && \
            !defined(STM32F401CDYx) && !defined(STM32F401CEYx) && !defined(STM32F401CCFx) && \
            !defined(STM32F401RBTx) && !defined(STM32F401RCTx) && !defined(STM32F401RDTx) && \
            !defined(STM32F401RETx) && !defined(STM32F401VBHx) && !defined(STM32F401VCHx) && \
            !defined(STM32F401VBTx) && !defined(STM32F401VCTx) && !defined(STM32F401VDHx) && \
            !defined(STM32F401VEHx) && !defined(STM32F401VDTx) && !defined(STM32F401VETx) && \
            !defined(STM32F405OEYx) && !defined(STM32F405OGYx) && !defined(STM32F405RGTx) && \
            !defined(STM32F405VGTx) && !defined(STM32F405ZGTx) && !defined(STM32F415OGYx) && \
            !defined(STM32F415RGTx) && !defined(STM32F415VGTx) && !defined(STM32F415ZGTx) && \
            !defined(STM32F407IEHx) && !defined(STM32F407IGHx) && !defined(STM32F407IETx) && \
            !defined(STM32F407IGTx) && !defined(STM32F407VETx) && !defined(STM32F407VGTx) && \
            !defined(STM32F407ZETx) && !defined(STM32F407ZGTx) && !defined(STM32F417IEHx) && \
            !defined(STM32F417IGHx) && !defined(STM32F417IETx) && !defined(STM32F417IGTx) && \
            !defined(STM32F417VETx) && !defined(STM32F417VGTx) && !defined(STM32F417ZETx) && \
            !defined(STM32F417ZGTx) && !defined(STM32F410C8Tx) && !defined(STM32F410CBTx) && \
            !defined(STM32F410C8Ux) && !defined(STM32F410CBUx) && !defined(STM32F410R8Ix) && \
            !defined(STM32F410RBIx) && !defined(STM32F410R8Tx) && !defined(STM32F410RBTx) && \
            !defined(STM32F410T8Yx) && !defined(STM32F410TBYx) && !defined(STM32F411CCUx) && \
            !defined(STM32F411CEUx) && !defined(STM32F411CCYx) && !defined(STM32F411CEYx) && \
            !defined(STM32F411RCTx) && !defined(STM32F411RETx) && !defined(STM32F411VCHx) && \
            !defined(STM32F411VEHx) && !defined(STM32F411VCTx) && !defined(STM32F411VETx) && \
            !defined(STM32F412CEUx) && !defined(STM32F412CGUx) && !defined(STM32F412RETx) && \
            !defined(STM32F412RGTx) && !defined(STM32F412REYx) && !defined(STM32F412RGYx) && \
            !defined(STM32F412REYxP) && !defined(STM32F412RGYxP) && !defined(STM32F412VEHx) && \
            !defined(STM32F412VGHx) && !defined(STM32F412VETx) && !defined(STM32F412VGTx) && \
            !defined(STM32F412ZEJx) && !defined(STM32F412ZGJx) && !defined(STM32F412ZETx) && \
            !defined(STM32F412ZGTx) && !defined(STM32F413CGUx) && !defined(STM32F413CHUx) && \
            !defined(STM32F413MGYx) && !defined(STM32F413MHYx) && !defined(STM32F413RGTx) && \
            !defined(STM32F413RHTx) && !defined(STM32F413VGHx) && !defined(STM32F413VHHx) && \
            !defined(STM32F413VGTx) && !defined(STM32F413VHTx) && !defined(STM32F413ZGJx) && \
            !defined(STM32F413ZHJx) && !defined(STM32F413ZGTx) && !defined(STM32F413ZHTx) && \
            !defined(STM32F423CHUx) && !defined(STM32F423MHYx) && !defined(STM32F423RHTx) && \
            !defined(STM32F423VHHx) && !defined(STM32F423VHTx) && !defined(STM32F423ZHJx) && \
            !defined(STM32F423ZHTx) && !defined(STM32F427AGHx) && !defined(STM32F427AIHx) && \
            !defined(STM32F427IGHx) && !defined(STM32F427IIHx) && !defined(STM32F427IGTx) && \
            !defined(STM32F427IITx) && !defined(STM32F427VGTx) && !defined(STM32F427VITx) && \
            !defined(STM32F427ZGTx) && !defined(STM32F427ZITx) && !defined(STM32F437AIHx) && \
            !defined(STM32F437IGHx) && !defined(STM32F437IIHx) && !defined(STM32F437IGTx) && \
            !defined(STM32F437IITx) && !defined(STM32F437VGTx) && !defined(STM32F437VITx) && \
            !defined(STM32F437ZGTx) && !defined(STM32F437ZITx) && !defined(STM32F429AGHx) && \
            !defined(STM32F429AIHx) && !defined(STM32F429BETx) && !defined(STM32F429BGTx) && \
            !defined(STM32F429BITx) && !defined(STM32F429IETx) && !defined(STM32F429IGTx) && \
            !defined(STM32F429IEHx) && !defined(STM32F429IGHx) && !defined(STM32F429IIHx) && \
            !defined(STM32F429IITx) && !defined(STM32F429NEHx) && !defined(STM32F429NGHx) && \
            !defined(STM32F429NIHx) && !defined(STM32F429VETx) && !defined(STM32F429VGTx) && \
            !defined(STM32F429VITx) && !defined(STM32F429ZETx) && !defined(STM32F429ZGTx) && \
            !defined(STM32F429ZGYx) && !defined(STM32F429ZITx) && !defined(STM32F429ZIYx) && \
            !defined(STM32F439AIHx) && !defined(STM32F439BGTx) && !defined(STM32F439BITx) && \
            !defined(STM32F439IGHx) && !defined(STM32F439IIHx) && !defined(STM32F439IGTx) && \
            !defined(STM32F439IITx) && !defined(STM32F439NGHx) && !defined(STM32F439NIHx) && \
            !defined(STM32F439VGTx) && !defined(STM32F439VITx) && !defined(STM32F439ZGTx) && \
            !defined(STM32F439ZITx) && !defined(STM32F439ZGYx) && !defined(STM32F439ZIYx) && \
            !defined(STM32F446MCYx) && !defined(STM32F446MEYx) && !defined(STM32F446RCTx) && \
            !defined(STM32F446RETx) && !defined(STM32F446VCTx) && !defined(STM32F446VETx) && \
            !defined(STM32F446ZCHx) && !defined(STM32F446ZEHx) && !defined(STM32F446ZCJx) && \
            !defined(STM32F446ZEJx) && !defined(STM32F446ZCTx) && !defined(STM32F446ZETx) && \
            !defined(STM32F469AEHx) && !defined(STM32F469AGHx) && !defined(STM32F469AIHx) && \
            !defined(STM32F469AEYx) && !defined(STM32F469AGYx) && !defined(STM32F469AIYx) && \
            !defined(STM32F469BETx) && !defined(STM32F469BGTx) && !defined(STM32F469BITx) && \
            !defined(STM32F469IETx) && !defined(STM32F469IGTx) && !defined(STM32F469IEHx) && \
            !defined(STM32F469IGHx) && !defined(STM32F469IIHx) && !defined(STM32F469IITx) && \
            !defined(STM32F469NEHx) && !defined(STM32F469NGHx) && !defined(STM32F469NIHx) && \
            !defined(STM32F469VETx) && !defined(STM32F469VGTx) && !defined(STM32F469VITx) && \
            !defined(STM32F469ZETx) && !defined(STM32F469ZGTx) && !defined(STM32F469ZITx) && \
            !defined(STM32F479AGHx) && !defined(STM32F479AIHx) && !defined(STM32F479AGYx) && \
            !defined(STM32F479AIYx) && !defined(STM32F479BGTx) && !defined(STM32F479BITx) && \
            !defined(STM32F479IGHx) && !defined(STM32F479IIHx) && !defined(STM32F479IGTx) && \
            !defined(STM32F479IITx) && !defined(STM32F479NGHx) && !defined(STM32F479NIHx) && \
            !defined(STM32F479VGTx) && !defined(STM32F479VITx) && !defined(STM32F479ZGTx) && \
            !defined(STM32F479ZITx)
                #error Invalid MCU specified
        #endif
        #if defined(STM32F401CBUx) || defined(STM32F401CCUx) || defined(STM32F401CBYx) || \
            defined(STM32F401CCYx) || defined(STM32F401CDUx) || defined(STM32F401CEUx) || \
            defined(STM32F401CDYx) || defined(STM32F401CEYx) || defined(STM32F401CCFx) || \
            defined(STM32F401RBTx) || defined(STM32F401RCTx) || defined(STM32F401RDTx) || \
            defined(STM32F401RETx) || defined(STM32F401VBHx) || defined(STM32F401VCHx) || \
            defined(STM32F401VBTx) || defined(STM32F401VCTx) || defined(STM32F401VDHx) || \
            defined(STM32F401VEHx) || defined(STM32F401VDTx) || defined(STM32F401VETx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 7;
                if ( port == GPIOB && pos == 6 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 7;
                if ( port == GPIOD && pos == 5 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 11 )
                    return 8;
                if ( port == GPIOC && pos == 6 )
                    return 8;
            }
        #endif
        #if defined(STM32F405OEYx) || defined(STM32F405OGYx) || defined(STM32F405RGTx) || \
            defined(STM32F405VGTx) || defined(STM32F405ZGTx) || defined(STM32F415OGYx) || \
            defined(STM32F415RGTx) || defined(STM32F415VGTx) || defined(STM32F415ZGTx) || \
            defined(STM32F407IEHx) || defined(STM32F407IGHx) || defined(STM32F407IETx) || \
            defined(STM32F407IGTx) || defined(STM32F407VETx) || defined(STM32F407VGTx) || \
            defined(STM32F407ZETx) || defined(STM32F407ZGTx) || defined(STM32F417IEHx) || \
            defined(STM32F417IGHx) || defined(STM32F417IETx) || defined(STM32F417IGTx) || \
            defined(STM32F417VETx) || defined(STM32F417VGTx) || defined(STM32F417ZETx) || \
            defined(STM32F417ZGTx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 0 )
                    return 8;
                if ( port == GPIOC && pos == 10 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOC && pos == 12 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 7;
                if ( port == GPIOB && pos == 6 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 7;
                if ( port == GPIOD && pos == 5 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 10 )
                    return 7;
                if ( port == GPIOC && pos == 10 )
                    return 7;
                if ( port == GPIOD && pos == 8 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOC && pos == 6 )
                    return 8;
                if ( port == GPIOG && pos == 14 )
                    return 8;
            }
        #endif
        #if defined(STM32F410C8Tx) || defined(STM32F410CBTx) || defined(STM32F410C8Ux) || \
            defined(STM32F410CBUx) || defined(STM32F410R8Ix) || defined(STM32F410RBIx) || \
            defined(STM32F410R8Tx) || defined(STM32F410RBTx) || defined(STM32F410T8Yx) || \
            defined(STM32F410TBYx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 7;
                if ( port == GPIOA && pos == 15 )
                    return 7;
                if ( port == GPIOB && pos == 6 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 11 )
                    return 8;
                if ( port == GPIOC && pos == 6 )
                    return 8;
            }
        #endif
        #if defined(STM32F411CCUx) || defined(STM32F411CEUx) || defined(STM32F411CCYx) || \
            defined(STM32F411CEYx) || defined(STM32F411RCTx) || defined(STM32F411RETx) || \
            defined(STM32F411VCHx) || defined(STM32F411VEHx) || defined(STM32F411VCTx) || \
            defined(STM32F411VETx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 7;
                if ( port == GPIOA && pos == 15 )
                    return 7;
                if ( port == GPIOB && pos == 6 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 7;
                if ( port == GPIOD && pos == 5 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 11 )
                    return 8;
                if ( port == GPIOC && pos == 6 )
                    return 8;
            }
        #endif
        #if defined(STM32F412CEUx) || defined(STM32F412CGUx) || defined(STM32F412RETx) || \
            defined(STM32F412RGTx) || defined(STM32F412REYx) || defined(STM32F412RGYx) || \
            defined(STM32F412REYxP) || defined(STM32F412RGYxP) || defined(STM32F412VEHx) || \
            defined(STM32F412VGHx) || defined(STM32F412VETx) || defined(STM32F412VGTx) || \
            defined(STM32F412ZEJx) || defined(STM32F412ZGJx) || defined(STM32F412ZETx) || \
            defined(STM32F412ZGTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 7;
                if ( port == GPIOA && pos == 15 )
                    return 7;
                if ( port == GPIOB && pos == 6 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 7;
                if ( port == GPIOD && pos == 5 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 10 )
                    return 7;
                if ( port == GPIOC && pos == 10 )
                    return 7;
                if ( port == GPIOD && pos == 8 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 11 )
                    return 8;
                if ( port == GPIOC && pos == 6 )
                    return 8;
                if ( port == GPIOG && pos == 14 )
                    return 8;
            }
        #endif
        #if defined(STM32F413CGUx) || defined(STM32F413CHUx) || defined(STM32F413MGYx) || \
            defined(STM32F413MHYx) || defined(STM32F413RGTx) || defined(STM32F413RHTx) || \
            defined(STM32F413VGHx) || defined(STM32F413VHHx) || defined(STM32F413VGTx) || \
            defined(STM32F413VHTx) || defined(STM32F413ZGJx) || defined(STM32F413ZHJx) || \
            defined(STM32F413ZGTx) || defined(STM32F413ZHTx) || defined(STM32F423CHUx) || \
            defined(STM32F423MHYx) || defined(STM32F423RHTx) || defined(STM32F423VHHx) || \
            defined(STM32F423VHTx) || defined(STM32F423ZHJx) || defined(STM32F423ZHTx)
            if ( periph == UART10 ) {
                if ( port == GPIOE && pos == 3 )
                    return 11;
                if ( port == GPIOG && pos == 12 )
                    return 11;
            }
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 0 )
                    return 8;
                if ( port == GPIOA && pos == 12 )
                    return 11;
                if ( port == GPIOD && pos == 1 )
                    return 11;
                if ( port == GPIOD && pos == 10 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOB && pos == 6 )
                    return 11;
                if ( port == GPIOB && pos == 9 )
                    return 11;
                if ( port == GPIOB && pos == 13 )
                    return 11;
                if ( port == GPIOC && pos == 12 )
                    return 8;
            }
            if ( periph == UART7 ) {
                if ( port == GPIOA && pos == 15 )
                    return 8;
                if ( port == GPIOB && pos == 4 )
                    return 8;
                if ( port == GPIOE && pos == 8 )
                    return 8;
                if ( port == GPIOF && pos == 7 )
                    return 8;
            }
            if ( periph == UART8 ) {
                if ( port == GPIOE && pos == 1 )
                    return 8;
                if ( port == GPIOF && pos == 9 )
                    return 8;
            }
            if ( periph == UART9 ) {
                if ( port == GPIOD && pos == 15 )
                    return 11;
                if ( port == GPIOG && pos == 1 )
                    return 11;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 7;
                if ( port == GPIOA && pos == 15 )
                    return 7;
                if ( port == GPIOB && pos == 6 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 7;
                if ( port == GPIOD && pos == 5 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 10 )
                    return 7;
                if ( port == GPIOC && pos == 10 )
                    return 7;
                if ( port == GPIOD && pos == 8 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 11 )
                    return 8;
                if ( port == GPIOC && pos == 6 )
                    return 8;
                if ( port == GPIOG && pos == 14 )
                    return 8;
            }
        #endif
        #if defined(STM32F427AGHx) || defined(STM32F427AIHx) || defined(STM32F427IGHx) || \
            defined(STM32F427IIHx) || defined(STM32F427IGTx) || defined(STM32F427IITx) || \
            defined(STM32F427VGTx) || defined(STM32F427VITx) || defined(STM32F427ZGTx) || \
            defined(STM32F427ZITx) || defined(STM32F437AIHx) || defined(STM32F437IGHx) || \
            defined(STM32F437IIHx) || defined(STM32F437IGTx) || defined(STM32F437IITx) || \
            defined(STM32F437VGTx) || defined(STM32F437VITx) || defined(STM32F437ZGTx) || \
            defined(STM32F437ZITx) || defined(STM32F429AGHx) || defined(STM32F429AIHx) || \
            defined(STM32F429BETx) || defined(STM32F429BGTx) || defined(STM32F429BITx) || \
            defined(STM32F429IETx) || defined(STM32F429IGTx) || defined(STM32F429IEHx) || \
            defined(STM32F429IGHx) || defined(STM32F429IIHx) || defined(STM32F429IITx) || \
            defined(STM32F429NEHx) || defined(STM32F429NGHx) || defined(STM32F429NIHx) || \
            defined(STM32F429VETx) || defined(STM32F429VGTx) || defined(STM32F429VITx) || \
            defined(STM32F429ZETx) || defined(STM32F429ZGTx) || defined(STM32F429ZGYx) || \
            defined(STM32F429ZITx) || defined(STM32F429ZIYx) || defined(STM32F439AIHx) || \
            defined(STM32F439BGTx) || defined(STM32F439BITx) || defined(STM32F439IGHx) || \
            defined(STM32F439IIHx) || defined(STM32F439IGTx) || defined(STM32F439IITx) || \
            defined(STM32F439NGHx) || defined(STM32F439NIHx) || defined(STM32F439VGTx) || \
            defined(STM32F439VITx) || defined(STM32F439ZGTx) || defined(STM32F439ZITx) || \
            defined(STM32F439ZGYx) || defined(STM32F439ZIYx) || defined(STM32F469AEHx) || \
            defined(STM32F469AGHx) || defined(STM32F469AIHx) || defined(STM32F469AEYx) || \
            defined(STM32F469AGYx) || defined(STM32F469AIYx) || defined(STM32F469BETx) || \
            defined(STM32F469BGTx) || defined(STM32F469BITx) || defined(STM32F469IETx) || \
            defined(STM32F469IGTx) || defined(STM32F469IEHx) || defined(STM32F469IGHx) || \
            defined(STM32F469IIHx) || defined(STM32F469IITx) || defined(STM32F469NEHx) || \
            defined(STM32F469NGHx) || defined(STM32F469NIHx) || defined(STM32F469VETx) || \
            defined(STM32F469VGTx) || defined(STM32F469VITx) || defined(STM32F469ZETx) || \
            defined(STM32F469ZGTx) || defined(STM32F469ZITx) || defined(STM32F479AGHx) || \
            defined(STM32F479AIHx) || defined(STM32F479AGYx) || defined(STM32F479AIYx) || \
            defined(STM32F479BGTx) || defined(STM32F479BITx) || defined(STM32F479IGHx) || \
            defined(STM32F479IIHx) || defined(STM32F479IGTx) || defined(STM32F479IITx) || \
            defined(STM32F479NGHx) || defined(STM32F479NIHx) || defined(STM32F479VGTx) || \
            defined(STM32F479VITx) || defined(STM32F479ZGTx) || defined(STM32F479ZITx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 0 )
                    return 8;
                if ( port == GPIOC && pos == 10 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOC && pos == 12 )
                    return 8;
            }
            if ( periph == UART7 ) {
                if ( port == GPIOE && pos == 8 )
                    return 8;
                if ( port == GPIOF && pos == 7 )
                    return 8;
            }
            if ( periph == UART8 ) {
                if ( port == GPIOE && pos == 1 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 7;
                if ( port == GPIOB && pos == 6 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 7;
                if ( port == GPIOD && pos == 5 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 10 )
                    return 7;
                if ( port == GPIOC && pos == 10 )
                    return 7;
                if ( port == GPIOD && pos == 8 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOC && pos == 6 )
                    return 8;
                if ( port == GPIOG && pos == 14 )
                    return 8;
            }
        #endif
        #if defined(STM32F446MCYx) || defined(STM32F446MEYx) || defined(STM32F446RCTx) || \
            defined(STM32F446RETx) || defined(STM32F446VCTx) || defined(STM32F446VETx) || \
            defined(STM32F446ZCHx) || defined(STM32F446ZEHx) || defined(STM32F446ZCJx) || \
            defined(STM32F446ZEJx) || defined(STM32F446ZCTx) || defined(STM32F446ZETx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 0 )
                    return 8;
                if ( port == GPIOC && pos == 10 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOC && pos == 12 )
                    return 8;
                if ( port == GPIOE && pos == 8 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 9 )
                    return 7;
                if ( port == GPIOB && pos == 6 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 2 )
                    return 7;
                if ( port == GPIOD && pos == 5 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 10 )
                    return 7;
                if ( port == GPIOC && pos == 10 )
                    return 7;
                if ( port == GPIOD && pos == 8 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOC && pos == 6 )
                    return 8;
                if ( port == GPIOG && pos == 14 )
                    return 8;
            }
        #endif
        impossible( "Incorrect TX pin" );
    }

    int uartAlternateFunRX( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if !defined(STM32F401CBUx) && !defined(STM32F401CCUx) && !defined(STM32F401CBYx) && \
            !defined(STM32F401CCYx) && !defined(STM32F401CDUx) && !defined(STM32F401CEUx) && \
            !defined(STM32F401CDYx) && !defined(STM32F401CEYx) && !defined(STM32F401CCFx) && \
            !defined(STM32F401RBTx) && !defined(STM32F401RCTx) && !defined(STM32F401RDTx) && \
            !defined(STM32F401RETx) && !defined(STM32F401VBHx) && !defined(STM32F401VCHx) && \
            !defined(STM32F401VBTx) && !defined(STM32F401VCTx) && !defined(STM32F401VDHx) && \
            !defined(STM32F401VEHx) && !defined(STM32F401VDTx) && !defined(STM32F401VETx) && \
            !defined(STM32F405OEYx) && !defined(STM32F405OGYx) && !defined(STM32F405RGTx) && \
            !defined(STM32F405VGTx) && !defined(STM32F405ZGTx) && !defined(STM32F415OGYx) && \
            !defined(STM32F415RGTx) && !defined(STM32F415VGTx) && !defined(STM32F415ZGTx) && \
            !defined(STM32F407IEHx) && !defined(STM32F407IGHx) && !defined(STM32F407IETx) && \
            !defined(STM32F407IGTx) && !defined(STM32F407VETx) && !defined(STM32F407VGTx) && \
            !defined(STM32F407ZETx) && !defined(STM32F407ZGTx) && !defined(STM32F417IEHx) && \
            !defined(STM32F417IGHx) && !defined(STM32F417IETx) && !defined(STM32F417IGTx) && \
            !defined(STM32F417VETx) && !defined(STM32F417VGTx) && !defined(STM32F417ZETx) && \
            !defined(STM32F417ZGTx) && !defined(STM32F410C8Tx) && !defined(STM32F410CBTx) && \
            !defined(STM32F410C8Ux) && !defined(STM32F410CBUx) && !defined(STM32F410R8Ix) && \
            !defined(STM32F410RBIx) && !defined(STM32F410R8Tx) && !defined(STM32F410RBTx) && \
            !defined(STM32F410T8Yx) && !defined(STM32F410TBYx) && !defined(STM32F411CCUx) && \
            !defined(STM32F411CEUx) && !defined(STM32F411CCYx) && !defined(STM32F411CEYx) && \
            !defined(STM32F411RCTx) && !defined(STM32F411RETx) && !defined(STM32F411VCHx) && \
            !defined(STM32F411VEHx) && !defined(STM32F411VCTx) && !defined(STM32F411VETx) && \
            !defined(STM32F412CEUx) && !defined(STM32F412CGUx) && !defined(STM32F412RETx) && \
            !defined(STM32F412RGTx) && !defined(STM32F412REYx) && !defined(STM32F412RGYx) && \
            !defined(STM32F412REYxP) && !defined(STM32F412RGYxP) && !defined(STM32F412VEHx) && \
            !defined(STM32F412VGHx) && !defined(STM32F412VETx) && !defined(STM32F412VGTx) && \
            !defined(STM32F412ZEJx) && !defined(STM32F412ZGJx) && !defined(STM32F412ZETx) && \
            !defined(STM32F412ZGTx) && !defined(STM32F413CGUx) && !defined(STM32F413CHUx) && \
            !defined(STM32F413MGYx) && !defined(STM32F413MHYx) && !defined(STM32F413RGTx) && \
            !defined(STM32F413RHTx) && !defined(STM32F413VGHx) && !defined(STM32F413VHHx) && \
            !defined(STM32F413VGTx) && !defined(STM32F413VHTx) && !defined(STM32F413ZGJx) && \
            !defined(STM32F413ZHJx) && !defined(STM32F413ZGTx) && !defined(STM32F413ZHTx) && \
            !defined(STM32F423CHUx) && !defined(STM32F423MHYx) && !defined(STM32F423RHTx) && \
            !defined(STM32F423VHHx) && !defined(STM32F423VHTx) && !defined(STM32F423ZHJx) && \
            !defined(STM32F423ZHTx) && !defined(STM32F427AGHx) && !defined(STM32F427AIHx) && \
            !defined(STM32F427IGHx) && !defined(STM32F427IIHx) && !defined(STM32F427IGTx) && \
            !defined(STM32F427IITx) && !defined(STM32F427VGTx) && !defined(STM32F427VITx) && \
            !defined(STM32F427ZGTx) && !defined(STM32F427ZITx) && !defined(STM32F437AIHx) && \
            !defined(STM32F437IGHx) && !defined(STM32F437IIHx) && !defined(STM32F437IGTx) && \
            !defined(STM32F437IITx) && !defined(STM32F437VGTx) && !defined(STM32F437VITx) && \
            !defined(STM32F437ZGTx) && !defined(STM32F437ZITx) && !defined(STM32F429AGHx) && \
            !defined(STM32F429AIHx) && !defined(STM32F429BETx) && !defined(STM32F429BGTx) && \
            !defined(STM32F429BITx) && !defined(STM32F429IETx) && !defined(STM32F429IGTx) && \
            !defined(STM32F429IEHx) && !defined(STM32F429IGHx) && !defined(STM32F429IIHx) && \
            !defined(STM32F429IITx) && !defined(STM32F429NEHx) && !defined(STM32F429NGHx) && \
            !defined(STM32F429NIHx) && !defined(STM32F429VETx) && !defined(STM32F429VGTx) && \
            !defined(STM32F429VITx) && !defined(STM32F429ZETx) && !defined(STM32F429ZGTx) && \
            !defined(STM32F429ZGYx) && !defined(STM32F429ZITx) && !defined(STM32F429ZIYx) && \
            !defined(STM32F439AIHx) && !defined(STM32F439BGTx) && !defined(STM32F439BITx) && \
            !defined(STM32F439IGHx) && !defined(STM32F439IIHx) && !defined(STM32F439IGTx) && \
            !defined(STM32F439IITx) && !defined(STM32F439NGHx) && !defined(STM32F439NIHx) && \
            !defined(STM32F439VGTx) && !defined(STM32F439VITx) && !defined(STM32F439ZGTx) && \
            !defined(STM32F439ZITx) && !defined(STM32F439ZGYx) && !defined(STM32F439ZIYx) && \
            !defined(STM32F446MCYx) && !defined(STM32F446MEYx) && !defined(STM32F446RCTx) && \
            !defined(STM32F446RETx) && !defined(STM32F446VCTx) && !defined(STM32F446VETx) && \
            !defined(STM32F446ZCHx) && !defined(STM32F446ZEHx) && !defined(STM32F446ZCJx) && \
            !defined(STM32F446ZEJx) && !defined(STM32F446ZCTx) && !defined(STM32F446ZETx) && \
            !defined(STM32F469AEHx) && !defined(STM32F469AGHx) && !defined(STM32F469AIHx) && \
            !defined(STM32F469AEYx) && !defined(STM32F469AGYx) && !defined(STM32F469AIYx) && \
            !defined(STM32F469BETx) && !defined(STM32F469BGTx) && !defined(STM32F469BITx) && \
            !defined(STM32F469IETx) && !defined(STM32F469IGTx) && !defined(STM32F469IEHx) && \
            !defined(STM32F469IGHx) && !defined(STM32F469IIHx) && !defined(STM32F469IITx) && \
            !defined(STM32F469NEHx) && !defined(STM32F469NGHx) && !defined(STM32F469NIHx) && \
            !defined(STM32F469VETx) && !defined(STM32F469VGTx) && !defined(STM32F469VITx) && \
            !defined(STM32F469ZETx) && !defined(STM32F469ZGTx) && !defined(STM32F469ZITx) && \
            !defined(STM32F479AGHx) && !defined(STM32F479AIHx) && !defined(STM32F479AGYx) && \
            !defined(STM32F479AIYx) && !defined(STM32F479BGTx) && !defined(STM32F479BITx) && \
            !defined(STM32F479IGHx) && !defined(STM32F479IIHx) && !defined(STM32F479IGTx) && \
            !defined(STM32F479IITx) && !defined(STM32F479NGHx) && !defined(STM32F479NIHx) && \
            !defined(STM32F479VGTx) && !defined(STM32F479VITx) && !defined(STM32F479ZGTx) && \
            !defined(STM32F479ZITx)
                #error Invalid MCU specified
        #endif
        #if defined(STM32F401CBUx) || defined(STM32F401CCUx) || defined(STM32F401CBYx) || \
            defined(STM32F401CCYx) || defined(STM32F401CDUx) || defined(STM32F401CEUx) || \
            defined(STM32F401CDYx) || defined(STM32F401CEYx) || defined(STM32F401CCFx) || \
            defined(STM32F401RBTx) || defined(STM32F401RCTx) || defined(STM32F401RDTx) || \
            defined(STM32F401RETx) || defined(STM32F401VBHx) || defined(STM32F401VCHx) || \
            defined(STM32F401VBTx) || defined(STM32F401VCTx) || defined(STM32F401VDHx) || \
            defined(STM32F401VEHx) || defined(STM32F401VDTx) || defined(STM32F401VETx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 7;
                if ( port == GPIOB && pos == 7 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 7;
                if ( port == GPIOD && pos == 6 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 12 )
                    return 8;
                if ( port == GPIOC && pos == 7 )
                    return 8;
            }
        #endif
        #if defined(STM32F405OEYx) || defined(STM32F405OGYx) || defined(STM32F405RGTx) || \
            defined(STM32F405VGTx) || defined(STM32F405ZGTx) || defined(STM32F415OGYx) || \
            defined(STM32F415RGTx) || defined(STM32F415VGTx) || defined(STM32F415ZGTx) || \
            defined(STM32F407IEHx) || defined(STM32F407IGHx) || defined(STM32F407IETx) || \
            defined(STM32F407IGTx) || defined(STM32F407VETx) || defined(STM32F407VGTx) || \
            defined(STM32F407ZETx) || defined(STM32F407ZGTx) || defined(STM32F417IEHx) || \
            defined(STM32F417IGHx) || defined(STM32F417IETx) || defined(STM32F417IGTx) || \
            defined(STM32F417VETx) || defined(STM32F417VGTx) || defined(STM32F417ZETx) || \
            defined(STM32F417ZGTx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 1 )
                    return 8;
                if ( port == GPIOC && pos == 11 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOD && pos == 2 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 7;
                if ( port == GPIOB && pos == 7 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 7;
                if ( port == GPIOD && pos == 6 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 11 )
                    return 7;
                if ( port == GPIOC && pos == 11 )
                    return 7;
                if ( port == GPIOD && pos == 9 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOC && pos == 7 )
                    return 8;
                if ( port == GPIOG && pos == 9 )
                    return 8;
            }
        #endif
        #if defined(STM32F410C8Tx) || defined(STM32F410CBTx) || defined(STM32F410C8Ux) || \
            defined(STM32F410CBUx) || defined(STM32F410R8Ix) || defined(STM32F410RBIx) || \
            defined(STM32F410R8Tx) || defined(STM32F410RBTx) || defined(STM32F410T8Yx) || \
            defined(STM32F410TBYx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 7;
                if ( port == GPIOB && pos == 3 )
                    return 7;
                if ( port == GPIOB && pos == 7 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 12 )
                    return 8;
                if ( port == GPIOC && pos == 7 )
                    return 8;
            }
        #endif
        #if defined(STM32F411CCUx) || defined(STM32F411CEUx) || defined(STM32F411CCYx) || \
            defined(STM32F411CEYx) || defined(STM32F411RCTx) || defined(STM32F411RETx) || \
            defined(STM32F411VCHx) || defined(STM32F411VEHx) || defined(STM32F411VCTx) || \
            defined(STM32F411VETx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 7;
                if ( port == GPIOB && pos == 3 )
                    return 7;
                if ( port == GPIOB && pos == 7 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 7;
                if ( port == GPIOD && pos == 6 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 12 )
                    return 8;
                if ( port == GPIOC && pos == 7 )
                    return 8;
            }
        #endif
        #if defined(STM32F412CEUx) || defined(STM32F412CGUx) || defined(STM32F412RETx) || \
            defined(STM32F412RGTx) || defined(STM32F412REYx) || defined(STM32F412RGYx) || \
            defined(STM32F412REYxP) || defined(STM32F412RGYxP) || defined(STM32F412VEHx) || \
            defined(STM32F412VGHx) || defined(STM32F412VETx) || defined(STM32F412VGTx) || \
            defined(STM32F412ZEJx) || defined(STM32F412ZGJx) || defined(STM32F412ZETx) || \
            defined(STM32F412ZGTx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 7;
                if ( port == GPIOB && pos == 3 )
                    return 7;
                if ( port == GPIOB && pos == 7 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 7;
                if ( port == GPIOD && pos == 6 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 11 )
                    return 7;
                if ( port == GPIOC && pos == 5 )
                    return 7;
                if ( port == GPIOC && pos == 11 )
                    return 7;
                if ( port == GPIOD && pos == 9 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 12 )
                    return 8;
                if ( port == GPIOC && pos == 7 )
                    return 8;
                if ( port == GPIOG && pos == 9 )
                    return 8;
            }
        #endif
        #if defined(STM32F413CGUx) || defined(STM32F413CHUx) || defined(STM32F413MGYx) || \
            defined(STM32F413MHYx) || defined(STM32F413RGTx) || defined(STM32F413RHTx) || \
            defined(STM32F413VGHx) || defined(STM32F413VHHx) || defined(STM32F413VGTx) || \
            defined(STM32F413VHTx) || defined(STM32F413ZGJx) || defined(STM32F413ZHJx) || \
            defined(STM32F413ZGTx) || defined(STM32F413ZHTx) || defined(STM32F423CHUx) || \
            defined(STM32F423MHYx) || defined(STM32F423RHTx) || defined(STM32F423VHHx) || \
            defined(STM32F423VHTx) || defined(STM32F423ZHJx) || defined(STM32F423ZHTx)
            if ( periph == UART10 ) {
                if ( port == GPIOE && pos == 2 )
                    return 11;
                if ( port == GPIOG && pos == 11 )
                    return 11;
            }
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 1 )
                    return 8;
                if ( port == GPIOA && pos == 11 )
                    return 11;
                if ( port == GPIOC && pos == 11 )
                    return 8;
                if ( port == GPIOD && pos == 0 )
                    return 11;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOB && pos == 5 )
                    return 11;
                if ( port == GPIOB && pos == 8 )
                    return 11;
                if ( port == GPIOB && pos == 12 )
                    return 11;
                if ( port == GPIOD && pos == 2 )
                    return 8;
            }
            if ( periph == UART7 ) {
                if ( port == GPIOA && pos == 8 )
                    return 8;
                if ( port == GPIOB && pos == 3 )
                    return 8;
                if ( port == GPIOE && pos == 7 )
                    return 8;
                if ( port == GPIOF && pos == 6 )
                    return 8;
            }
            if ( periph == UART8 ) {
                if ( port == GPIOE && pos == 0 )
                    return 8;
                if ( port == GPIOF && pos == 8 )
                    return 8;
            }
            if ( periph == UART9 ) {
                if ( port == GPIOD && pos == 14 )
                    return 11;
                if ( port == GPIOG && pos == 0 )
                    return 11;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 7;
                if ( port == GPIOB && pos == 3 )
                    return 7;
                if ( port == GPIOB && pos == 7 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 7;
                if ( port == GPIOD && pos == 6 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 11 )
                    return 7;
                if ( port == GPIOC && pos == 5 )
                    return 7;
                if ( port == GPIOC && pos == 11 )
                    return 7;
                if ( port == GPIOD && pos == 9 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOA && pos == 12 )
                    return 8;
                if ( port == GPIOC && pos == 7 )
                    return 8;
                if ( port == GPIOG && pos == 9 )
                    return 8;
            }
        #endif
        #if defined(STM32F427AGHx) || defined(STM32F427AIHx) || defined(STM32F427IGHx) || \
            defined(STM32F427IIHx) || defined(STM32F427IGTx) || defined(STM32F427IITx) || \
            defined(STM32F427VGTx) || defined(STM32F427VITx) || defined(STM32F427ZGTx) || \
            defined(STM32F427ZITx) || defined(STM32F437AIHx) || defined(STM32F437IGHx) || \
            defined(STM32F437IIHx) || defined(STM32F437IGTx) || defined(STM32F437IITx) || \
            defined(STM32F437VGTx) || defined(STM32F437VITx) || defined(STM32F437ZGTx) || \
            defined(STM32F437ZITx) || defined(STM32F429AGHx) || defined(STM32F429AIHx) || \
            defined(STM32F429BETx) || defined(STM32F429BGTx) || defined(STM32F429BITx) || \
            defined(STM32F429IETx) || defined(STM32F429IGTx) || defined(STM32F429IEHx) || \
            defined(STM32F429IGHx) || defined(STM32F429IIHx) || defined(STM32F429IITx) || \
            defined(STM32F429NEHx) || defined(STM32F429NGHx) || defined(STM32F429NIHx) || \
            defined(STM32F429VETx) || defined(STM32F429VGTx) || defined(STM32F429VITx) || \
            defined(STM32F429ZETx) || defined(STM32F429ZGTx) || defined(STM32F429ZGYx) || \
            defined(STM32F429ZITx) || defined(STM32F429ZIYx) || defined(STM32F439AIHx) || \
            defined(STM32F439BGTx) || defined(STM32F439BITx) || defined(STM32F439IGHx) || \
            defined(STM32F439IIHx) || defined(STM32F439IGTx) || defined(STM32F439IITx) || \
            defined(STM32F439NGHx) || defined(STM32F439NIHx) || defined(STM32F439VGTx) || \
            defined(STM32F439VITx) || defined(STM32F439ZGTx) || defined(STM32F439ZITx) || \
            defined(STM32F439ZGYx) || defined(STM32F439ZIYx) || defined(STM32F469AEHx) || \
            defined(STM32F469AGHx) || defined(STM32F469AIHx) || defined(STM32F469AEYx) || \
            defined(STM32F469AGYx) || defined(STM32F469AIYx) || defined(STM32F469BETx) || \
            defined(STM32F469BGTx) || defined(STM32F469BITx) || defined(STM32F469IETx) || \
            defined(STM32F469IGTx) || defined(STM32F469IEHx) || defined(STM32F469IGHx) || \
            defined(STM32F469IIHx) || defined(STM32F469IITx) || defined(STM32F469NEHx) || \
            defined(STM32F469NGHx) || defined(STM32F469NIHx) || defined(STM32F469VETx) || \
            defined(STM32F469VGTx) || defined(STM32F469VITx) || defined(STM32F469ZETx) || \
            defined(STM32F469ZGTx) || defined(STM32F469ZITx) || defined(STM32F479AGHx) || \
            defined(STM32F479AIHx) || defined(STM32F479AGYx) || defined(STM32F479AIYx) || \
            defined(STM32F479BGTx) || defined(STM32F479BITx) || defined(STM32F479IGHx) || \
            defined(STM32F479IIHx) || defined(STM32F479IGTx) || defined(STM32F479IITx) || \
            defined(STM32F479NGHx) || defined(STM32F479NIHx) || defined(STM32F479VGTx) || \
            defined(STM32F479VITx) || defined(STM32F479ZGTx) || defined(STM32F479ZITx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 1 )
                    return 8;
                if ( port == GPIOC && pos == 11 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOD && pos == 2 )
                    return 8;
            }
            if ( periph == UART7 ) {
                if ( port == GPIOE && pos == 7 )
                    return 8;
                if ( port == GPIOF && pos == 6 )
                    return 8;
            }
            if ( periph == UART8 ) {
                if ( port == GPIOE && pos == 0 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 7;
                if ( port == GPIOB && pos == 7 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 7;
                if ( port == GPIOD && pos == 6 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 11 )
                    return 7;
                if ( port == GPIOC && pos == 11 )
                    return 7;
                if ( port == GPIOD && pos == 9 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOC && pos == 7 )
                    return 8;
                if ( port == GPIOG && pos == 9 )
                    return 8;
            }
        #endif
        #if defined(STM32F446MCYx) || defined(STM32F446MEYx) || defined(STM32F446RCTx) || \
            defined(STM32F446RETx) || defined(STM32F446VCTx) || defined(STM32F446VETx) || \
            defined(STM32F446ZCHx) || defined(STM32F446ZEHx) || defined(STM32F446ZCJx) || \
            defined(STM32F446ZEJx) || defined(STM32F446ZCTx) || defined(STM32F446ZETx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 1 )
                    return 8;
                if ( port == GPIOC && pos == 11 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOD && pos == 2 )
                    return 8;
                if ( port == GPIOE && pos == 7 )
                    return 8;
            }
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 10 )
                    return 7;
                if ( port == GPIOB && pos == 7 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 3 )
                    return 7;
                if ( port == GPIOD && pos == 6 )
                    return 7;
            }
            if ( periph == USART3 ) {
                if ( port == GPIOB && pos == 11 )
                    return 7;
                if ( port == GPIOC && pos == 5 )
                    return 7;
                if ( port == GPIOC && pos == 11 )
                    return 7;
                if ( port == GPIOD && pos == 9 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOC && pos == 7 )
                    return 8;
                if ( port == GPIOG && pos == 9 )
                    return 8;
            }
        #endif
        impossible( "Incorrect RX pin" );
    }

    int uartAlternateFunCTS( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if !defined(STM32F401CBUx) && !defined(STM32F401CCUx) && !defined(STM32F401CBYx) && \
            !defined(STM32F401CCYx) && !defined(STM32F401CDUx) && !defined(STM32F401CEUx) && \
            !defined(STM32F401CDYx) && !defined(STM32F401CEYx) && !defined(STM32F401CCFx) && \
            !defined(STM32F401RBTx) && !defined(STM32F401RCTx) && !defined(STM32F401RDTx) && \
            !defined(STM32F401RETx) && !defined(STM32F401VBHx) && !defined(STM32F401VCHx) && \
            !defined(STM32F401VBTx) && !defined(STM32F401VCTx) && !defined(STM32F401VDHx) && \
            !defined(STM32F401VEHx) && !defined(STM32F401VDTx) && !defined(STM32F401VETx) && \
            !defined(STM32F405OEYx) && !defined(STM32F405OGYx) && !defined(STM32F405RGTx) && \
            !defined(STM32F405VGTx) && !defined(STM32F405ZGTx) && !defined(STM32F415OGYx) && \
            !defined(STM32F415RGTx) && !defined(STM32F415VGTx) && !defined(STM32F415ZGTx) && \
            !defined(STM32F407IEHx) && !defined(STM32F407IGHx) && !defined(STM32F407IETx) && \
            !defined(STM32F407IGTx) && !defined(STM32F407VETx) && !defined(STM32F407VGTx) && \
            !defined(STM32F407ZETx) && !defined(STM32F407ZGTx) && !defined(STM32F417IEHx) && \
            !defined(STM32F417IGHx) && !defined(STM32F417IETx) && !defined(STM32F417IGTx) && \
            !defined(STM32F417VETx) && !defined(STM32F417VGTx) && !defined(STM32F417ZETx) && \
            !defined(STM32F417ZGTx) && !defined(STM32F410C8Tx) && !defined(STM32F410CBTx) && \
            !defined(STM32F410C8Ux) && !defined(STM32F410CBUx) && !defined(STM32F410R8Ix) && \
            !defined(STM32F410RBIx) && !defined(STM32F410R8Tx) && !defined(STM32F410RBTx) && \
            !defined(STM32F410T8Yx) && !defined(STM32F410TBYx) && !defined(STM32F411CCUx) && \
            !defined(STM32F411CEUx) && !defined(STM32F411CCYx) && !defined(STM32F411CEYx) && \
            !defined(STM32F411RCTx) && !defined(STM32F411RETx) && !defined(STM32F411VCHx) && \
            !defined(STM32F411VEHx) && !defined(STM32F411VCTx) && !defined(STM32F411VETx) && \
            !defined(STM32F412CEUx) && !defined(STM32F412CGUx) && !defined(STM32F412RETx) && \
            !defined(STM32F412RGTx) && !defined(STM32F412REYx) && !defined(STM32F412RGYx) && \
            !defined(STM32F412REYxP) && !defined(STM32F412RGYxP) && !defined(STM32F412VEHx) && \
            !defined(STM32F412VGHx) && !defined(STM32F412VETx) && !defined(STM32F412VGTx) && \
            !defined(STM32F412ZEJx) && !defined(STM32F412ZGJx) && !defined(STM32F412ZETx) && \
            !defined(STM32F412ZGTx) && !defined(STM32F413CGUx) && !defined(STM32F413CHUx) && \
            !defined(STM32F413MGYx) && !defined(STM32F413MHYx) && !defined(STM32F413RGTx) && \
            !defined(STM32F413RHTx) && !defined(STM32F413VGHx) && !defined(STM32F413VHHx) && \
            !defined(STM32F413VGTx) && !defined(STM32F413VHTx) && !defined(STM32F413ZGJx) && \
            !defined(STM32F413ZHJx) && !defined(STM32F413ZGTx) && !defined(STM32F413ZHTx) && \
            !defined(STM32F423CHUx) && !defined(STM32F423MHYx) && !defined(STM32F423RHTx) && \
            !defined(STM32F423VHHx) && !defined(STM32F423VHTx) && !defined(STM32F423ZHJx) && \
            !defined(STM32F423ZHTx) && !defined(STM32F427AGHx) && !defined(STM32F427AIHx) && \
            !defined(STM32F427IGHx) && !defined(STM32F427IIHx) && !defined(STM32F427IGTx) && \
            !defined(STM32F427IITx) && !defined(STM32F427VGTx) && !defined(STM32F427VITx) && \
            !defined(STM32F427ZGTx) && !defined(STM32F427ZITx) && !defined(STM32F437AIHx) && \
            !defined(STM32F437IGHx) && !defined(STM32F437IIHx) && !defined(STM32F437IGTx) && \
            !defined(STM32F437IITx) && !defined(STM32F437VGTx) && !defined(STM32F437VITx) && \
            !defined(STM32F437ZGTx) && !defined(STM32F437ZITx) && !defined(STM32F429AGHx) && \
            !defined(STM32F429AIHx) && !defined(STM32F429BETx) && !defined(STM32F429BGTx) && \
            !defined(STM32F429BITx) && !defined(STM32F429IETx) && !defined(STM32F429IGTx) && \
            !defined(STM32F429IEHx) && !defined(STM32F429IGHx) && !defined(STM32F429IIHx) && \
            !defined(STM32F429IITx) && !defined(STM32F429NEHx) && !defined(STM32F429NGHx) && \
            !defined(STM32F429NIHx) && !defined(STM32F429VETx) && !defined(STM32F429VGTx) && \
            !defined(STM32F429VITx) && !defined(STM32F429ZETx) && !defined(STM32F429ZGTx) && \
            !defined(STM32F429ZGYx) && !defined(STM32F429ZITx) && !defined(STM32F429ZIYx) && \
            !defined(STM32F439AIHx) && !defined(STM32F439BGTx) && !defined(STM32F439BITx) && \
            !defined(STM32F439IGHx) && !defined(STM32F439IIHx) && !defined(STM32F439IGTx) && \
            !defined(STM32F439IITx) && !defined(STM32F439NGHx) && !defined(STM32F439NIHx) && \
            !defined(STM32F439VGTx) && !defined(STM32F439VITx) && !defined(STM32F439ZGTx) && \
            !defined(STM32F439ZITx) && !defined(STM32F439ZGYx) && !defined(STM32F439ZIYx) && \
            !defined(STM32F446MCYx) && !defined(STM32F446MEYx) && !defined(STM32F446RCTx) && \
            !defined(STM32F446RETx) && !defined(STM32F446VCTx) && !defined(STM32F446VETx) && \
            !defined(STM32F446ZCHx) && !defined(STM32F446ZEHx) && !defined(STM32F446ZCJx) && \
            !defined(STM32F446ZEJx) && !defined(STM32F446ZCTx) && !defined(STM32F446ZETx) && \
            !defined(STM32F469AEHx) && !defined(STM32F469AGHx) && !defined(STM32F469AIHx) && \
            !defined(STM32F469AEYx) && !defined(STM32F469AGYx) && !defined(STM32F469AIYx) && \
            !defined(STM32F469BETx) && !defined(STM32F469BGTx) && !defined(STM32F469BITx) && \
            !defined(STM32F469IETx) && !defined(STM32F469IGTx) && !defined(STM32F469IEHx) && \
            !defined(STM32F469IGHx) && !defined(STM32F469IIHx) && !defined(STM32F469IITx) && \
            !defined(STM32F469NEHx) && !defined(STM32F469NGHx) && !defined(STM32F469NIHx) && \
            !defined(STM32F469VETx) && !defined(STM32F469VGTx) && !defined(STM32F469VITx) && \
            !defined(STM32F469ZETx) && !defined(STM32F469ZGTx) && !defined(STM32F469ZITx) && \
            !defined(STM32F479AGHx) && !defined(STM32F479AIHx) && !defined(STM32F479AGYx) && \
            !defined(STM32F479AIYx) && !defined(STM32F479BGTx) && !defined(STM32F479BITx) && \
            !defined(STM32F479IGHx) && !defined(STM32F479IIHx) && !defined(STM32F479IGTx) && \
            !defined(STM32F479IITx) && !defined(STM32F479NGHx) && !defined(STM32F479NIHx) && \
            !defined(STM32F479VGTx) && !defined(STM32F479VITx) && !defined(STM32F479ZGTx) && \
            !defined(STM32F479ZITx)
                #error Invalid MCU specified
        #endif
        #if defined(STM32F401CBUx) || defined(STM32F401CCUx) || defined(STM32F401CBYx) || \
            defined(STM32F401CCYx) || defined(STM32F401CDUx) || defined(STM32F401CEUx) || \
            defined(STM32F401CDYx) || defined(STM32F401CEYx) || defined(STM32F401CCFx) || \
            defined(STM32F401RBTx) || defined(STM32F401RCTx) || defined(STM32F401RDTx) || \
            defined(STM32F401RETx) || defined(STM32F401VBHx) || defined(STM32F401VCHx) || \
            defined(STM32F401VBTx) || defined(STM32F401VCTx) || defined(STM32F401VDHx) || \
            defined(STM32F401VEHx) || defined(STM32F401VDTx) || defined(STM32F401VETx) || \
            defined(STM32F411CCUx) || defined(STM32F411CEUx) || defined(STM32F411CCYx) || \
            defined(STM32F411CEYx) || defined(STM32F411RCTx) || defined(STM32F411RETx) || \
            defined(STM32F411VCHx) || defined(STM32F411VEHx) || defined(STM32F411VCTx) || \
            defined(STM32F411VETx)
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
        #endif
        #if defined(STM32F405OEYx) || defined(STM32F405OGYx) || defined(STM32F405RGTx) || \
            defined(STM32F405VGTx) || defined(STM32F405ZGTx) || defined(STM32F415OGYx) || \
            defined(STM32F415RGTx) || defined(STM32F415VGTx) || defined(STM32F415ZGTx) || \
            defined(STM32F407IEHx) || defined(STM32F407IGHx) || defined(STM32F407IETx) || \
            defined(STM32F407IGTx) || defined(STM32F407VETx) || defined(STM32F407VGTx) || \
            defined(STM32F407ZETx) || defined(STM32F407ZGTx) || defined(STM32F417IEHx) || \
            defined(STM32F417IGHx) || defined(STM32F417IETx) || defined(STM32F417IGTx) || \
            defined(STM32F417VETx) || defined(STM32F417VGTx) || defined(STM32F417ZETx) || \
            defined(STM32F417ZGTx)
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
                if ( port == GPIOB && pos == 13 )
                    return 7;
                if ( port == GPIOD && pos == 11 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOG && pos == 13 )
                    return 8;
                if ( port == GPIOG && pos == 15 )
                    return 8;
            }
        #endif
        #if defined(STM32F410C8Tx) || defined(STM32F410CBTx) || defined(STM32F410C8Ux) || \
            defined(STM32F410CBUx) || defined(STM32F410R8Ix) || defined(STM32F410RBIx) || \
            defined(STM32F410R8Tx) || defined(STM32F410RBTx) || defined(STM32F410T8Yx) || \
            defined(STM32F410TBYx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 11 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 0 )
                    return 7;
            }
        #endif
        #if defined(STM32F412CEUx) || defined(STM32F412CGUx) || defined(STM32F412RETx) || \
            defined(STM32F412RGTx) || defined(STM32F412REYx) || defined(STM32F412RGYx) || \
            defined(STM32F412REYxP) || defined(STM32F412RGYxP) || defined(STM32F412VEHx) || \
            defined(STM32F412VGHx) || defined(STM32F412VETx) || defined(STM32F412VGTx) || \
            defined(STM32F412ZEJx) || defined(STM32F412ZGJx) || defined(STM32F412ZETx) || \
            defined(STM32F412ZGTx) || defined(STM32F413CGUx) || defined(STM32F413CHUx) || \
            defined(STM32F413MGYx) || defined(STM32F413MHYx) || defined(STM32F413RGTx) || \
            defined(STM32F413RHTx) || defined(STM32F413VGHx) || defined(STM32F413VHHx) || \
            defined(STM32F413VGTx) || defined(STM32F413VHTx) || defined(STM32F413ZGJx) || \
            defined(STM32F413ZHJx) || defined(STM32F413ZGTx) || defined(STM32F413ZHTx) || \
            defined(STM32F423CHUx) || defined(STM32F423MHYx) || defined(STM32F423RHTx) || \
            defined(STM32F423VHHx) || defined(STM32F423VHTx) || defined(STM32F423ZHJx) || \
            defined(STM32F423ZHTx)
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
                if ( port == GPIOB && pos == 13 )
                    return 8;
                if ( port == GPIOD && pos == 11 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOG && pos == 13 )
                    return 8;
                if ( port == GPIOG && pos == 15 )
                    return 8;
            }
        #endif
        #if defined(STM32F427AGHx) || defined(STM32F427AIHx) || defined(STM32F427IGHx) || \
            defined(STM32F427IIHx) || defined(STM32F427IGTx) || defined(STM32F427IITx) || \
            defined(STM32F427VGTx) || defined(STM32F427VITx) || defined(STM32F427ZGTx) || \
            defined(STM32F427ZITx) || defined(STM32F437AIHx) || defined(STM32F437IGHx) || \
            defined(STM32F437IIHx) || defined(STM32F437IGTx) || defined(STM32F437IITx) || \
            defined(STM32F437VGTx) || defined(STM32F437VITx) || defined(STM32F437ZGTx) || \
            defined(STM32F437ZITx) || defined(STM32F429AGHx) || defined(STM32F429AIHx) || \
            defined(STM32F429BETx) || defined(STM32F429BGTx) || defined(STM32F429BITx) || \
            defined(STM32F429IETx) || defined(STM32F429IGTx) || defined(STM32F429IEHx) || \
            defined(STM32F429IGHx) || defined(STM32F429IIHx) || defined(STM32F429IITx) || \
            defined(STM32F429NEHx) || defined(STM32F429NGHx) || defined(STM32F429NIHx) || \
            defined(STM32F429VETx) || defined(STM32F429VGTx) || defined(STM32F429VITx) || \
            defined(STM32F429ZETx) || defined(STM32F429ZGTx) || defined(STM32F429ZGYx) || \
            defined(STM32F429ZITx) || defined(STM32F429ZIYx) || defined(STM32F439AIHx) || \
            defined(STM32F439BGTx) || defined(STM32F439BITx) || defined(STM32F439IGHx) || \
            defined(STM32F439IIHx) || defined(STM32F439IGTx) || defined(STM32F439IITx) || \
            defined(STM32F439NGHx) || defined(STM32F439NIHx) || defined(STM32F439VGTx) || \
            defined(STM32F439VITx) || defined(STM32F439ZGTx) || defined(STM32F439ZITx) || \
            defined(STM32F439ZGYx) || defined(STM32F439ZIYx) || defined(STM32F469AEHx) || \
            defined(STM32F469AGHx) || defined(STM32F469AIHx) || defined(STM32F469AEYx) || \
            defined(STM32F469AGYx) || defined(STM32F469AIYx) || defined(STM32F469BETx) || \
            defined(STM32F469BGTx) || defined(STM32F469BITx) || defined(STM32F469IETx) || \
            defined(STM32F469IGTx) || defined(STM32F469IEHx) || defined(STM32F469IGHx) || \
            defined(STM32F469IIHx) || defined(STM32F469IITx) || defined(STM32F469NEHx) || \
            defined(STM32F469NGHx) || defined(STM32F469NIHx) || defined(STM32F469VETx) || \
            defined(STM32F469VGTx) || defined(STM32F469VITx) || defined(STM32F469ZETx) || \
            defined(STM32F469ZGTx) || defined(STM32F469ZITx) || defined(STM32F479AGHx) || \
            defined(STM32F479AIHx) || defined(STM32F479AGYx) || defined(STM32F479AIYx) || \
            defined(STM32F479BGTx) || defined(STM32F479BITx) || defined(STM32F479IGHx) || \
            defined(STM32F479IIHx) || defined(STM32F479IGTx) || defined(STM32F479IITx) || \
            defined(STM32F479NGHx) || defined(STM32F479NIHx) || defined(STM32F479VGTx) || \
            defined(STM32F479VITx) || defined(STM32F479ZGTx) || defined(STM32F479ZITx)
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
                if ( port == GPIOB && pos == 13 )
                    return 7;
                if ( port == GPIOD && pos == 11 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOG && pos == 13 )
                    return 8;
                if ( port == GPIOG && pos == 15 )
                    return 8;
            }
        #endif
        #if defined(STM32F446MCYx) || defined(STM32F446MEYx) || defined(STM32F446RCTx) || \
            defined(STM32F446RETx) || defined(STM32F446VCTx) || defined(STM32F446VETx) || \
            defined(STM32F446ZCHx) || defined(STM32F446ZEHx) || defined(STM32F446ZCJx) || \
            defined(STM32F446ZEJx) || defined(STM32F446ZCTx) || defined(STM32F446ZETx)
            if ( periph == UART4 ) {
                if ( port == GPIOB && pos == 0 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOC && pos == 9 )
                    return 7;
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
                if ( port == GPIOB && pos == 13 )
                    return 7;
                if ( port == GPIOD && pos == 11 )
                    return 7;
            }
            if ( periph == USART6 ) {
                if ( port == GPIOG && pos == 13 )
                    return 8;
                if ( port == GPIOG && pos == 15 )
                    return 8;
            }
        #endif
        impossible( "Incorrect CTS pin" );
    }

    int uartAlternateFunRTS( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if !defined(STM32F401CBUx) && !defined(STM32F401CCUx) && !defined(STM32F401CBYx) && \
            !defined(STM32F401CCYx) && !defined(STM32F401CDUx) && !defined(STM32F401CEUx) && \
            !defined(STM32F401CDYx) && !defined(STM32F401CEYx) && !defined(STM32F401CCFx) && \
            !defined(STM32F401RBTx) && !defined(STM32F401RCTx) && !defined(STM32F401RDTx) && \
            !defined(STM32F401RETx) && !defined(STM32F401VBHx) && !defined(STM32F401VCHx) && \
            !defined(STM32F401VBTx) && !defined(STM32F401VCTx) && !defined(STM32F401VDHx) && \
            !defined(STM32F401VEHx) && !defined(STM32F401VDTx) && !defined(STM32F401VETx) && \
            !defined(STM32F405OEYx) && !defined(STM32F405OGYx) && !defined(STM32F405RGTx) && \
            !defined(STM32F405VGTx) && !defined(STM32F405ZGTx) && !defined(STM32F415OGYx) && \
            !defined(STM32F415RGTx) && !defined(STM32F415VGTx) && !defined(STM32F415ZGTx) && \
            !defined(STM32F407IEHx) && !defined(STM32F407IGHx) && !defined(STM32F407IETx) && \
            !defined(STM32F407IGTx) && !defined(STM32F407VETx) && !defined(STM32F407VGTx) && \
            !defined(STM32F407ZETx) && !defined(STM32F407ZGTx) && !defined(STM32F417IEHx) && \
            !defined(STM32F417IGHx) && !defined(STM32F417IETx) && !defined(STM32F417IGTx) && \
            !defined(STM32F417VETx) && !defined(STM32F417VGTx) && !defined(STM32F417ZETx) && \
            !defined(STM32F417ZGTx) && !defined(STM32F410C8Tx) && !defined(STM32F410CBTx) && \
            !defined(STM32F410C8Ux) && !defined(STM32F410CBUx) && !defined(STM32F410R8Ix) && \
            !defined(STM32F410RBIx) && !defined(STM32F410R8Tx) && !defined(STM32F410RBTx) && \
            !defined(STM32F410T8Yx) && !defined(STM32F410TBYx) && !defined(STM32F411CCUx) && \
            !defined(STM32F411CEUx) && !defined(STM32F411CCYx) && !defined(STM32F411CEYx) && \
            !defined(STM32F411RCTx) && !defined(STM32F411RETx) && !defined(STM32F411VCHx) && \
            !defined(STM32F411VEHx) && !defined(STM32F411VCTx) && !defined(STM32F411VETx) && \
            !defined(STM32F412CEUx) && !defined(STM32F412CGUx) && !defined(STM32F412RETx) && \
            !defined(STM32F412RGTx) && !defined(STM32F412REYx) && !defined(STM32F412RGYx) && \
            !defined(STM32F412REYxP) && !defined(STM32F412RGYxP) && !defined(STM32F412VEHx) && \
            !defined(STM32F412VGHx) && !defined(STM32F412VETx) && !defined(STM32F412VGTx) && \
            !defined(STM32F412ZEJx) && !defined(STM32F412ZGJx) && !defined(STM32F412ZETx) && \
            !defined(STM32F412ZGTx) && !defined(STM32F413CGUx) && !defined(STM32F413CHUx) && \
            !defined(STM32F413MGYx) && !defined(STM32F413MHYx) && !defined(STM32F413RGTx) && \
            !defined(STM32F413RHTx) && !defined(STM32F413VGHx) && !defined(STM32F413VHHx) && \
            !defined(STM32F413VGTx) && !defined(STM32F413VHTx) && !defined(STM32F413ZGJx) && \
            !defined(STM32F413ZHJx) && !defined(STM32F413ZGTx) && !defined(STM32F413ZHTx) && \
            !defined(STM32F423CHUx) && !defined(STM32F423MHYx) && !defined(STM32F423RHTx) && \
            !defined(STM32F423VHHx) && !defined(STM32F423VHTx) && !defined(STM32F423ZHJx) && \
            !defined(STM32F423ZHTx) && !defined(STM32F427AGHx) && !defined(STM32F427AIHx) && \
            !defined(STM32F427IGHx) && !defined(STM32F427IIHx) && !defined(STM32F427IGTx) && \
            !defined(STM32F427IITx) && !defined(STM32F427VGTx) && !defined(STM32F427VITx) && \
            !defined(STM32F427ZGTx) && !defined(STM32F427ZITx) && !defined(STM32F437AIHx) && \
            !defined(STM32F437IGHx) && !defined(STM32F437IIHx) && !defined(STM32F437IGTx) && \
            !defined(STM32F437IITx) && !defined(STM32F437VGTx) && !defined(STM32F437VITx) && \
            !defined(STM32F437ZGTx) && !defined(STM32F437ZITx) && !defined(STM32F429AGHx) && \
            !defined(STM32F429AIHx) && !defined(STM32F429BETx) && !defined(STM32F429BGTx) && \
            !defined(STM32F429BITx) && !defined(STM32F429IETx) && !defined(STM32F429IGTx) && \
            !defined(STM32F429IEHx) && !defined(STM32F429IGHx) && !defined(STM32F429IIHx) && \
            !defined(STM32F429IITx) && !defined(STM32F429NEHx) && !defined(STM32F429NGHx) && \
            !defined(STM32F429NIHx) && !defined(STM32F429VETx) && !defined(STM32F429VGTx) && \
            !defined(STM32F429VITx) && !defined(STM32F429ZETx) && !defined(STM32F429ZGTx) && \
            !defined(STM32F429ZGYx) && !defined(STM32F429ZITx) && !defined(STM32F429ZIYx) && \
            !defined(STM32F439AIHx) && !defined(STM32F439BGTx) && !defined(STM32F439BITx) && \
            !defined(STM32F439IGHx) && !defined(STM32F439IIHx) && !defined(STM32F439IGTx) && \
            !defined(STM32F439IITx) && !defined(STM32F439NGHx) && !defined(STM32F439NIHx) && \
            !defined(STM32F439VGTx) && !defined(STM32F439VITx) && !defined(STM32F439ZGTx) && \
            !defined(STM32F439ZITx) && !defined(STM32F439ZGYx) && !defined(STM32F439ZIYx) && \
            !defined(STM32F446MCYx) && !defined(STM32F446MEYx) && !defined(STM32F446RCTx) && \
            !defined(STM32F446RETx) && !defined(STM32F446VCTx) && !defined(STM32F446VETx) && \
            !defined(STM32F446ZCHx) && !defined(STM32F446ZEHx) && !defined(STM32F446ZCJx) && \
            !defined(STM32F446ZEJx) && !defined(STM32F446ZCTx) && !defined(STM32F446ZETx) && \
            !defined(STM32F469AEHx) && !defined(STM32F469AGHx) && !defined(STM32F469AIHx) && \
            !defined(STM32F469AEYx) && !defined(STM32F469AGYx) && !defined(STM32F469AIYx) && \
            !defined(STM32F469BETx) && !defined(STM32F469BGTx) && !defined(STM32F469BITx) && \
            !defined(STM32F469IETx) && !defined(STM32F469IGTx) && !defined(STM32F469IEHx) && \
            !defined(STM32F469IGHx) && !defined(STM32F469IIHx) && !defined(STM32F469IITx) && \
            !defined(STM32F469NEHx) && !defined(STM32F469NGHx) && !defined(STM32F469NIHx) && \
            !defined(STM32F469VETx) && !defined(STM32F469VGTx) && !defined(STM32F469VITx) && \
            !defined(STM32F469ZETx) && !defined(STM32F469ZGTx) && !defined(STM32F469ZITx) && \
            !defined(STM32F479AGHx) && !defined(STM32F479AIHx) && !defined(STM32F479AGYx) && \
            !defined(STM32F479AIYx) && !defined(STM32F479BGTx) && !defined(STM32F479BITx) && \
            !defined(STM32F479IGHx) && !defined(STM32F479IIHx) && !defined(STM32F479IGTx) && \
            !defined(STM32F479IITx) && !defined(STM32F479NGHx) && !defined(STM32F479NIHx) && \
            !defined(STM32F479VGTx) && !defined(STM32F479VITx) && !defined(STM32F479ZGTx) && \
            !defined(STM32F479ZITx)
                #error Invalid MCU specified
        #endif
        #if defined(STM32F401CBUx) || defined(STM32F401CCUx) || defined(STM32F401CBYx) || \
            defined(STM32F401CCYx) || defined(STM32F401CDUx) || defined(STM32F401CEUx) || \
            defined(STM32F401CDYx) || defined(STM32F401CEYx) || defined(STM32F401CCFx) || \
            defined(STM32F401RBTx) || defined(STM32F401RCTx) || defined(STM32F401RDTx) || \
            defined(STM32F401RETx) || defined(STM32F401VBHx) || defined(STM32F401VCHx) || \
            defined(STM32F401VBTx) || defined(STM32F401VCTx) || defined(STM32F401VDHx) || \
            defined(STM32F401VEHx) || defined(STM32F401VDTx) || defined(STM32F401VETx) || \
            defined(STM32F411CCUx) || defined(STM32F411CEUx) || defined(STM32F411CCYx) || \
            defined(STM32F411CEYx) || defined(STM32F411RCTx) || defined(STM32F411RETx) || \
            defined(STM32F411VCHx) || defined(STM32F411VEHx) || defined(STM32F411VCTx) || \
            defined(STM32F411VETx)
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
        #endif
        #if defined(STM32F405OEYx) || defined(STM32F405OGYx) || defined(STM32F405RGTx) || \
            defined(STM32F405VGTx) || defined(STM32F405ZGTx) || defined(STM32F415OGYx) || \
            defined(STM32F415RGTx) || defined(STM32F415VGTx) || defined(STM32F415ZGTx) || \
            defined(STM32F407IEHx) || defined(STM32F407IGHx) || defined(STM32F407IETx) || \
            defined(STM32F407IGTx) || defined(STM32F407VETx) || defined(STM32F407VGTx) || \
            defined(STM32F407ZETx) || defined(STM32F407ZGTx) || defined(STM32F417IEHx) || \
            defined(STM32F417IGHx) || defined(STM32F417IETx) || defined(STM32F417IGTx) || \
            defined(STM32F417VETx) || defined(STM32F417VGTx) || defined(STM32F417ZETx) || \
            defined(STM32F417ZGTx) || defined(STM32F412CEUx) || defined(STM32F412CGUx) || \
            defined(STM32F412RETx) || defined(STM32F412RGTx) || defined(STM32F412REYx) || \
            defined(STM32F412RGYx) || defined(STM32F412REYxP) || defined(STM32F412RGYxP) || \
            defined(STM32F412VEHx) || defined(STM32F412VGHx) || defined(STM32F412VETx) || \
            defined(STM32F412VGTx) || defined(STM32F412ZEJx) || defined(STM32F412ZGJx) || \
            defined(STM32F412ZETx) || defined(STM32F412ZGTx) || defined(STM32F413CGUx) || \
            defined(STM32F413CHUx) || defined(STM32F413MGYx) || defined(STM32F413MHYx) || \
            defined(STM32F413RGTx) || defined(STM32F413RHTx) || defined(STM32F413VGHx) || \
            defined(STM32F413VHHx) || defined(STM32F413VGTx) || defined(STM32F413VHTx) || \
            defined(STM32F413ZGJx) || defined(STM32F413ZHJx) || defined(STM32F413ZGTx) || \
            defined(STM32F413ZHTx) || defined(STM32F423CHUx) || defined(STM32F423MHYx) || \
            defined(STM32F423RHTx) || defined(STM32F423VHHx) || defined(STM32F423VHTx) || \
            defined(STM32F423ZHJx) || defined(STM32F423ZHTx) || defined(STM32F427AGHx) || \
            defined(STM32F427AIHx) || defined(STM32F427IGHx) || defined(STM32F427IIHx) || \
            defined(STM32F427IGTx) || defined(STM32F427IITx) || defined(STM32F427VGTx) || \
            defined(STM32F427VITx) || defined(STM32F427ZGTx) || defined(STM32F427ZITx) || \
            defined(STM32F437AIHx) || defined(STM32F437IGHx) || defined(STM32F437IIHx) || \
            defined(STM32F437IGTx) || defined(STM32F437IITx) || defined(STM32F437VGTx) || \
            defined(STM32F437VITx) || defined(STM32F437ZGTx) || defined(STM32F437ZITx) || \
            defined(STM32F429AGHx) || defined(STM32F429AIHx) || defined(STM32F429BETx) || \
            defined(STM32F429BGTx) || defined(STM32F429BITx) || defined(STM32F429IETx) || \
            defined(STM32F429IGTx) || defined(STM32F429IEHx) || defined(STM32F429IGHx) || \
            defined(STM32F429IIHx) || defined(STM32F429IITx) || defined(STM32F429NEHx) || \
            defined(STM32F429NGHx) || defined(STM32F429NIHx) || defined(STM32F429VETx) || \
            defined(STM32F429VGTx) || defined(STM32F429VITx) || defined(STM32F429ZETx) || \
            defined(STM32F429ZGTx) || defined(STM32F429ZGYx) || defined(STM32F429ZITx) || \
            defined(STM32F429ZIYx) || defined(STM32F439AIHx) || defined(STM32F439BGTx) || \
            defined(STM32F439BITx) || defined(STM32F439IGHx) || defined(STM32F439IIHx) || \
            defined(STM32F439IGTx) || defined(STM32F439IITx) || defined(STM32F439NGHx) || \
            defined(STM32F439NIHx) || defined(STM32F439VGTx) || defined(STM32F439VITx) || \
            defined(STM32F439ZGTx) || defined(STM32F439ZITx) || defined(STM32F439ZGYx) || \
            defined(STM32F439ZIYx) || defined(STM32F469AEHx) || defined(STM32F469AGHx) || \
            defined(STM32F469AIHx) || defined(STM32F469AEYx) || defined(STM32F469AGYx) || \
            defined(STM32F469AIYx) || defined(STM32F469BETx) || defined(STM32F469BGTx) || \
            defined(STM32F469BITx) || defined(STM32F469IETx) || defined(STM32F469IGTx) || \
            defined(STM32F469IEHx) || defined(STM32F469IGHx) || defined(STM32F469IIHx) || \
            defined(STM32F469IITx) || defined(STM32F469NEHx) || defined(STM32F469NGHx) || \
            defined(STM32F469NIHx) || defined(STM32F469VETx) || defined(STM32F469VGTx) || \
            defined(STM32F469VITx) || defined(STM32F469ZETx) || defined(STM32F469ZGTx) || \
            defined(STM32F469ZITx) || defined(STM32F479AGHx) || defined(STM32F479AIHx) || \
            defined(STM32F479AGYx) || defined(STM32F479AIYx) || defined(STM32F479BGTx) || \
            defined(STM32F479BITx) || defined(STM32F479IGHx) || defined(STM32F479IIHx) || \
            defined(STM32F479IGTx) || defined(STM32F479IITx) || defined(STM32F479NGHx) || \
            defined(STM32F479NIHx) || defined(STM32F479VGTx) || defined(STM32F479VITx) || \
            defined(STM32F479ZGTx) || defined(STM32F479ZITx)
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
            if ( periph == USART6 ) {
                if ( port == GPIOG && pos == 8 )
                    return 8;
                if ( port == GPIOG && pos == 12 )
                    return 8;
            }
        #endif
        #if defined(STM32F410C8Tx) || defined(STM32F410CBTx) || defined(STM32F410C8Ux) || \
            defined(STM32F410CBUx) || defined(STM32F410R8Ix) || defined(STM32F410RBIx) || \
            defined(STM32F410R8Tx) || defined(STM32F410RBTx) || defined(STM32F410T8Yx) || \
            defined(STM32F410TBYx)
            if ( periph == USART1 ) {
                if ( port == GPIOA && pos == 12 )
                    return 7;
            }
            if ( periph == USART2 ) {
                if ( port == GPIOA && pos == 1 )
                    return 7;
            }
        #endif
        #if defined(STM32F446MCYx) || defined(STM32F446MEYx) || defined(STM32F446RCTx) || \
            defined(STM32F446RETx) || defined(STM32F446VCTx) || defined(STM32F446VETx) || \
            defined(STM32F446ZCHx) || defined(STM32F446ZEHx) || defined(STM32F446ZCJx) || \
            defined(STM32F446ZEJx) || defined(STM32F446ZCTx) || defined(STM32F446ZETx)
            if ( periph == UART4 ) {
                if ( port == GPIOA && pos == 15 )
                    return 8;
            }
            if ( periph == UART5 ) {
                if ( port == GPIOC && pos == 8 )
                    return 7;
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
            if ( periph == USART6 ) {
                if ( port == GPIOG && pos == 8 )
                    return 8;
                if ( port == GPIOG && pos == 12 )
                    return 8;
            }
        #endif
        impossible( "Incorrect RTS pin" );
    }

    int uartAlternateFunDE( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {
        #if !defined(STM32F401CBUx) && !defined(STM32F401CCUx) && !defined(STM32F401CBYx) && \
            !defined(STM32F401CCYx) && !defined(STM32F401CDUx) && !defined(STM32F401CEUx) && \
            !defined(STM32F401CDYx) && !defined(STM32F401CEYx) && !defined(STM32F401CCFx) && \
            !defined(STM32F401RBTx) && !defined(STM32F401RCTx) && !defined(STM32F401RDTx) && \
            !defined(STM32F401RETx) && !defined(STM32F401VBHx) && !defined(STM32F401VCHx) && \
            !defined(STM32F401VBTx) && !defined(STM32F401VCTx) && !defined(STM32F401VDHx) && \
            !defined(STM32F401VEHx) && !defined(STM32F401VDTx) && !defined(STM32F401VETx) && \
            !defined(STM32F405OEYx) && !defined(STM32F405OGYx) && !defined(STM32F405RGTx) && \
            !defined(STM32F405VGTx) && !defined(STM32F405ZGTx) && !defined(STM32F415OGYx) && \
            !defined(STM32F415RGTx) && !defined(STM32F415VGTx) && !defined(STM32F415ZGTx) && \
            !defined(STM32F407IEHx) && !defined(STM32F407IGHx) && !defined(STM32F407IETx) && \
            !defined(STM32F407IGTx) && !defined(STM32F407VETx) && !defined(STM32F407VGTx) && \
            !defined(STM32F407ZETx) && !defined(STM32F407ZGTx) && !defined(STM32F417IEHx) && \
            !defined(STM32F417IGHx) && !defined(STM32F417IETx) && !defined(STM32F417IGTx) && \
            !defined(STM32F417VETx) && !defined(STM32F417VGTx) && !defined(STM32F417ZETx) && \
            !defined(STM32F417ZGTx) && !defined(STM32F410C8Tx) && !defined(STM32F410CBTx) && \
            !defined(STM32F410C8Ux) && !defined(STM32F410CBUx) && !defined(STM32F410R8Ix) && \
            !defined(STM32F410RBIx) && !defined(STM32F410R8Tx) && !defined(STM32F410RBTx) && \
            !defined(STM32F410T8Yx) && !defined(STM32F410TBYx) && !defined(STM32F411CCUx) && \
            !defined(STM32F411CEUx) && !defined(STM32F411CCYx) && !defined(STM32F411CEYx) && \
            !defined(STM32F411RCTx) && !defined(STM32F411RETx) && !defined(STM32F411VCHx) && \
            !defined(STM32F411VEHx) && !defined(STM32F411VCTx) && !defined(STM32F411VETx) && \
            !defined(STM32F412CEUx) && !defined(STM32F412CGUx) && !defined(STM32F412RETx) && \
            !defined(STM32F412RGTx) && !defined(STM32F412REYx) && !defined(STM32F412RGYx) && \
            !defined(STM32F412REYxP) && !defined(STM32F412RGYxP) && !defined(STM32F412VEHx) && \
            !defined(STM32F412VGHx) && !defined(STM32F412VETx) && !defined(STM32F412VGTx) && \
            !defined(STM32F412ZEJx) && !defined(STM32F412ZGJx) && !defined(STM32F412ZETx) && \
            !defined(STM32F412ZGTx) && !defined(STM32F413CGUx) && !defined(STM32F413CHUx) && \
            !defined(STM32F413MGYx) && !defined(STM32F413MHYx) && !defined(STM32F413RGTx) && \
            !defined(STM32F413RHTx) && !defined(STM32F413VGHx) && !defined(STM32F413VHHx) && \
            !defined(STM32F413VGTx) && !defined(STM32F413VHTx) && !defined(STM32F413ZGJx) && \
            !defined(STM32F413ZHJx) && !defined(STM32F413ZGTx) && !defined(STM32F413ZHTx) && \
            !defined(STM32F423CHUx) && !defined(STM32F423MHYx) && !defined(STM32F423RHTx) && \
            !defined(STM32F423VHHx) && !defined(STM32F423VHTx) && !defined(STM32F423ZHJx) && \
            !defined(STM32F423ZHTx) && !defined(STM32F427AGHx) && !defined(STM32F427AIHx) && \
            !defined(STM32F427IGHx) && !defined(STM32F427IIHx) && !defined(STM32F427IGTx) && \
            !defined(STM32F427IITx) && !defined(STM32F427VGTx) && !defined(STM32F427VITx) && \
            !defined(STM32F427ZGTx) && !defined(STM32F427ZITx) && !defined(STM32F437AIHx) && \
            !defined(STM32F437IGHx) && !defined(STM32F437IIHx) && !defined(STM32F437IGTx) && \
            !defined(STM32F437IITx) && !defined(STM32F437VGTx) && !defined(STM32F437VITx) && \
            !defined(STM32F437ZGTx) && !defined(STM32F437ZITx) && !defined(STM32F429AGHx) && \
            !defined(STM32F429AIHx) && !defined(STM32F429BETx) && !defined(STM32F429BGTx) && \
            !defined(STM32F429BITx) && !defined(STM32F429IETx) && !defined(STM32F429IGTx) && \
            !defined(STM32F429IEHx) && !defined(STM32F429IGHx) && !defined(STM32F429IIHx) && \
            !defined(STM32F429IITx) && !defined(STM32F429NEHx) && !defined(STM32F429NGHx) && \
            !defined(STM32F429NIHx) && !defined(STM32F429VETx) && !defined(STM32F429VGTx) && \
            !defined(STM32F429VITx) && !defined(STM32F429ZETx) && !defined(STM32F429ZGTx) && \
            !defined(STM32F429ZGYx) && !defined(STM32F429ZITx) && !defined(STM32F429ZIYx) && \
            !defined(STM32F439AIHx) && !defined(STM32F439BGTx) && !defined(STM32F439BITx) && \
            !defined(STM32F439IGHx) && !defined(STM32F439IIHx) && !defined(STM32F439IGTx) && \
            !defined(STM32F439IITx) && !defined(STM32F439NGHx) && !defined(STM32F439NIHx) && \
            !defined(STM32F439VGTx) && !defined(STM32F439VITx) && !defined(STM32F439ZGTx) && \
            !defined(STM32F439ZITx) && !defined(STM32F439ZGYx) && !defined(STM32F439ZIYx) && \
            !defined(STM32F446MCYx) && !defined(STM32F446MEYx) && !defined(STM32F446RCTx) && \
            !defined(STM32F446RETx) && !defined(STM32F446VCTx) && !defined(STM32F446VETx) && \
            !defined(STM32F446ZCHx) && !defined(STM32F446ZEHx) && !defined(STM32F446ZCJx) && \
            !defined(STM32F446ZEJx) && !defined(STM32F446ZCTx) && !defined(STM32F446ZETx) && \
            !defined(STM32F469AEHx) && !defined(STM32F469AGHx) && !defined(STM32F469AIHx) && \
            !defined(STM32F469AEYx) && !defined(STM32F469AGYx) && !defined(STM32F469AIYx) && \
            !defined(STM32F469BETx) && !defined(STM32F469BGTx) && !defined(STM32F469BITx) && \
            !defined(STM32F469IETx) && !defined(STM32F469IGTx) && !defined(STM32F469IEHx) && \
            !defined(STM32F469IGHx) && !defined(STM32F469IIHx) && !defined(STM32F469IITx) && \
            !defined(STM32F469NEHx) && !defined(STM32F469NGHx) && !defined(STM32F469NIHx) && \
            !defined(STM32F469VETx) && !defined(STM32F469VGTx) && !defined(STM32F469VITx) && \
            !defined(STM32F469ZETx) && !defined(STM32F469ZGTx) && !defined(STM32F469ZITx) && \
            !defined(STM32F479AGHx) && !defined(STM32F479AIHx) && !defined(STM32F479AGYx) && \
            !defined(STM32F479AIYx) && !defined(STM32F479BGTx) && !defined(STM32F479BITx) && \
            !defined(STM32F479IGHx) && !defined(STM32F479IIHx) && !defined(STM32F479IGTx) && \
            !defined(STM32F479IITx) && !defined(STM32F479NGHx) && !defined(STM32F479NIHx) && \
            !defined(STM32F479VGTx) && !defined(STM32F479VITx) && !defined(STM32F479ZGTx) && \
            !defined(STM32F479ZITx)
                #error Invalid MCU specified
        #endif
        #if defined(STM32F401CBUx) || defined(STM32F401CCUx) || defined(STM32F401CBYx) || \
            defined(STM32F401CCYx) || defined(STM32F401CDUx) || defined(STM32F401CEUx) || \
            defined(STM32F401CDYx) || defined(STM32F401CEYx) || defined(STM32F401CCFx) || \
            defined(STM32F401RBTx) || defined(STM32F401RCTx) || defined(STM32F401RDTx) || \
            defined(STM32F401RETx) || defined(STM32F401VBHx) || defined(STM32F401VCHx) || \
            defined(STM32F401VBTx) || defined(STM32F401VCTx) || defined(STM32F401VDHx) || \
            defined(STM32F401VEHx) || defined(STM32F401VDTx) || defined(STM32F401VETx) || \
            defined(STM32F405OEYx) || defined(STM32F405OGYx) || defined(STM32F405RGTx) || \
            defined(STM32F405VGTx) || defined(STM32F405ZGTx) || defined(STM32F415OGYx) || \
            defined(STM32F415RGTx) || defined(STM32F415VGTx) || defined(STM32F415ZGTx) || \
            defined(STM32F407IEHx) || defined(STM32F407IGHx) || defined(STM32F407IETx) || \
            defined(STM32F407IGTx) || defined(STM32F407VETx) || defined(STM32F407VGTx) || \
            defined(STM32F407ZETx) || defined(STM32F407ZGTx) || defined(STM32F417IEHx) || \
            defined(STM32F417IGHx) || defined(STM32F417IETx) || defined(STM32F417IGTx) || \
            defined(STM32F417VETx) || defined(STM32F417VGTx) || defined(STM32F417ZETx) || \
            defined(STM32F417ZGTx) || defined(STM32F410C8Tx) || defined(STM32F410CBTx) || \
            defined(STM32F410C8Ux) || defined(STM32F410CBUx) || defined(STM32F410R8Ix) || \
            defined(STM32F410RBIx) || defined(STM32F410R8Tx) || defined(STM32F410RBTx) || \
            defined(STM32F410T8Yx) || defined(STM32F410TBYx) || defined(STM32F411CCUx) || \
            defined(STM32F411CEUx) || defined(STM32F411CCYx) || defined(STM32F411CEYx) || \
            defined(STM32F411RCTx) || defined(STM32F411RETx) || defined(STM32F411VCHx) || \
            defined(STM32F411VEHx) || defined(STM32F411VCTx) || defined(STM32F411VETx) || \
            defined(STM32F412CEUx) || defined(STM32F412CGUx) || defined(STM32F412RETx) || \
            defined(STM32F412RGTx) || defined(STM32F412REYx) || defined(STM32F412RGYx) || \
            defined(STM32F412REYxP) || defined(STM32F412RGYxP) || defined(STM32F412VEHx) || \
            defined(STM32F412VGHx) || defined(STM32F412VETx) || defined(STM32F412VGTx) || \
            defined(STM32F412ZEJx) || defined(STM32F412ZGJx) || defined(STM32F412ZETx) || \
            defined(STM32F412ZGTx) || defined(STM32F413CGUx) || defined(STM32F413CHUx) || \
            defined(STM32F413MGYx) || defined(STM32F413MHYx) || defined(STM32F413RGTx) || \
            defined(STM32F413RHTx) || defined(STM32F413VGHx) || defined(STM32F413VHHx) || \
            defined(STM32F413VGTx) || defined(STM32F413VHTx) || defined(STM32F413ZGJx) || \
            defined(STM32F413ZHJx) || defined(STM32F413ZGTx) || defined(STM32F413ZHTx) || \
            defined(STM32F423CHUx) || defined(STM32F423MHYx) || defined(STM32F423RHTx) || \
            defined(STM32F423VHHx) || defined(STM32F423VHTx) || defined(STM32F423ZHJx) || \
            defined(STM32F423ZHTx) || defined(STM32F427AGHx) || defined(STM32F427AIHx) || \
            defined(STM32F427IGHx) || defined(STM32F427IIHx) || defined(STM32F427IGTx) || \
            defined(STM32F427IITx) || defined(STM32F427VGTx) || defined(STM32F427VITx) || \
            defined(STM32F427ZGTx) || defined(STM32F427ZITx) || defined(STM32F437AIHx) || \
            defined(STM32F437IGHx) || defined(STM32F437IIHx) || defined(STM32F437IGTx) || \
            defined(STM32F437IITx) || defined(STM32F437VGTx) || defined(STM32F437VITx) || \
            defined(STM32F437ZGTx) || defined(STM32F437ZITx) || defined(STM32F429AGHx) || \
            defined(STM32F429AIHx) || defined(STM32F429BETx) || defined(STM32F429BGTx) || \
            defined(STM32F429BITx) || defined(STM32F429IETx) || defined(STM32F429IGTx) || \
            defined(STM32F429IEHx) || defined(STM32F429IGHx) || defined(STM32F429IIHx) || \
            defined(STM32F429IITx) || defined(STM32F429NEHx) || defined(STM32F429NGHx) || \
            defined(STM32F429NIHx) || defined(STM32F429VETx) || defined(STM32F429VGTx) || \
            defined(STM32F429VITx) || defined(STM32F429ZETx) || defined(STM32F429ZGTx) || \
            defined(STM32F429ZGYx) || defined(STM32F429ZITx) || defined(STM32F429ZIYx) || \
            defined(STM32F439AIHx) || defined(STM32F439BGTx) || defined(STM32F439BITx) || \
            defined(STM32F439IGHx) || defined(STM32F439IIHx) || defined(STM32F439IGTx) || \
            defined(STM32F439IITx) || defined(STM32F439NGHx) || defined(STM32F439NIHx) || \
            defined(STM32F439VGTx) || defined(STM32F439VITx) || defined(STM32F439ZGTx) || \
            defined(STM32F439ZITx) || defined(STM32F439ZGYx) || defined(STM32F439ZIYx) || \
            defined(STM32F446MCYx) || defined(STM32F446MEYx) || defined(STM32F446RCTx) || \
            defined(STM32F446RETx) || defined(STM32F446VCTx) || defined(STM32F446VETx) || \
            defined(STM32F446ZCHx) || defined(STM32F446ZEHx) || defined(STM32F446ZCJx) || \
            defined(STM32F446ZEJx) || defined(STM32F446ZCTx) || defined(STM32F446ZETx) || \
            defined(STM32F469AEHx) || defined(STM32F469AGHx) || defined(STM32F469AIHx) || \
            defined(STM32F469AEYx) || defined(STM32F469AGYx) || defined(STM32F469AIYx) || \
            defined(STM32F469BETx) || defined(STM32F469BGTx) || defined(STM32F469BITx) || \
            defined(STM32F469IETx) || defined(STM32F469IGTx) || defined(STM32F469IEHx) || \
            defined(STM32F469IGHx) || defined(STM32F469IIHx) || defined(STM32F469IITx) || \
            defined(STM32F469NEHx) || defined(STM32F469NGHx) || defined(STM32F469NIHx) || \
            defined(STM32F469VETx) || defined(STM32F469VGTx) || defined(STM32F469VITx) || \
            defined(STM32F469ZETx) || defined(STM32F469ZGTx) || defined(STM32F469ZITx) || \
            defined(STM32F479AGHx) || defined(STM32F479AIHx) || defined(STM32F479AGYx) || \
            defined(STM32F479AIYx) || defined(STM32F479BGTx) || defined(STM32F479BITx) || \
            defined(STM32F479IGHx) || defined(STM32F479IIHx) || defined(STM32F479IGTx) || \
            defined(STM32F479IITx) || defined(STM32F479NGHx) || defined(STM32F479NIHx) || \
            defined(STM32F479VGTx) || defined(STM32F479VITx) || defined(STM32F479ZGTx) || \
            defined(STM32F479ZITx)
        #endif
        impossible( "Incorrect DE pin" );
    }

} // namespace detail

