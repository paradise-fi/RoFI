#pragma once

#include <drivers/spi.hpp>
#include <drivers/dma.hpp>
#include <system/memory.hpp>

template < typename AllocatorT >
struct SpiReaderWriter {
    using Allocator = AllocatorT;
    using Mem = typename Allocator::Block;

    SpiReaderWriter( Spi& spi, Dma::Channel rxChannel, Dma::Channel txChannel )
        : _spi( spi )
    {
        _rxChannel = std::move( rxChannel );
        assert( _rxChannel );
        _setupRx();

        _txChannel = std::move( txChannel );
        assert( _txChannel );
        _setupTx();
    }

    template < typename Callback >
    void readBlock( Mem block, int offset, int size, Callback callback ) {
        assert( block );

        LL_DMA_DisableChannel( _rxChannel, _rxChannel );
        LL_DMA_DisableChannel( _txChannel, _txChannel );
        _rxBlock = std::move( block );

        LL_SPI_SetTransferDirection( _spi.periph(), LL_SPI_HALF_DUPLEX_RX );
        LL_DMA_SetMemoryAddress( _rxChannel, _rxChannel, uint32_t( _rxBlock.get() + offset ) );
        LL_DMA_SetDataLength( _rxChannel, _rxChannel, size );

        _rxChannel.onComplete( [this, callback, size]() {
            LL_DMA_DisableChannel( _rxChannel, _rxChannel );
            int read = size - LL_DMA_GetDataLength( _rxChannel, _rxChannel );
            callback( std::move( _rxBlock ), read );
        } );

        LL_DMA_EnableChannel( _rxChannel, _rxChannel );
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

        _txChannel.onComplete( [this, callback, size]() {
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
