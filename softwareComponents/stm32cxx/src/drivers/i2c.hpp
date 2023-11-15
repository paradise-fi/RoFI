#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <ranges>

#include <system/assert.hpp>

#include <drivers/peripheral.hpp>
#include <drivers/gpio.hpp>

#include <stm32g0xx_ll_i2c.h>

#include <i2c.port.hpp>

namespace {
    #include <concepts>

    template < typename container >
    concept RandomAccess = requires( container cont, uint32_t index ) {
        // Note: it may cut higher bits if the return type is bigger than uint8_t
        { cont[index] } -> std::common_with< uint8_t >;
    };
}

struct I2CPin : public Gpio::Pin
{
    I2CPin( Gpio::Pin&& pin ) : Gpio::Pin( pin ) {}

    virtual int alternativeFun( I2C_TypeDef *periph ) = 0;

    I2CPin& setupODAlternate( I2C_TypeDef *i2cPeriph ) {
        setupODOutput( true, true );

        assert( 0 <= _pos && _pos <= 15 );

        auto pin = 1 << _pos;
        LL_GPIO_SetPinMode( _periph, pin, LL_GPIO_MODE_ALTERNATE );

        auto alternateFunction = alternativeFun( i2cPeriph );
        if ( pin <= 7 ) {
            LL_GPIO_SetAFPin_0_7( _periph, pin, alternateFunction );
        } else {
            LL_GPIO_SetAFPin_8_15( _periph, pin, alternateFunction );
        }

        return *this;
    }
};

struct SdaPin : public I2CPin, public detail::SdaPin< SdaPin >
{
    using I2CPin::I2CPin;

    int alternativeFun( I2C_TypeDef *periph ) final {
        return detail::SdaPin< SdaPin >::alternativeFun( periph );
    }
};

struct SclPin : public I2CPin, public detail::SclPin< SclPin >
{
    using I2CPin::I2CPin;

    int alternativeFun( I2C_TypeDef *periph ) final {
        return detail::SclPin< SclPin >::alternativeFun( periph );
    }
};

struct I2C: public Peripheral< I2C_TypeDef >, public detail::I2C< I2C > {
    /// \brief Since this error is used in C library it is defined as enum not enum class
    enum Error : int8_t {
        /// This goes in hand with `VL53L1X_ERROR`.
        Valid = 0,
        // VL53L1X lib error = 1
        NotAcknowledge = 2,
        BusError = 3,
        ArbitrationLoss,
        OverrunUnderrun,
        BufferOverflow,
    };

    using error_type = Error;

    /**
     * \brief Speed mode settings for the i2c peripheral, the values are calculated by STM32CUBEMX tool
    */
    enum class SpeedMode : uint32_t {
        Standard = 0x00303D5B,
        Fast     = 0x0010061A,
    };

    static std::string_view errorMessage( error_type error )
    {
        using namespace std::literals::string_view_literals;
        switch (error) {
        case NotAcknowledge:  return "Not acknowledged"sv;
        case BusError:        return "Bus error"sv;
        case ArbitrationLoss: return "Arbitration loss"sv;
        case OverrunUnderrun: return "Overrun/Underrun"sv;
        case BufferOverflow:  return "Buffer overflow"sv;
        default:              return "Unkown error"sv;
        }
    }

    I2C( I2C_TypeDef *i2c, SdaPin sdaPin, SclPin sclPin, SpeedMode timing )
        : Peripheral< I2C_TypeDef >( i2c )
    {
        sdaPin.setupODAlternate( _periph );
        sclPin.setupODAlternate( _periph );

        enableClock();

        LL_I2C_InitTypeDef I2C_InitStruct = { };

        I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
        I2C_InitStruct.Timing = static_cast< uint32_t >( timing );
        I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
        I2C_InitStruct.DigitalFilter = 0;
        I2C_InitStruct.OwnAddress1 = 0;
        I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
        I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;

        LL_I2C_Init( _periph, &I2C_InitStruct );
        LL_I2C_EnableAutoEndMode( _periph );
        LL_I2C_SetOwnAddress2( _periph, 0, LL_I2C_OWNADDRESS2_NOMASK );
        LL_I2C_DisableOwnAddress2( _periph );
        LL_I2C_DisableGeneralCall( _periph );
        LL_I2C_EnableClockStretching( _periph );

        LL_I2C_EnableIT_ERR( _periph );
    }

