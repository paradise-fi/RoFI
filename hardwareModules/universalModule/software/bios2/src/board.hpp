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

/* Power management */
#define PM_SHUTDOWN_PIN       GpioB[ 1 ]
#define PM_CHG_OK_PIN         GpioB[ 2 ]
#define PM_CHG_EN_PIN         GpioB[ 10 ]
#define PM_BATT_TO_BUS_EN_PIN GpioB[ 6 ]
#define PM_USB_TO_BUS_EN_PIN  GpioB[ 9 ]
#define PM_BUS_VOLTAGE_PIN    GpioA[ 0 ]
#define PM_USB_VOLTAGE_PIN    GpioA[ 1 ]
#define PM_BATT_VOLTAGE_PIN   GpioA[ 4 ]
#define PM_ADC_PERIPH         Adc1
#define ADC_REF               3.3   // V
#define ADC_BIT_RES           12    // bits
#define PM_USB_R_DIVIDER_TOP  68.0
#define PM_USB_R_DIVIDER_BOT  10.0
#define PM_BATT_R_DIVIDER_TOP 20.0
#define PM_BATT_R_DIVIDER_BOT 10.0
#define PM_BUS_R_DIVIDER_TOP  68.0
#define PM_BUS_R_DIVIDER_BOT  10.0

/* USB QC */
#define QC_2_M_PIN  GpioB[ 0 ]
#define QC_10_P_PIN GpioB[ 3 ]
#define QC_10_N_PIN GpioB[ 12 ]
#define QC_2_P_PIN  GpioB[ 14 ]
#define QC_USB_DP   GpioA[ 12 ]
#define QC_USB_DM   GpioA[ 11 ]
