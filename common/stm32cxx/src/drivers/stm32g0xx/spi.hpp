#pragma once

#include <cassert>

#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_spi.h>
#include <stm32g0xx_ll_gpio.h>
#include <stm32g0xx_ll_dma.h>

namespace detail {

template < typename Self >
class Spi {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    void enableClock() {
        if ( self()._periph == SPI1 )
            LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_SPI1 );
        else if ( self()._periph == SPI2 )
            LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_SPI2 );
        else
            assert( false && "Invalid SPI specified" );
    }
};

template < typename Self >
class CsOn {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    int alternativeFun( SPI_TypeDef *periph ) {
        if ( periph == SPI1 ) {
            if ( self()._pin._periph == GPIOA && self()._pin._pos == 4 )
                return LL_GPIO_AF_0;
        }
        // ToDo: Implement more configurations
        assert( false && "Unsupported configuration" );
    }
};

template < typename Self >
class SckOn {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    int alternativeFun( SPI_TypeDef *periph ) {
        if ( periph == SPI1 ) {
            if ( self()._pin._periph == GPIOA && self()._pin._pos == 1 )
                return LL_GPIO_AF_0;
        }
        // ToDo: Implement more configurations
        assert( false && "Unsupported configuration" );
    }
};

template < typename Self >
class MisoOn {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    int alternativeFun( SPI_TypeDef *periph ) {
        if ( periph == SPI1 ) {
            if ( self()._pin._periph == GPIOA && self()._pin._pos == 6 )
                return LL_GPIO_AF_0;
        }
        // ToDo: Implement more configurations
        assert( false && "Unsupported configuration" );
    }
};

} // namespace detail

inline int LL_DMAMUX_REQ_RX( SPI_TypeDef *periph ) {
    if ( periph == SPI1 )
        return LL_DMAMUX_REQ_SPI1_RX;
    else if ( periph == SPI2 )
        return LL_DMAMUX_REQ_SPI2_RX;
    assert( false && "Invalid SPI peripheral" );
}

inline int LL_DMAMUX_REQ_TX( SPI_TypeDef *periph ) {
    if ( periph == SPI1 )
        return LL_DMAMUX_REQ_SPI1_TX;
    else if ( periph == SPI2 )
        return LL_DMAMUX_REQ_SPI2_TX;
    assert( false && "Invalid SPI peripheral" );
}