    template < std::ranges::contiguous_range container >
    error_type write( const uint32_t peripheralAddress, const container& buffer ) {
        return _write( peripheralAddress, buffer.data(), buffer.size() );
    }

    /**
     * \brief Unsafe variant mostly for `vl53l1` i2c.
    */
    template < ::RandomAccess data_type >
    error_type unsafe_write( const uint32_t periheralAddress, const data_type & data, const uint32_t transferSize ) {
        return _write( periheralAddress, data, transferSize );
    }

    template < std::ranges::contiguous_range container >
    error_type read( const uint32_t peripheralAddress, container& buffer ) {
        return _read( peripheralAddress, buffer.data(), buffer.size() );
    }

    /**
     * \brief Unsafe variant mostly for `vl53l1` i2c.
    */
    error_type unsafe_read( const uint32_t peripheralAddress, uint8_t* data, const uint32_t transferSize ) {
        return _read( peripheralAddress, data, transferSize );
    }

private:
    /**
     * Checks whether an error occurred on the i2c peripheral.
     * 
     * NOTE: This function clears given error flag after checking it.
    */
    error_type checkError() {
        if ( LL_I2C_IsActiveFlag_NACK( _periph ) ) {
            LL_I2C_ClearFlag_NACK( _periph );
            return NotAcknowledge;
        }

        if ( LL_I2C_IsActiveFlag_BERR( _periph ) ) {
            LL_I2C_ClearFlag_BERR( _periph );
            return BusError;
        }

         if ( LL_I2C_IsActiveFlag_ARLO( _periph ) ) {
            LL_I2C_ClearFlag_ARLO( _periph );
            return ArbitrationLoss;
        }

        if ( LL_I2C_IsActiveFlag_OVR( _periph ) ) {
            LL_I2C_ClearFlag_OVR( _periph );
            return OverrunUnderrun;
        }

        return Valid;
    }

    template < ::RandomAccess data_type >
    error_type _write( const uint32_t peripheralAddress, const data_type data, const uint32_t transferSize ) {
        // const data_type start = data; 
        LL_I2C_HandleTransfer( _periph, peripheralAddress, LL_I2C_ADDRSLAVE_7BIT, transferSize, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE );

        uint32_t index = 0;
        while( !LL_I2C_IsActiveFlag_STOP( _periph ) ) {
            if ( LL_I2C_IsActiveFlag_TXIS( _periph ) ) {
                if ( index >= transferSize ) {
                    // TODO: should never happen??
                    return BufferOverflow;
                }
                LL_I2C_TransmitData8( _periph, data[index] );
                ++index;
            }
        }

        error_type error = checkError();

        LL_I2C_ClearFlag_STOP( _periph );
        
        return error;
    }

    error_type _read( const uint32_t peripheralAddress, uint8_t* buffer, const uint32_t transferSize ) {
        LL_I2C_HandleTransfer( _periph, peripheralAddress, LL_I2C_ADDRSLAVE_7BIT, transferSize, LL_I2C_MODE_AUTOEND, I2C_GENERATE_START_READ );

        uint8_t i = 0;
        while( !LL_I2C_IsActiveFlag_STOP( _periph ) ) {
            if ( LL_I2C_IsActiveFlag_RXNE( _periph ) ) {
                if( i >= transferSize ) {
                    return BufferOverflow;
                }
                buffer[i] = LL_I2C_ReceiveData8( _periph );
                ++i;
            }
        }

        error_type error = checkError();

        LL_I2C_ClearFlag_STOP( _periph );

        return error;
    }
};
