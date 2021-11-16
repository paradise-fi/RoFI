#pragma once

#include <drivers/uart.hpp>
#include <drivers/usb.hpp>

using PrimaryAllocator = memory::Pool;

/* USB generic */
#define USB_VID      0x0483
#define USB_PID      0x5740
#define USB_MFR_STR  u"Paradise"
#define USB_PROD_STR u"RoFI Universal Module"
#define USB_SER_STR  u"RUM1"

/* USB <--> ESP tunnel */
#define ESP_TUNNEL_UART            USART1
#define ESP_TUNNEL_RX_PIN          GpioA[ 10 ]
#define ESP_TUNNEL_TX_PIN          GpioA[ 9 ]
#define ESP_TUNNEL_RX_DMA_ALLOCATE Dma::allocate( DMA2, 2 )
#define ESP_TUNNEL_TX_DMA_ALLOCATE Dma::allocate( DMA2, 7 )
#define ESP_TUNNEL_BOOT_PIN        GpioB[ 5 ]
#define ESP_TUNNEL_EN_PIN          GpioB[ 4 ]
#define ESP_TUNNEL_PACKET_SIZE     64
#define ESP_TUNNEL_MGMT_EP         0x82
#define ESP_TUNNEL_RX_EP           0x01
#define ESP_TUNNEL_TX_EP           0x81


