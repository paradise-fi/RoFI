#pragma once

#include <cassert>
#include <functional>

#if defined(STM32G0xx)
    #include <drivers/stm32g0xx/spi.hpp>
#elif defined(STM32F0xx)
    #include <drivers/stm32f0xx/spi.hpp>
#else
    #error "Unsuported MCU family"
#endif

#include <drivers/peripheral.hpp>
#include <drivers/gpio.hpp>
#include <drivers/dma.hpp>
#include <system/memory.hpp>

extern "C" void SPI1_IRQHandler();
extern "C" void SPI2_IRQHandler();

struct Spi: public Peripheral< SPI_TypeDef >, public detail::Spi< Spi > {
    template < typename... Configs >
    Spi( SPI_TypeDef *spi, Configs... configs )
        : Peripheral< SPI_TypeDef >( spi )
    {
        enableClock();

        LL_SPI_InitTypeDef SPI_InitStruct{};
        SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
        SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
        SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
        SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
        SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
        SPI_InitStruct.NSS = LL_SPI_NSS_HARD_INPUT;
        SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
        SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
        SPI_InitStruct.CRCPoly = 7;
        LL_SPI_Init( _periph, &SPI_InitStruct );
        LL_SPI_DisableNSSPulseMgt( _periph );
        LL_SPI_SetRxFIFOThreshold( _periph, LL_SPI_RX_FIFO_TH_QUARTER );

        ( configs.post( _periph ), ... );
        _enableInterrupt( 1 );
    }

    void enable() {
        LL_SPI_Enable( _periph );
    }

    void disable() {
        LL_SPI_Disable( _periph );
    }

    void clearRx() {
        if ( LL_SPI_GetRxFIFOThreshold( _periph ) == LL_SPI_RX_FIFO_TH_QUARTER ) {
            while ( LL_SPI_IsActiveFlag_RXNE( _periph ) )
                LL_SPI_ReceiveData8( _periph );
        } else {
            while ( LL_SPI_IsActiveFlag_RXNE( _periph ) )
                LL_SPI_ReceiveData16( _periph );
        }

    }

    bool available() {
        return LL_SPI_IsActiveFlag_RXNE( _periph );
    }

    template < typename Callback >
    void onTransactionBegins( Callback c ) {
        handlers().begin = c;
    }

    template < typename Callback >
    void onTransactionEnds( Callback c ) {
        handlers().end = c;
    }

    void enableTx() {
        LL_SPI_EnableIT_TXE( _periph );
    }

    template < typename Callback >
    void onTx( Callback c ) {
        handlers().tx = c;
    }

    void disableTx() {
        LL_SPI_DisableIT_TXE( _periph );
    }

    void enableRx() {
        LL_SPI_SetRxFIFOThreshold( _periph, LL_SPI_RX_FIFO_TH_QUARTER );
        LL_SPI_EnableIT_RXNE( _periph );
    }

    template < typename Callback >
    void onRx( Callback c ) {
        handlers().rx = c;
    }

    void disableRx() {
        LL_SPI_DisableIT_RXNE( _periph );
    }

    uint8_t read() {
        return LL_SPI_ReceiveData8( _periph );
    }

private:
    void _enableInterrupt( int priority = 0 ) {
        if ( _periph == SPI1 ) {
            NVIC_SetPriority( SPI1_IRQn, priority );
            NVIC_EnableIRQ( SPI1_IRQn );
        }
        else if ( _periph == SPI2 ) {
            NVIC_SetPriority( SPI2_IRQn, priority );
            NVIC_EnableIRQ( SPI2_IRQn );
        }
    }

    using Handler = std::function< void() >;

    struct Handlers {
        Handler begin;
        Handler end;
        Handler rx;
        Handler tx;

        void _handleIsr( SPI_TypeDef *spi ) {
            if (LL_SPI_IsEnabledIT_RXNE( spi ) && LL_SPI_IsActiveFlag_RXNE( spi ) ) {
                rx();
            }
            if ( LL_SPI_IsEnabledIT_TXE( SPI1 ) && LL_SPI_IsActiveFlag_TXE( SPI1 ) ) {
                tx();
            }
        }

