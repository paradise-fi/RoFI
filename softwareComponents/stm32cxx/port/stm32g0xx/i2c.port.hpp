#pragma once

namespace detail {

template < typename Self >
class SdaPin {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    int alternativeFun( I2C_TypeDef *periph ) {
        if ( periph == I2C2 ) {
            if ( self()._periph == GPIOA && self()._pos == 12 )
                return LL_GPIO_AF_6;
        }
        // ToDo: Implement more configurations
        assert( false && "Unsupported I2C SdaPin configuration" );
        __builtin_trap();
    }
};

template < typename Self >
class SclPin {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    int alternativeFun( I2C_TypeDef *periph ) {
        if ( periph == I2C2 ) {
            if ( self()._periph == GPIOA && self()._pos == 11 )
                return LL_GPIO_AF_6;
        }
        // ToDo: Implement more configurations
        assert( false && "Unsupported I2C SdaPin configuration" );
        __builtin_trap();
    }
};

template < typename Self >
class I2C {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    void enableClock() {
        if ( self()._periph == I2C1 )
            LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_I2C1 );
        else if ( self()._periph == I2C2 )
            LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_I2C2 );
        else
            assert( false && "Invalid I2C specified" );
    }
};

} // namespace detail