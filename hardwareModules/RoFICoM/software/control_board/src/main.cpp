#include <cassert>
#include <functional>
#include <type_traits>

#include <system/idle.hpp>
#include <system/dbg.hpp>

#include <drivers/gpio.hpp>
#include <drivers/timer.hpp>
#include <drivers/spi.hpp>
#include <drivers/uart.hpp>
#include <drivers/adc.hpp>

#include <motor.hpp>
#include "lidar.hpp"
#include <util.hpp>

#include <spiInterface.hpp>
#include <connInterface.hpp>

#include <stm32g0xx_hal.h>
#include <stm32g0xx_ll_rcc.h>
#include <stm32g0xx_ll_system.h>
#include <stm32g0xx_ll_cortex.h>
#include <stm32g0xx_ll_utils.h>

#include <bsp.hpp>
#include <configuration.hpp>


using Block = memory::Pool::Block;

using LidarResult = decltype( std::declval< Lidar >().getRangingMeasurementData() );

enum ConnectorStateFlags {
    PositionExpanded  = 1 << 0,
    InternalConnected = 1 << 1,
    ExternalConnected = 1 << 2,
    /**
     * Usually is affected by ambient light.
     * Ob00 = autonomous distance measurement
     * 0b01 = short distance measurement <= 1.3m 
     * 0b10 = long distance measurement <= 4m
    */
    LidarDistanceMode = 0b11 << 3,
    MatingSide        = 1 << 8,
    Orientation       = 0b11 << 9,
    /**
     * 0b00 = error
     * 0b01 = Data not yet measured
     * 0b10 = Data outside measured range (data DOESN'T HAVE TO BE valid, but can be, usually means is below or above of range we can measure )
     * 0b11 = valid measurement data
    */
    LidarStatus       = 0b11 << 11,
};


void onCmdVersion( SpiInterface& interf ) {
    auto block = memory::Pool::allocate( 4 );
    viewAs< uint16_t >( block.get() ) = 1;
    viewAs< uint16_t >( block.get() + 2 ) = 1;
    interf.sendBlock( std::move( block ), 4 );
}

void onCmdStatus( SpiInterface& interf, Block header,
    ConnComInterface& connInt, ConnectorStatus connectorStatus,
    Slider& slider, PowerSwitch& powerInterface,
    Lidar& lidar, std::optional< LidarResult >& lidarResult )
{
    uint16_t status = viewAs< uint16_t >( header.get() );
    uint16_t mask = viewAs< uint16_t >( header.get() + 2 );
    if ( mask & ConnectorStateFlags::PositionExpanded ) {
        if ( status & ConnectorStateFlags::PositionExpanded )
            slider.expand();
        else
            slider.retract();
    }
    if ( mask & ConnectorStateFlags::InternalConnected ) {
        powerInterface.connectInternal( static_cast< bool >( status & ConnectorStateFlags::InternalConnected ) );
    }
    if ( mask & ConnectorStateFlags::ExternalConnected ) {
        powerInterface.connectExternal( static_cast< bool >( status & ConnectorStateFlags::ExternalConnected ) );
    }

    if ( mask & ConnectorStateFlags::LidarDistanceMode ) {
        auto mode = ( status & ConnectorStateFlags::LidarDistanceMode ) >> 11;
        if ( mode != 0b11 ) {
            // We cannot report error back
            static_cast< void >( lidar.setDistanceMode( Lidar::DistanceMode( mode ) ) );
        }
    }

    uint8_t lidarStatus;
    if ( ! lidarResult.has_value() ) {
        lidarStatus = 0b01;
    } else if ( ! ( lidarResult.value().has_value() ) ) {
        lidarStatus = 0b00;
    } else if ( auto& lidarData = lidarResult.value().assume_value();
                lidarData.Status == 0 ) {
        lidarStatus = 0b11;
    } else {
        lidarStatus = 0b10;
    }

    const auto blockSize = 14;
    auto block = memory::Pool::allocate( blockSize );
    uint16_t flags = 0;
    flags |= ( slider.getGoal() == Slider::State::Expanded ) << 0;
    flags |= powerInterface.getInternalConnection() << 1;
    flags |= powerInterface.getExternalConnection() << 2;
    flags |= uint8_t( lidar.getDistanceModeInstant() ) << 3;
    flags |= lidarStatus << 11;

    if ( connectorStatus.getOrientation() != ConnectorOrientation::Unknown ) {
        flags |= ConnectorStateFlags::MatingSide;
        flags |= connectorStatus.getOrientation() << 9;
    }

    viewAs< uint16_t >( block.get() ) = flags;
    viewAs< uint8_t >( block.get() + 2 ) = connInt.pending();
    viewAs< uint8_t >( block.get() + 3 ) = connInt.available();
    viewAs< uint16_t >( block.get() + 4 ) = powerInterface.getIntVoltage();
    viewAs< uint16_t >( block.get() + 6 ) = powerInterface.getIntCurrent();
    viewAs< uint16_t >( block.get() + 8 ) = powerInterface.getExtVoltage();
    viewAs< uint16_t >( block.get() + 10 ) = powerInterface.getExtCurrent();
    if ( !( lidarStatus & 0b10 ) )
        viewAs< uint16_t >( block.get() + 12 ) = lidarResult.value().assume_value().Distance;
    interf.sendBlock( std::move( block ), blockSize );
}

void onCmdInterrupt( SpiInterface& interf, Block /*header*/ ) {
    auto block = memory::Pool::allocate( 2 );
    viewAs< uint16_t >( block.get() ) = 0;
    interf.sendBlock( std::move( block ), 2 );
}

