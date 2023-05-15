#pragma once

#include <cassert>
#include <cstdint>
#include <string_view>
#include <optional>

#include <atoms/result.hpp>

#include <system/idle.hpp>

#include <drivers/timer.hpp>
#include <drivers/i2c.hpp>

#include <VL53L1X_api.h>
#include <vl53l1_platform.h>

// namespace that's used to propagate needed peripherals/functions
// to library's `vl53l1_plaftorm.cpp`
namespace lidar_inner {
    void initialize_platform( I2C* );
}

struct Lidar {
    using Void = atoms::Void;

    template< typename T, typename E = std::string >
    using Result = atoms::Result< T, E >;

    using error_type = std::string_view;
    using result_type = Result< Void,  error_type >;

    using address_type = uint16_t;
    using data_type = VL53L1X_Result_t;

    const int8_t VL53L1X_ERROR_NONE = 0;

    enum class DistanceMode : uint16_t {
        Autonomous = 0,
        Short      = 1,
        Long       = 2,
    };

    Lidar(
            I2C* i2c,
            const Gpio::Pin lidarEnable,
            std::optional< const Gpio::Pin > interruptPin = std::nullopt,
            const address_type deviceAddress = 0x52,
            const uint32_t communicationSpeed = 400
        )
        : _i2c( i2c )
        , _lidarEnable( lidarEnable )
        , _interruptPin( interruptPin )
        , _deviceAddress( deviceAddress )
    {
        // As mentioned in datasheet VL53L1X has maximum speed of 400 kbits/s
        assert( communicationSpeed <= 400 );
    }

    ~Lidar() { }

    result_type initialize()
    {
        _lidarEnable.setupPPOutput( );
        _lidarEnable.write( true );
        lidar_inner::initialize_platform( _i2c );

        VL53L1X_ERROR status;

        if ( _deviceAddress != _defaultAddress ) {
            status = VL53L1X_SetI2CAddress( _defaultAddress, _deviceAddress );
            if ( status != VL53L1X_ERROR_NONE ) {
                return result_type::error( errorMessage( status ) );
            }
        }

        uint8_t booted;
        do {
            status = VL53L1X_BootState( _deviceAddress, &booted );
        } while ( !booted );

        status = VL53L1X_SensorInit( _deviceAddress );
        if (status != VL53L1X_ERROR_NONE) {
            return result_type::error( errorMessage( status ) );
        }

        return atoms::make_result_value< Void >();
    }

    result_type startMeasurement()
    {
        VL53L1X_ERROR status = VL53L1X_StartRanging( _deviceAddress );
        if (status != VL53L1X_ERROR_NONE) {
            return result_type::error( errorMessage( status ) );
        }

        return atoms::make_result_value< Void >();
    }

    result_type stopMeasurement()
    {
        VL53L1X_ERROR status = VL53L1X_ClearInterrupt( _deviceAddress );
        if ( status != VL53L1X_ERROR_NONE ) {
            return result_type::error( errorMessage( status ) );
        }

        status = VL53L1X_StopRanging( _deviceAddress );
        if ( status != VL53L1X_ERROR_NONE ) {
            return result_type::error( errorMessage( status ) );
        }

        return atoms::make_result_value< Void >();
    }

    Result< bool, error_type > getMeasurementDataReady()
    {
        uint8_t ready;
        VL53L1X_ERROR status = VL53L1X_CheckForDataReady( _deviceAddress, &ready );
        if ( status != VL53L1X_ERROR_NONE ) {
            return Result< bool, error_type >::error( errorMessage( status ) );
        }

        return atoms::make_result_value< bool >( bool(ready) );
    }

    result_type clearInterruptAndStartMeasurement()
    {
        VL53L1X_ERROR status = VL53L1X_ClearInterrupt( _deviceAddress );
        if ( status != VL53L1X_ERROR_NONE ) {
            return result_type::error( errorMessage( status ) );
        }

        return atoms::make_result_value< Void >();
    }

    Result< data_type, error_type > getRangingMeasurementData()
    {
        data_type rangingMeasurementData;

        VL53L1X_ERROR status = VL53L1X_GetResult( _deviceAddress, &rangingMeasurementData );
        if ( status != VL53L1X_ERROR_NONE ) {
            return atoms::result_error( errorMessage( status ) );
        }

        if ( _isAutonomousMode ) {
            _handleAutonomousMode( rangingMeasurementData );
        }

        return atoms::result_value< data_type >( rangingMeasurementData );
    }

    Result< DistanceMode, error_type > getDistanceMode()
    {
        if ( _isAutonomousMode ) {
            return atoms::result_value( DistanceMode::Autonomous );
        }

        return _getDistanceMode();
    }

    /** \brief This function is used for the status command; thus, cannot wait
     * for the response of the lidar.
    */
    DistanceMode getDistanceModeInstant()
    {
        if ( _isAutonomousMode ) {
            return DistanceMode::Autonomous;
        }

        return static_cast< DistanceMode >( uint16_t(_currentDistanceMode) );
    }

    result_type setDistanceMode( DistanceMode mode )
    {
        _isAutonomousMode = mode == DistanceMode::Autonomous; 

        VL53L1X_DistanceMode setMode = static_cast< decltype( setMode ) >( _isAutonomousMode ? _defaultMode : mode );

        return _setDistanceMode( setMode );
    }

    result_type setTimingBudget( uint16_t timingBudget )
    {
        VL53L1X_ERROR status = VL53L1X_SetTimingBudgetInMs( _deviceAddress, timingBudget );
        if ( status != VL53L1X_ERROR_NONE ) {
            return atoms::result_error( errorMessage( status ) );
        }

        return atoms::make_result_value< Void >();
    }