        void _onBegin() {
            if ( begin )
                begin();
        }

        void _onEnd() {
            if ( end )
                end();
        }
    };

    static Handlers& handlers( SPI_TypeDef *spi ) {
        if ( spi == SPI1 )
            return _spis[ 0 ];
        if ( spi == SPI2 )
            return _spis[ 1 ];
        assert( false && "Invalid SPI specified" );
        __builtin_trap();
    }

    Handlers& handlers() {
        return handlers( _periph );
    }

    static std::array< Handlers, 2 > _spis;

    friend void SPI1_IRQHandler();
    friend void SPI2_IRQHandler();
    friend struct CsOn;
};

template < typename Self >
struct PinCfg {
    Self& self() { return *static_cast< Self * >( this ); }
    void post( SPI_TypeDef *periph ) {
        self()._pin.port().enableClock();

        LL_GPIO_InitTypeDef cfg{};
        cfg.Pin = 1 << self()._pin._pos;
        cfg.Mode = LL_GPIO_MODE_ALTERNATE;
        cfg.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
        cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        cfg.Pull = LL_GPIO_PULL_NO;
        cfg.Alternate = self().alternativeFun( periph );
        LL_GPIO_Init( self()._pin._periph, &cfg );
    }
};

struct CsOn: public PinCfg< CsOn >, public detail::CsOn< CsOn > {
    CsOn( Gpio::Pin pin ): _pin( pin ) {}

    void post( SPI_TypeDef *periph ) {
        PinCfg< CsOn >::post( periph );
        _pin.setupInterrupt( LL_EXTI_TRIGGER_RISING_FALLING, [periph]( bool rising ) {
            if ( rising ) {
                // Transaction ends
                LL_SPI_Disable( periph );
                Spi::handlers( periph )._onEnd();
            }
            else {
                // Transaction begins
                LL_SPI_Enable( periph );
                Spi::handlers( periph )._onBegin();
            }
        });
    }

    Gpio::Pin _pin;
};

struct SckOn: public PinCfg< SckOn >, public detail::SckOn< SckOn > {
    SckOn( Gpio::Pin pin ): _pin( pin ) {}
    Gpio::Pin _pin;
};

struct MisoOn: public PinCfg< MisoOn >, public detail::MisoOn< MisoOn > {
    MisoOn( Gpio::Pin pin ): _pin( pin ) {}
    Gpio::Pin _pin;
};

struct Slave {
    void post( SPI_TypeDef *periph ) {
        LL_SPI_SetMode( periph, LL_SPI_MODE_SLAVE );
        LL_SPI_SetNSSMode( periph, LL_SPI_NSS_HARD_INPUT );
    }
};

struct SpiReaderWriter {
    using Mem = memory::Pool::Block;

    SpiReaderWriter( Spi& spi, int dmaRxChannel = 0, int dmaTxChannel = 0 )
        : _spi( spi )
    {
        LL_AHB1_GRP1_EnableClock( LL_AHB1_GRP1_PERIPH_DMA1 );

        _rxChannel = Dma::allocate( dmaRxChannel );
        assert( _rxChannel );
        _setupRx();

        _txChannel = Dma::allocate( dmaTxChannel );
        assert( _txChannel );
        _setupTx();
    }

    template < typename Callback >
    void readBlock( Mem block, int offset, int size, Callback callback ) {
        assert( block );

        LL_DMA_DisableChannel( DMA1, _rxChannel );
        LL_DMA_DisableChannel( DMA1, _txChannel );
        _rxBlock = std::move( block );

        LL_SPI_SetTransferDirection( _spi.periph(), LL_SPI_HALF_DUPLEX_RX );
        LL_DMA_SetMemoryAddress( DMA1, _rxChannel, uint32_t( _rxBlock.get() + offset ) );
        LL_DMA_SetDataLength( DMA1, _rxChannel, size );

        _rxChannel.onComplete( [&, callback, size]( int /* channel */ ) {
            LL_DMA_DisableChannel( DMA1, _rxChannel );
            int read = size - LL_DMA_GetDataLength( DMA1, _rxChannel );
            callback( std::move( _rxBlock ), read );
        } );

        LL_DMA_EnableChannel( DMA1, _rxChannel );
    }

