#pragma once

#include <cassert>

#include <stm32f0xx_ll_spi.h>
#include <stm32f0xx_ll_gpio.h>

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
        assert( false && "Not implemented" );
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
        assert( false && "Not implemented" );
        __builtin_trap();
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
        assert( false && "Not implemented" );
        __builtin_trap();
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
        assert( false && "Not implemented" );
        __builtin_trap();
    }
};

} // namespace detail

inline int LL_DMAMUX_REQ_RX( SPI_TypeDef *periph ) {
    assert( false && "Not implemented" );
    __builtin_trap();
}

inline int LL_DMAMUX_REQ_TX( SPI_TypeDef *periph ) {
    assert( false && "Not implemented" );
    __builtin_trap();
}