    Result< uint16_t, error_type > getTimingBudget()
    {
        uint16_t timingBudget = 0;
        VL53L1X_ERROR status = VL53L1X_GetTimingBudgetInMs( _deviceAddress, &timingBudget );
        if ( status != VL53L1X_ERROR_NONE ) {
            return atoms::result_error( errorMessage( status ) );
        }

        return atoms::result_value( timingBudget );
    }

    result_type setInterMeasurement( uint32_t interMeasurement )
    {
        auto timingBudget = getTimingBudget();
        if ( !timingBudget.has_value() ){
            return atoms::result_error( timingBudget.assume_error() );
        }
        if ( interMeasurement < timingBudget.assume_value() ) {
            // TODO: Handle this 
        }

        VL53L1X_ERROR status = VL53L1X_SetInterMeasurementInMs( _deviceAddress, interMeasurement );
        if ( status != VL53L1X_ERROR_NONE ) {
            return atoms::result_error( errorMessage( status ) );
        }

        return atoms::make_result_value< Void >();
    }

    Result< uint16_t, error_type > getInterMeasurement()
    {
        uint16_t interMeasurement = 0;
        VL53L1X_ERROR status = VL53L1X_GetInterMeasurementInMs( _deviceAddress, &interMeasurement );
        if ( status != VL53L1X_ERROR_NONE ) {
            return atoms::result_error( errorMessage( status ) );
        }

        return atoms::result_value( interMeasurement );
    }

    Result< uint16_t, int8_t > getSensorId()
    {
        _lidarEnable.setupODOutput( true, true );
        _lidarEnable.write( true );
        lidar_inner::initialize_platform( _i2c );

        uint16_t id = 0;
        VL53L1X_ERROR status = VL53L1X_GetSensorId( _deviceAddress, &id );
        if ( status != VL53L1X_ERROR_NONE ) {
            return atoms::result_error( status );
        }

        return atoms::result_value< uint16_t >( id );
    }

    template < typename Callback >
    void setupInterrupt( Callback callback )
    {
        assert( _interruptPin );
        
        _interruptPin->setupInput( true, true );
        _interruptPin->setupInterrupt( LL_EXTI_TRIGGER_FALLING, [&]( auto ){ IdleTask::defer( callback ); } );
    }

    result_type setInterruptPolarity( bool polarity ) {
        VL53L1X_ERROR status = VL53L1X_SetInterruptPolarity( _deviceAddress, polarity );
        if ( status != VL53L1X_ERROR_NONE ) {
            return atoms::result_error( errorMessage( status ) );
        }

        return atoms::make_result_value< Void >();
    }

private:
    /// Values are specified by the VL53L1X library
    enum class VL53L1X_DistanceMode : uint16_t {
        Short = 1,
        Long  = 2,
    };

    result_type _setDistanceMode( VL53L1X_DistanceMode mode )
    {
        VL53L1X_ERROR status = VL53L1X_SetDistanceMode( _deviceAddress, uint16_t(mode) );
        if ( status != VL53L1X_ERROR_NONE ) {
            return atoms::result_error( errorMessage( status ) );
        }

        _currentDistanceMode = mode;

        return atoms::result_error( errorMessage( status ) );
    }

    Result< DistanceMode, error_type > _getDistanceMode()
    {
        uint16_t distanceMode;
        VL53L1X_ERROR status = VL53L1X_GetDistanceMode( _deviceAddress,  &distanceMode );
        if ( status != VL53L1X_ERROR_NONE ) {
            return atoms::result_error( errorMessage( status ) );
        }

        return atoms::result_value( static_cast< DistanceMode >( distanceMode ) );
    }

    void _handleAutonomousMode( const data_type& data )
    {
        /// From manual um2510
        enum DataStatus {
            RangeValid  = 0,
            SigmaFail   = 1,
            SignalFail  = 2,
            OutOfBounds = 4,
            WrapAround  = 7,            
        };

        auto modeRes = _getDistanceMode();
        // In case communication failed the set mode is irrelevant
        if ( !modeRes.has_value() )
            return;
        auto mode = modeRes.assume_value();

        auto threshold = 200;

        switch (mode) {
        case DistanceMode::Short: {
            if (
                data.Status == SigmaFail
                || data.Status == OutOfBounds
                || data.Status == WrapAround
            ) {
                static_cast< void >( _setDistanceMode( VL53L1X_DistanceMode::Long ) );    
            }
            break;
            }
        case DistanceMode::Long: {
            if (
                data.Status == RangeValid
                && mode == DistanceMode::Long
                && data.Distance <= 1300 - threshold)
            {
                // Assume the distance is short enough to use short mode for better
                // immunity against ambient light.
                static_cast< void >( _setDistanceMode( VL53L1X_DistanceMode::Short ) );
            }
            break;
            }
        default:
            // TODO: from C++23 std::unreachable();
            break;
        }
    }

    error_type errorMessage( VL53L1X_ERROR lidarError )
    {
        // This is possible due to the lidar driver (specifically Ultra Lite Driver)
        // not having defined its own errors, only defined value is `0` which is *no error*.
        // IMPLEMENTATION DETAIL: 
        //   The ULD uses only value `1` as its own error.
        const auto error = static_cast< I2C::Error >( lidarError );
        return I2C::errorMessage( error );
    };

    I2C* _i2c;
    Gpio::Pin _lidarEnable;
    std::optional< Gpio::Pin > _interruptPin;

    address_type _deviceAddress;

    bool _isAutonomousMode = true;
    VL53L1X_DistanceMode _currentDistanceMode = VL53L1X_DistanceMode::Long;
    
    const address_type _defaultAddress = 0x52;
    const DistanceMode _defaultMode = DistanceMode::Short;
};

