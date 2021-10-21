#pragma once

#include <stm32f4xx_ll_rcc.h>
#include <stm32f4xx_ll_bus.h>
#include <stm32f4xx_ll_gpio.h>

namespace detail {
template < typename Self >
class UsbDevice {
protected:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    auto& _getDriver() {
        return usbd_hw;
    }

    void _setupGpio() {
        LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_GPIOA );
        LL_GPIO_InitTypeDef GPIO_InitStruct{};
        GPIO_InitStruct.Pin = LL_GPIO_PIN_11 | LL_GPIO_PIN_12;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
        GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
        GPIO_InitStruct.Alternate = LL_GPIO_AF_10;
        LL_GPIO_Init( GPIOA, &GPIO_InitStruct );
    }

    void _enableInterrupt() {
        NVIC_EnableIRQ( OTG_FS_IRQn );
    }

    void _disableInterrupt() {
        NVIC_DisableIRQ( OTG_FS_IRQn );
    }

public:
    void _onInterrupt() {
        usbd_poll( &self()._device );
    }

    friend void OTG_FS_IRQHandler();
};

} // namespace detail