    void abortRx() {
        _rxChannel.abort();
    }

    template < typename Callback >
    void writeBlock( Mem block, int offset, int size, Callback callback ) {
        assert( block );

        LL_DMA_DisableChannel( DMA1, _txChannel );
        LL_DMA_DisableChannel( DMA1, _rxChannel );
        _txBlock = std::move( block );

        LL_SPI_SetTransferDirection( _spi.periph(), LL_SPI_HALF_DUPLEX_TX );
        LL_DMA_SetMemoryAddress( DMA1, _txChannel, uint32_t( _txBlock.get() + offset ) );
        LL_DMA_SetDataLength( DMA1, _txChannel, size );

        _txChannel.onComplete( [&, callback, size]( int /* channel */ ) {
            LL_DMA_DisableChannel( DMA1, _txChannel );
            int sent = size - LL_DMA_GetDataLength( DMA1, _txChannel );
            callback( std::move( _txBlock ), sent );
        } );

        LL_DMA_EnableChannel( DMA1, _txChannel );
    }

    void abortTx() {
        _txChannel.abort();
    }

public:
    void _setupRx() {
        // If DMA supports muxing
        #ifdef DMAMUX1
            LL_DMA_SetPeriphRequest (DMA1, _rxChannel, LL_DMAMUX_REQ_RX( _spi.periph() ) );
        #endif
        LL_DMA_SetDataTransferDirection( DMA1, _rxChannel, LL_DMA_DIRECTION_PERIPH_TO_MEMORY );
        LL_DMA_SetChannelPriorityLevel( DMA1, _rxChannel, LL_DMA_PRIORITY_LOW );
        LL_DMA_SetMode( DMA1, _rxChannel, LL_DMA_MODE_NORMAL );
        LL_DMA_SetPeriphIncMode( DMA1, _rxChannel, LL_DMA_PERIPH_NOINCREMENT );
        LL_DMA_SetMemoryIncMode( DMA1, _rxChannel, LL_DMA_MEMORY_INCREMENT );
        LL_DMA_SetPeriphSize( DMA1, _rxChannel, LL_DMA_PDATAALIGN_BYTE );
        LL_DMA_SetMemorySize( DMA1, _rxChannel, LL_DMA_MDATAALIGN_BYTE);
        LL_DMA_SetPeriphAddress( DMA1, _rxChannel, LL_SPI_DMA_GetRegAddr( _spi.periph() ) );
        LL_SPI_EnableDMAReq_RX( _spi.periph() );
        _rxChannel.enableInterrupt();
    }

    void _setupTx() {
        // If DMA supports muxing
        #ifdef DMAMUX1
            LL_DMA_SetPeriphRequest( DMA1, _txChannel, LL_DMAMUX_REQ_TX( _spi.periph() ) );
        #endif
        LL_DMA_SetDataTransferDirection( DMA1, _txChannel, LL_DMA_DIRECTION_MEMORY_TO_PERIPH );
        LL_DMA_SetChannelPriorityLevel( DMA1, _txChannel, LL_DMA_PRIORITY_LOW );
        LL_DMA_SetMode( DMA1, _txChannel, LL_DMA_MODE_NORMAL );
        LL_DMA_SetPeriphIncMode( DMA1, _txChannel, LL_DMA_PERIPH_NOINCREMENT );
        LL_DMA_SetMemoryIncMode( DMA1, _txChannel, LL_DMA_MEMORY_INCREMENT );
        LL_DMA_SetPeriphSize( DMA1, _txChannel, LL_DMA_PDATAALIGN_BYTE );
        LL_DMA_SetMemorySize( DMA1, _txChannel, LL_DMA_MDATAALIGN_BYTE );
        LL_DMA_SetPeriphAddress( DMA1, _txChannel, LL_SPI_DMA_GetRegAddr( _spi.periph() ) );
        LL_SPI_EnableDMAReq_TX( _spi.periph() );
        _txChannel.enableInterrupt();
    }



    Spi& _spi;
    Dma::Channel _txChannel, _rxChannel;
    Mem _rxBlock, _txBlock;
    int _placeholder;
};