void onCmdSendBlob( SpiInterface& spiInt, ConnComInterface& connInt ) {
    spiInt.receiveBlob([&spiInt, &connInt]( Block blob, int size ) {
        int blobLen = viewAs< uint16_t >( blob.get() + 2 );
        if ( size != 4 + blobLen )
            return;
        connInt.sendBlob( std::move( blob ) );
    } );
}

void onCmdReceiveBlob( SpiInterface& spiInt, ConnComInterface& connInt ) {
    if ( connInt.available() > 0 ) {
        Block blob = connInt.getBlob();
        spiInt.sendBlob( std::move( blob ) );
    }
    else {
        auto block = memory::Pool::allocate( 4 );
        viewAs< uint16_t >( block.get() ) = 0; // content type
        viewAs< uint16_t >( block.get() + 2 ) = 0; // length
        spiInt.sendBlock( std::move( block ), 4 );
    }
}


void lidarGet( Lidar& lidar, std::optional< LidarResult >& );

void lidarInit( Lidar& lidar, std::optional< LidarResult >& currentLidarMeasurement ) {
    auto result = lidar.initialize().and_then( [&] ( auto ) {
            Dbg::blockingInfo("start measuring\n");
            return lidar.startMeasurement();
    });

    if ( !result ) {
        *currentLidarMeasurement = atoms::result_error( result.assume_error() );
        Dbg::error( "\n Error init:" );
        Dbg::error( result.assume_error().data() );
        IdleTask::defer( [&]( ) { lidarInit( lidar, currentLidarMeasurement ); } );
    } else {
        IdleTask::defer( [&]( ) { lidarGet( lidar, currentLidarMeasurement ); } );
    }
}

void lidarGet( Lidar& lidar, std::optional< LidarResult >& currentLidarMeasurement ) {
    using Data = LidarResult::value_type;

    auto result = lidar.getMeasurementDataReady().and_then( [&] ( bool ready ) -> atoms::Result< std::optional< Data >, std::string_view >  {
        if (!ready) {
            return atoms::Result< std::optional< Data >, std::string_view >::value( std::nullopt );
        }

        return lidar.getRangingMeasurementData()
            .and_then( [&] ( auto rangingMeasurementData ) {
                return lidar.clearInterruptAndStartMeasurement().and_then( [&] ( auto ) {
                    return atoms::Result< std::optional< Data >, std::string_view >::value( std::make_optional( rangingMeasurementData ) );
                } );
            });
    });
    
    if ( !result ) {
        currentLidarMeasurement = LidarResult::error( result.assume_error() );
        Dbg::error( "\nError get: " );
        Dbg::error( result.assume_error().data() );
        IdleTask::defer( [&]( ) { lidarInit( lidar, currentLidarMeasurement ); } );
    } else {
        std::optional< Data > data = result.assume_value();
        if ( data ) {
            currentLidarMeasurement = LidarResult::value( data.value() );
        }
        IdleTask::defer( [&]( ) { lidarGet( lidar, currentLidarMeasurement ); } );
    }
}


int main() {
    bsp::setupBoard();
    HAL_Init();

    Dbg::blockingInfo( "Starting" );

    std::optional< LidarResult > currentLidarMeasurement;

    Slider slider( std::move( bsp::sliderMotor ).value(), bsp::posPins );
    PowerSwitch powerInterface(
        bsp::internalSwitchPin, bsp::internalVoltagePin, bsp::internalCurrentPin,
        bsp::externalSwitchPin, bsp::externalVoltagePin, bsp::externalCurrentPin );
    ConnectorStatus connectorStatus ( bsp::connectorSenseA, bsp::connectorSenseB );
    Lidar lidar( &*bsp::i2c, bsp::lidarEnablePin, bsp::lidarIRQPin );

    lidarInit( lidar, currentLidarMeasurement );

    ConnComInterface connComInterface( std::move( bsp::connectorComm ).value() );

    using Command = SpiInterface::Command;
    SpiInterface spiInterface( std::move( bsp::moduleComm ).value(), bsp::spiCSPin,
        [&]( Command cmd, Block b ) {
            switch( cmd ) {
            case Command::VERSION:
                onCmdVersion( spiInterface );
                break;
            case Command::STATUS:
                onCmdStatus( spiInterface, std::move( b ), connComInterface, connectorStatus, slider, powerInterface, lidar, currentLidarMeasurement );
                break;
            case Command::INTERRUPT:
                onCmdInterrupt( spiInterface, std::move( b ) );
                break;
            case Command::SEND_BLOB:
                onCmdSendBlob( spiInterface, connComInterface );
                break;
            case Command::RECEIVE_BLOB:
                onCmdReceiveBlob( spiInterface, connComInterface );
                break;
            default:
                Dbg::warning( "Unknown command %d", cmd );
            };
        } );
    connComInterface.onNewBlob( [&] { spiInterface.interruptMaster(); } );

    Dbg::blockingInfo( "Ready for operation" );

    while ( true ) {

        powerInterface.run();
        if ( connectorStatus.run() )
            spiInterface.interruptMaster();

        if ( Dbg::available() ) {
            char chr = Dbg::get();
            switch( chr ) {
            case 'e':
                slider.expand();
                Dbg::blockingInfo("Expanding");
                break;
            case 'r':
                slider.retract();
                Dbg::blockingInfo("Retracting");
                break;
            case 's':
                slider.stop();
                Dbg::blockingInfo("Stopping");
                break;
            default:
                Dbg::error( "DBG received: %c, %d", chr, int( chr ) );
            }
        }

        slider.run();

        connComInterface.run();

        IdleTask::run();
    }
}

