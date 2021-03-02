EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 6 6
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L rofi:ESP32-WROVER U6
U 1 1 5E084D17
P 2200 2000
F 0 "U6" H 2200 3375 50  0000 C CNN
F 1 "ESP32-WROVER" H 2200 3284 50  0000 C CNN
F 2 "rofi:ESP32-WROVER" H 2200 2950 50  0001 C CNN
F 3 "https://www.espressif.com/sites/default/files/documentation/esp32-wrover_datasheet_en.pdf" H 2200 2950 50  0001 C CNN
F 4 "356-ESP32-WROB(16MB)" H 2200 2000 50  0001 C CNN "#manf"
	1    2200 2000
	1    0    0    -1  
$EndComp
Text GLabel 1550 1900 0    50   Input ~ 0
ESP_EN
Text Notes 4250 2300 0    50   ~ 0
IO0: LOW => Bootloader
Text Notes 4250 2200 0    50   ~ 0
IO2: Floating/LOW => bootloader
Text Notes 4250 2100 0    50   ~ 0
IO15: LOW => Silence boot message
Text Notes 4250 2000 0    50   ~ 0
IO12: HIGH => 1.8V flash
Text GLabel 7850 2050 2    50   Input ~ 0
TXD0
Text GLabel 3450 2850 2    50   Input ~ 0
RXD0
NoConn ~ 1550 1300
NoConn ~ 1550 1400
NoConn ~ 1550 1500
NoConn ~ 1550 1600
NoConn ~ 1550 1700
NoConn ~ 1550 1200
Wire Wire Line
	1550 2850 1450 2850
Wire Wire Line
	1450 2850 1450 2950
Wire Wire Line
	1450 2950 1550 2950
Wire Wire Line
	1550 3050 1450 3050
Wire Wire Line
	1450 3050 1450 2950
Connection ~ 1450 2950
Wire Wire Line
	1550 3150 1450 3150
Wire Wire Line
	1450 3150 1450 3050
Connection ~ 1450 3050
Wire Wire Line
	1450 3150 1450 3250
Connection ~ 1450 3150
$Comp
L Device:C C40
U 1 1 5E136009
P 1250 2550
F 0 "C40" H 1365 2596 50  0000 L CNN
F 1 "470n" H 1365 2505 50  0000 L CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 1288 2400 50  0001 C CNN
F 3 "~" H 1250 2550 50  0001 C CNN
F 4 "JMK105BJ474KV-F" H 1250 2550 50  0001 C CNN "#manf"
	1    1250 2550
	1    0    0    -1  
$EndComp
$Comp
L Device:C C39
U 1 1 5E1367F8
P 800 2550
F 0 "C39" H 915 2596 50  0000 L CNN
F 1 "22u" H 915 2505 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 838 2400 50  0001 C CNN
F 3 "~" H 800 2550 50  0001 C CNN
F 4 "GRM21BR61C226ME44L" H 800 2550 50  0001 C CNN "#manf"
	1    800  2550
	1    0    0    -1  
$EndComp
Wire Wire Line
	1550 2400 1250 2400
Wire Wire Line
	1250 2400 800  2400
Connection ~ 1250 2400
Text GLabel 2850 2650 2    50   Input ~ 0
SPI_MISO_MOSI
Text GLabel 2850 1650 2    50   Input ~ 0
DOCK_1_CE
Text GLabel 2850 1450 2    50   Input ~ 0
DOCK_2_CE
Text GLabel 2850 1250 2    50   Input ~ 0
DOCK_3_CE
Text GLabel 2850 1150 2    50   Input ~ 0
DOCK_4_CE
Text GLabel 2850 1350 2    50   Input ~ 0
DOCK_5_CE
Text GLabel 2850 1550 2    50   Input ~ 0
DOCK_6_CE
Text GLabel 2850 2750 2    50   Input ~ 0
MOTORS_RX
Wire Wire Line
	2850 3150 2950 3150
Wire Wire Line
	2950 3150 2950 3700
Wire Wire Line
	2850 3050 3050 3050
Wire Wire Line
	3050 3050 3050 3550
Wire Wire Line
	4200 3700 4200 3800
Wire Wire Line
	4200 3450 4200 3550
Text GLabel 2850 2250 2    50   Input ~ 0
BIOS_TX
Text GLabel 2850 2150 2    50   Input ~ 0
BIOS_RX
Text GLabel 2850 2550 2    50   Input ~ 0
SPI_SCK
$Comp
L Connector_Generic:Conn_02x05_Odd_Even J1
U 1 1 5E6930A0
P 4400 1100
F 0 "J1" H 4450 1517 50  0000 C CNN
F 1 "ESP_JTAG" H 4450 1426 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x05_P2.54mm_Vertical" H 4400 1100 50  0001 C CNN
F 3 "~" H 4400 1100 50  0001 C CNN
	1    4400 1100
	1    0    0    -1  
$EndComp
Wire Wire Line
	4200 1000 4100 1000
Wire Wire Line
	4100 1000 4100 1100
Wire Wire Line
	4100 1100 4200 1100
Wire Wire Line
	4100 1100 4100 1200
Wire Wire Line
	4100 1200 4200 1200
Connection ~ 4100 1100
Wire Wire Line
	4100 1200 4100 1300
Wire Wire Line
	4100 1300 4200 1300
Connection ~ 4100 1200
Text GLabel 4700 900  2    50   Input ~ 0
ESP_TMS
Text GLabel 4700 1000 2    50   Input ~ 0
ESP_TCK
Text GLabel 4700 1100 2    50   Input ~ 0
ESP_TDO
Text GLabel 4700 1200 2    50   Input ~ 0
ESP_TDI
NoConn ~ 4700 1300
Text GLabel 2850 1750 2    50   Input ~ 0
ESP_TMS
Text GLabel 2850 2050 2    50   Input ~ 0
ESP_TDO
Text GLabel 2850 1850 2    50   Input ~ 0
ESP_TDI
Text GLabel 2850 1950 2    50   Input ~ 0
ESP_TCK
$Comp
L Sensor_Motion:ICM-20948 U7
U 1 1 5E6C3FF7
P 2100 5950
F 0 "U7" H 2450 6650 50  0000 C CNN
F 1 "ICM-20948" H 2400 5300 50  0000 C CNN
F 2 "Sensor_Motion:InvenSense_QFN-24_3x3mm_P0.4mm" H 2100 4950 50  0001 C CNN
F 3 "http://www.invensense.com/wp-content/uploads/2016/06/DS-000189-ICM-20948-v1.3.pdf" H 2100 5800 50  0001 C CNN
F 4 "ICM-20948" H 2100 5950 50  0001 C CNN "#manf"
	1    2100 5950
	1    0    0    -1  
$EndComp
Wire Wire Line
	4200 3450 4400 3450
Wire Wire Line
	4200 3550 4400 3550
Connection ~ 4200 3550
Wire Wire Line
	4200 3700 4400 3700
Connection ~ 4200 3700
Wire Wire Line
	4200 3800 4400 3800
Connection ~ 3950 3550
Wire Wire Line
	3950 3550 4200 3550
Connection ~ 3950 3700
Wire Wire Line
	3950 3700 4200 3700
NoConn ~ 2600 5850
NoConn ~ 2600 5950
Wire Wire Line
	2200 5250 2200 5050
Wire Wire Line
	2000 5050 2000 5250
Wire Wire Line
	2200 5050 2700 5050
$Comp
L Device:C C43
U 1 1 5E86A657
P 2800 6300
F 0 "C43" H 2685 6254 50  0000 R CNN
F 1 "470n" H 2685 6345 50  0000 R CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 2838 6150 50  0001 C CNN
F 3 "~" H 2800 6300 50  0001 C CNN
F 4 "JMK105BJ474KV-F" H 2800 6300 50  0001 C CNN "#manf"
	1    2800 6300
	-1   0    0    1   
$EndComp
Wire Wire Line
	2800 6150 2600 6150
$Comp
L Device:C C42
U 1 1 5E86746B
P 2700 5200
F 0 "C42" H 2815 5246 50  0000 L CNN
F 1 "470n" H 2815 5155 50  0000 L CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 2738 5050 50  0001 C CNN
F 3 "~" H 2700 5200 50  0001 C CNN
F 4 "JMK105BJ474KV-F" H 2700 5200 50  0001 C CNN "#manf"
	1    2700 5200
	1    0    0    -1  
$EndComp
$Comp
L Device:C C41
U 1 1 5E86F45B
P 1450 5250
F 0 "C41" H 1565 5296 50  0000 L CNN
F 1 "470n" H 1565 5205 50  0000 L CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 1488 5100 50  0001 C CNN
F 3 "~" H 1450 5250 50  0001 C CNN
F 4 "JMK105BJ474KV-F" H 1450 5250 50  0001 C CNN "#manf"
	1    1450 5250
	1    0    0    -1  
$EndComp
Wire Wire Line
	2000 5050 1450 5050
Wire Wire Line
	1450 5050 1450 5100
Wire Wire Line
	1000 5950 1600 5950
Wire Wire Line
	1000 6150 1600 6150
NoConn ~ 2850 850 
NoConn ~ 2850 950 
NoConn ~ 2850 1050
NoConn ~ 1600 6250
$Comp
L Device:R R51
U 1 1 5E9E00D9
P 1150 5650
F 0 "R51" V 943 5650 50  0000 C CNN
F 1 "100k" V 1034 5650 50  0000 C CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 1080 5650 50  0001 C CNN
F 3 "~" H 1150 5650 50  0001 C CNN
F 4 "RT0402FRE07100KL" H 1150 5650 50  0001 C CNN "#manf"
	1    1150 5650
	0    1    1    0   
$EndComp
Wire Wire Line
	1300 5650 1600 5650
$Comp
L Connector_Generic:Conn_01x04 J?
U 1 1 5DFCF9E0
P 4950 4750
AR Path="/5E8D0C73/5DFCF9E0" Ref="J?"  Part="1" 
AR Path="/5E080FD6/5DFCF9E0" Ref="J5"  Part="1" 
F 0 "J5" H 5030 4742 50  0000 L CNN
F 1 "MOTOR_BUS" H 5030 4651 50  0000 L CNN
F 2 "Connector_PinHeader_2.00mm:PinHeader_1x04_P2.00mm_Vertical" H 4950 4750 50  0001 C CNN
F 3 "~" H 4950 4750 50  0001 C CNN
	1    4950 4750
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J?
U 1 1 5DFCF9E6
P 4950 5300
AR Path="/5E8D0C73/5DFCF9E6" Ref="J?"  Part="1" 
AR Path="/5E080FD6/5DFCF9E6" Ref="J6"  Part="1" 
F 0 "J6" H 5030 5342 50  0000 L CNN
F 1 "MOTOR_BUS_ALT" H 5030 5251 50  0000 L CNN
F 2 "Connector_PinHeader_2.00mm:PinHeader_1x03_P2.00mm_Vertical" H 4950 5300 50  0001 C CNN
F 3 "~" H 4950 5300 50  0001 C CNN
	1    4950 5300
	1    0    0    -1  
$EndComp
Text GLabel 4750 4950 0    50   Input ~ 0
MOTORS_TX
Text GLabel 4750 5400 0    50   Input ~ 0
MOTORS_TX
$Comp
L Connector_Generic:Conn_02x05_Odd_Even J?
U 1 1 5DFCF9FC
P 4800 6100
AR Path="/5E8D0C73/5DFCF9FC" Ref="J?"  Part="1" 
AR Path="/5E080FD6/5DFCF9FC" Ref="J4"  Part="1" 
F 0 "J4" H 4850 6517 50  0000 C CNN
F 1 "ShoeAConnectors" H 4850 6426 50  0000 C CNN
F 2 "Connector_PinHeader_2.00mm:PinHeader_2x05_P2.00mm_Vertical" H 4800 6100 50  0001 C CNN
F 3 "~" H 4800 6100 50  0001 C CNN
	1    4800 6100
	1    0    0    -1  
$EndComp
Text GLabel 4600 6200 0    50   Input ~ 0
SPI_SCK
Text GLabel 5100 6200 2    50   Input ~ 0
SPI_MISO_MOSI
Text GLabel 5100 6100 2    50   Input ~ 0
DOCK_1_CE
Text GLabel 5100 6000 2    50   Input ~ 0
DOCK_2_CE
Text GLabel 5100 5900 2    50   Input ~ 0
DOCK_3_CE
$Comp
L Connector_Generic:Conn_01x12 J?
U 1 1 5DFCFA20
P 4950 7100
AR Path="/5E8D0C73/5DFCFA20" Ref="J?"  Part="1" 
AR Path="/5E080FD6/5DFCFA20" Ref="J7"  Part="1" 
F 0 "J7" H 5030 7142 50  0000 L CNN
F 1 "ShoeBConnectors" H 5030 7051 50  0000 L CNN
F 2 "rofi:slipRingBrush" H 4950 7100 50  0001 C CNN
F 3 "~" H 4950 7100 50  0001 C CNN
	1    4950 7100
	1    0    0    -1  
$EndComp
Text GLabel 4750 4850 0    50   Input ~ 0
MOTORS_RX
Text GLabel 4750 6800 0    50   Input ~ 0
MOTORS_RX
Text GLabel 4750 6900 0    50   Input ~ 0
MOTORS_TX
Text GLabel 4750 7200 0    50   Input ~ 0
SPI_SCK
Text GLabel 4750 7300 0    50   Input ~ 0
SPI_MISO_MOSI
Text GLabel 4750 7400 0    50   Input ~ 0
DOCK_4_CE
Text GLabel 4750 7500 0    50   Input ~ 0
DOCK_5_CE
Text GLabel 4750 7600 0    50   Input ~ 0
DOCK_6_CE
Text HLabel 4600 6000 0    50   Input ~ 0
INT
Wire Wire Line
	4600 6100 4250 6100
Text HLabel 4750 4750 0    50   Input ~ 0
BATT_VDD
Text HLabel 4600 5900 0    50   Input ~ 0
BATT_VDD
Text HLabel 4750 5300 0    50   Input ~ 0
BATT_VDD
Text HLabel 4750 6700 0    50   Input ~ 0
BATT_VDD
Text HLabel 4600 6300 0    50   Input ~ 0
GND
Text HLabel 5100 6300 2    50   Input ~ 0
GND
Text HLabel 4750 7700 0    50   Input ~ 0
GND
Text HLabel 4750 5200 0    50   Input ~ 0
GND
Text HLabel 4750 4650 0    50   Input ~ 0
GND
Text HLabel 4750 6600 0    50   Input ~ 0
GND
Text HLabel 4100 1300 3    50   Input ~ 0
GND
Text HLabel 4200 900  0    50   Input ~ 0
3V3
Text HLabel 3950 3250 1    50   Input ~ 0
3V3
Text HLabel 3950 4000 3    50   Input ~ 0
3V3
Text HLabel 1450 3250 3    50   Input ~ 0
GND
Text HLabel 1250 2700 3    50   Input ~ 0
GND
Text HLabel 800  2700 3    50   Input ~ 0
GND
Text HLabel 800  2400 1    50   Input ~ 0
3V3
Text HLabel 1450 5050 0    50   Input ~ 0
3V3
Text HLabel 2700 5050 2    50   Input ~ 0
3V3
Text HLabel 2700 5350 3    50   Input ~ 0
GND
Text HLabel 1450 5400 3    50   Input ~ 0
GND
Text HLabel 1000 5650 0    50   Input ~ 0
GND
Text HLabel 1000 6150 0    50   Input ~ 0
GND
Text HLabel 1000 5950 0    50   Input ~ 0
3V3
Text HLabel 2800 6450 3    50   Input ~ 0
GND
Text HLabel 2100 6650 3    50   Input ~ 0
GND
Text HLabel 1600 5750 0    50   Input ~ 0
SDA
Text HLabel 1600 5850 0    50   Input ~ 0
SCL
Wire Wire Line
	4750 7100 4400 7100
Text HLabel 4750 7000 0    50   Input ~ 0
INT
Text Label 4400 7100 0    50   ~ 0
EXT
Text Label 4250 6100 0    50   ~ 0
EXT
Wire Wire Line
	2850 2950 3450 2950
Wire Wire Line
	2850 2850 3450 2850
Wire Wire Line
	3400 3550 3950 3550
Wire Wire Line
	3050 3550 3400 3550
Connection ~ 3400 3550
$Comp
L Connector:TestPoint TP3
U 1 1 5E8B6CC3
P 3400 3450
F 0 "TP3" H 3458 3568 50  0000 L CNN
F 1 "TP_SCL" H 3458 3477 50  0000 L CNN
F 2 "TestPoint:TestPoint_Pad_D2.0mm" H 3600 3450 50  0001 C CNN
F 3 "~" H 3600 3450 50  0001 C CNN
	1    3400 3450
	1    0    0    -1  
$EndComp
Wire Wire Line
	3400 3450 3400 3550
Wire Wire Line
	3400 3700 3950 3700
Connection ~ 3400 3700
Wire Wire Line
	2950 3700 3400 3700
Wire Wire Line
	3400 3800 3400 3700
$Comp
L Connector:TestPoint TP4
U 1 1 5E8BA514
P 3400 3800
F 0 "TP4" H 3342 3826 50  0000 R CNN
F 1 "TP_SDA" H 3342 3917 50  0000 R CNN
F 2 "TestPoint:TestPoint_Pad_D2.0mm" H 3600 3800 50  0001 C CNN
F 3 "~" H 3600 3800 50  0001 C CNN
	1    3400 3800
	-1   0    0    1   
$EndComp
$Comp
L Device:R R29
U 1 1 5E6E28E0
P 3950 3850
F 0 "R29" H 4020 3896 50  0000 L CNN
F 1 "3k3" H 4020 3805 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 3880 3850 50  0001 C CNN
F 3 "~" H 3950 3850 50  0001 C CNN
F 4 "RR0510P-332-D" H 3950 3850 50  0001 C CNN "#manf"
	1    3950 3850
	1    0    0    -1  
$EndComp
$Comp
L Device:R R28
U 1 1 5E6E0D40
P 3950 3400
F 0 "R28" H 4020 3446 50  0000 L CNN
F 1 "3k3" H 4020 3355 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 3880 3400 50  0001 C CNN
F 3 "~" H 3950 3400 50  0001 C CNN
F 4 "RR0510P-332-D" H 3950 3400 50  0001 C CNN "#manf"
	1    3950 3400
	1    0    0    -1  
$EndComp
Text HLabel 4400 3700 2    50   Input ~ 0
SCL
Text GLabel 4400 3800 2    50   Input ~ 0
BIOS_SCL
Text HLabel 4400 3550 2    50   Input ~ 0
SDA
Text GLabel 4400 3450 2    50   Input ~ 0
BIOS_SDA
Text GLabel 2850 2350 2    50   Input ~ 0
MOTORS_TX
Text HLabel 9400 3150 0    50   Input ~ 0
PWR_START
Text HLabel 9400 3300 0    50   Input ~ 0
PWR_SHUTDOWN
Text GLabel 7850 2150 2    50   Input ~ 0
USB_C_D-
Text GLabel 7850 2250 2    50   Input ~ 0
USB_C_D+
Text HLabel 10200 2600 2    50   Input ~ 0
QC_PULL_UP
Text HLabel 10200 2700 2    50   Input ~ 0
QC_PULL_DOWN
Text Label 8250 1350 0    50   ~ 0
BUS_VOLTAGE
Text Label 8250 1450 0    50   ~ 0
USB_VOLTAGE
Text HLabel 10200 3050 2    50   Input ~ 0
BATT_TO_BUS_EN
Text HLabel 10200 3150 2    50   Input ~ 0
USB_TO_BUS_EN
Text HLabel 10200 3250 2    50   Input ~ 0
SW_LEFT
Text HLabel 10200 3350 2    50   Input ~ 0
SW_MID
Text HLabel 10200 3450 2    50   Input ~ 0
SW_RIGHT
Text HLabel 10200 3550 2    50   Input ~ 0
PWR_SHUTDOWN
$Comp
L MCU_ST_STM32L4:STM32L432KBUx U?
U 1 1 5FF817C5
P 7350 1750
AR Path="/5E2C3773/5FF817C5" Ref="U?"  Part="1" 
AR Path="/5E080FD6/5FF817C5" Ref="U?"  Part="1" 
F 0 "U?" H 7550 2700 50  0000 C CNN
F 1 "STM32L432KBUx" H 7800 2600 50  0000 C CNN
F 2 "Package_DFN_QFN:QFN-32-1EP_5x5mm_P0.5mm_EP3.45x3.45mm" H 6950 850 50  0001 R CNN
F 3 "http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/DM00257205.pdf" H 7350 1750 50  0001 C CNN
	1    7350 1750
	1    0    0    -1  
$EndComp
$Comp
L Device:C C?
U 1 1 5FF817CD
P 8850 900
AR Path="/5E2C3773/5FF817CD" Ref="C?"  Part="1" 
AR Path="/5E080FD6/5FF817CD" Ref="C?"  Part="1" 
F 0 "C?" H 8965 946 50  0000 L CNN
F 1 "22u" H 8965 855 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 8888 750 50  0001 C CNN
F 3 "~" H 8850 900 50  0001 C CNN
F 4 "GRM21BR61C226ME44L" H 8850 900 50  0001 C CNN "#manf"
	1    8850 900 
	1    0    0    -1  
$EndComp
$Comp
L Device:C C?
U 1 1 5FF817D4
P 9250 900
AR Path="/5E2C3773/5FF817D4" Ref="C?"  Part="1" 
AR Path="/5E080FD6/5FF817D4" Ref="C?"  Part="1" 
F 0 "C?" H 9365 946 50  0000 L CNN
F 1 "470n" H 9365 855 50  0000 L CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 9288 750 50  0001 C CNN
F 3 "~" H 9250 900 50  0001 C CNN
F 4 "JMK105BJ474KV-F" H 9250 900 50  0001 C CNN "#manf"
	1    9250 900 
	1    0    0    -1  
$EndComp
$Comp
L Device:C C?
U 1 1 5FF817DB
P 9700 900
AR Path="/5E2C3773/5FF817DB" Ref="C?"  Part="1" 
AR Path="/5E080FD6/5FF817DB" Ref="C?"  Part="1" 
F 0 "C?" H 9815 946 50  0000 L CNN
F 1 "470n" H 9815 855 50  0000 L CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 9738 750 50  0001 C CNN
F 3 "~" H 9700 900 50  0001 C CNN
F 4 "JMK105BJ474KV-F" H 9700 900 50  0001 C CNN "#manf"
	1    9700 900 
	1    0    0    -1  
$EndComp
$Comp
L Device:C C?
U 1 1 5FF817E2
P 10150 900
AR Path="/5E2C3773/5FF817E2" Ref="C?"  Part="1" 
AR Path="/5E080FD6/5FF817E2" Ref="C?"  Part="1" 
F 0 "C?" H 10265 946 50  0000 L CNN
F 1 "470n" H 10265 855 50  0000 L CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 10188 750 50  0001 C CNN
F 3 "~" H 10150 900 50  0001 C CNN
F 4 "JMK105BJ474KV-F" H 10150 900 50  0001 C CNN "#manf"
	1    10150 900 
	1    0    0    -1  
$EndComp
Wire Wire Line
	10150 750  9700 750 
Wire Wire Line
	8850 750  9250 750 
Connection ~ 9700 750 
Connection ~ 9250 750 
Wire Wire Line
	9250 750  9700 750 
Wire Wire Line
	8850 750  7450 750 
Wire Wire Line
	7250 750  7250 850 
Connection ~ 8850 750 
Wire Wire Line
	7350 850  7350 750 
Connection ~ 7350 750 
Wire Wire Line
	7350 750  7250 750 
Wire Wire Line
	7450 750  7450 850 
Connection ~ 7450 750 
Wire Wire Line
	7450 750  7350 750 
Text HLabel 7050 750  0    50   Input ~ 0
3V3
Text GLabel 6850 1050 0    50   Input ~ 0
SWRST
Wire Wire Line
	7050 750  7250 750 
Connection ~ 7250 750 
Text HLabel 8850 1050 3    50   Input ~ 0
GND
Text HLabel 9250 1050 3    50   Input ~ 0
GND
Text HLabel 9700 1050 3    50   Input ~ 0
GND
Text HLabel 10150 1050 3    50   Input ~ 0
GND
Text HLabel 7250 2750 3    50   Input ~ 0
GND
Text HLabel 7350 2750 3    50   Input ~ 0
GND
Text HLabel 7450 2750 3    50   Input ~ 0
GND
Text GLabel 7850 2450 2    50   Input ~ 0
SWCLK
Text GLabel 7850 2350 2    50   Input ~ 0
SWDIO
Text GLabel 8300 5450 2    50   Input ~ 0
USB_C_D+
NoConn ~ 8300 5050
NoConn ~ 8300 4950
$Comp
L Connector_Generic:Conn_01x04 J?
U 1 1 5FF8A74B
P 9200 5450
AR Path="/5E2C3773/5FF8A74B" Ref="J?"  Part="1" 
AR Path="/5E080FD6/5FF8A74B" Ref="J?"  Part="1" 
F 0 "J?" H 9118 5025 50  0000 C CNN
F 1 "USB_ALT" H 9118 5116 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 9200 5450 50  0001 C CNN
F 3 "~" H 9200 5450 50  0001 C CNN
	1    9200 5450
	-1   0    0    1   
$EndComp
Text GLabel 9400 5350 2    50   Input ~ 0
USB_C_D+
Text GLabel 9400 5450 2    50   Input ~ 0
USB_C_D-
Wire Wire Line
	7400 6250 7700 6250
Wire Wire Line
	8600 4750 8550 4750
$Comp
L power:PWR_FLAG #FLG?
U 1 1 5FF8A755
P 8550 4750
AR Path="/5E2C3773/5FF8A755" Ref="#FLG?"  Part="1" 
AR Path="/5E080FD6/5FF8A755" Ref="#FLG?"  Part="1" 
F 0 "#FLG?" H 8550 4825 50  0001 C CNN
F 1 "PWR_FLAG" H 8550 4923 50  0000 C CNN
F 2 "" H 8550 4750 50  0001 C CNN
F 3 "~" H 8550 4750 50  0001 C CNN
	1    8550 4750
	1    0    0    -1  
$EndComp
Connection ~ 8550 4750
Wire Wire Line
	8550 4750 8300 4750
$Comp
L Connector:USB_C_Receptacle_USB2.0 J?
U 1 1 5FF8A75D
P 7700 5350
AR Path="/5E2C3773/5FF8A75D" Ref="J?"  Part="1" 
AR Path="/5E080FD6/5FF8A75D" Ref="J?"  Part="1" 
F 0 "J?" H 7807 6217 50  0000 C CNN
F 1 "USB_C_Receptacle_USB2.0" H 7807 6126 50  0000 C CNN
F 2 "rofi:USB_C_Female-16Pin-HPJF" H 7850 5350 50  0001 C CNN
F 3 "https://www.usb.org/sites/default/files/documents/usb_type-c.zip" H 7850 5350 50  0001 C CNN
	1    7700 5350
	1    0    0    -1  
$EndComp
Wire Wire Line
	8300 5250 8300 5350
Wire Wire Line
	8300 5450 8300 5550
NoConn ~ 8300 5850
NoConn ~ 8300 5950
Text HLabel 7700 6250 3    50   Input ~ 0
GND
Text HLabel 9400 5550 2    50   Input ~ 0
GND
Text HLabel 8600 4750 2    50   Input ~ 0
USB_C_VDD
Text HLabel 9400 5250 2    50   Input ~ 0
USB_C_VDD
Text GLabel 8300 5250 2    50   Input ~ 0
USB_C_D-
$Comp
L Device:R R?
U 1 1 5FF9725F
P 13250 550
AR Path="/5E080FD6/5FF9725F" Ref="R?"  Part="1" 
AR Path="/5E2C3773/5FF9725F" Ref="R?"  Part="1" 
F 0 "R?" H 13320 596 50  0000 L CNN
F 1 "2k2" H 13320 505 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 13180 550 50  0001 C CNN
F 3 "~" H 13250 550 50  0001 C CNN
F 4 "RR0510P-222-D" H 13250 550 50  0001 C CNN "#manf"
	1    13250 550 
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 5FF97266
P 13650 550
AR Path="/5E080FD6/5FF97266" Ref="R?"  Part="1" 
AR Path="/5E2C3773/5FF97266" Ref="R?"  Part="1" 
F 0 "R?" H 13720 596 50  0000 L CNN
F 1 "2k2" H 13720 505 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 13580 550 50  0001 C CNN
F 3 "~" H 13650 550 50  0001 C CNN
F 4 "RR0510P-222-D" H 13650 550 50  0001 C CNN "#manf"
	1    13650 550 
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 5FF9726D
P 13250 1050
AR Path="/5E080FD6/5FF9726D" Ref="R?"  Part="1" 
AR Path="/5E2C3773/5FF9726D" Ref="R?"  Part="1" 
F 0 "R?" H 13320 1096 50  0000 L CNN
F 1 "10k" H 13320 1005 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 13180 1050 50  0001 C CNN
F 3 "~" H 13250 1050 50  0001 C CNN
F 4 "RT0402FRE0710KL" H 13250 1050 50  0001 C CNN "#manf"
	1    13250 1050
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 5FF97274
P 13650 1050
AR Path="/5E080FD6/5FF97274" Ref="R?"  Part="1" 
AR Path="/5E2C3773/5FF97274" Ref="R?"  Part="1" 
F 0 "R?" H 13720 1096 50  0000 L CNN
F 1 "10k" H 13720 1005 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 13580 1050 50  0001 C CNN
F 3 "~" H 13650 1050 50  0001 C CNN
F 4 "RT0402FRE0710KL" H 13650 1050 50  0001 C CNN "#manf"
	1    13650 1050
	1    0    0    -1  
$EndComp
Text GLabel 13850 800  2    50   Input ~ 0
USB_C_D+
Text GLabel 13150 800  0    50   Input ~ 0
USB_C_D-
Wire Wire Line
	13650 700  13650 800 
Wire Wire Line
	13650 800  13850 800 
Connection ~ 13650 800 
Wire Wire Line
	13650 800  13650 900 
Wire Wire Line
	13250 700  13250 800 
Wire Wire Line
	13250 800  13150 800 
Wire Wire Line
	13250 800  13250 900 
Connection ~ 13250 800 
Wire Wire Line
	13250 400  13250 300 
Wire Wire Line
	13250 300  13200 300 
Wire Wire Line
	13650 400  13650 300 
Wire Wire Line
	13650 300  13700 300 
Wire Wire Line
	13650 1200 13650 1400
Wire Wire Line
	13650 1400 13700 1400
Wire Wire Line
	13250 1200 13250 1400
Wire Wire Line
	13250 1400 13200 1400
Text GLabel 13200 300  0    50   Input ~ 0
QC_2_M
Text GLabel 13200 1400 0    50   Input ~ 0
QC_10_N
Text GLabel 13700 1400 2    50   Input ~ 0
QC_10_P
Text GLabel 13700 300  2    50   Input ~ 0
QC_2_P
Text Notes 13200 150  0    50   ~ 0
QuickCharge 3
Text Notes 12750 1700 0    50   ~ 0
https://github.com/Crypter/QC3Client/\nhttp://blog.deconinck.info/post/2017/08/09/Turning-a-Quick-Charge-3.0-charger-into-a-variable-voltage-power-supply
$Comp
L Device:R R?
U 1 1 5FF97293
P 6750 3600
AR Path="/5E2C3773/5FF97293" Ref="R?"  Part="1" 
AR Path="/5E080FD6/5FF97293" Ref="R?"  Part="1" 
F 0 "R?" H 6820 3646 50  0000 L CNN
F 1 "68k" H 6820 3555 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 6680 3600 50  0001 C CNN
F 3 "~" H 6750 3600 50  0001 C CNN
F 4 "RR0510P-683-D" H 6750 3600 50  0001 C CNN "#manf"
	1    6750 3600
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 5FF9729A
P 6750 4000
AR Path="/5E2C3773/5FF9729A" Ref="R?"  Part="1" 
AR Path="/5E080FD6/5FF9729A" Ref="R?"  Part="1" 
F 0 "R?" H 6820 4046 50  0000 L CNN
F 1 "10k" H 6820 3955 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 6680 4000 50  0001 C CNN
F 3 "~" H 6750 4000 50  0001 C CNN
F 4 "RT0402FRE0710KL" H 6750 4000 50  0001 C CNN "#manf"
	1    6750 4000
	1    0    0    -1  
$EndComp
Wire Wire Line
	6750 3750 6750 3800
Wire Wire Line
	6750 3800 7300 3800
Connection ~ 6750 3800
Wire Wire Line
	6750 3800 6750 3850
$Comp
L Device:R R?
U 1 1 5FF972A5
P 5700 3550
AR Path="/5E2C3773/5FF972A5" Ref="R?"  Part="1" 
AR Path="/5E080FD6/5FF972A5" Ref="R?"  Part="1" 
F 0 "R?" H 5770 3596 50  0000 L CNN
F 1 "68k" H 5770 3505 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 5630 3550 50  0001 C CNN
F 3 "~" H 5700 3550 50  0001 C CNN
F 4 "RR0510P-683-D" H 5700 3550 50  0001 C CNN "#manf"
	1    5700 3550
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 5FF972AC
P 5700 4000
AR Path="/5E2C3773/5FF972AC" Ref="R?"  Part="1" 
AR Path="/5E080FD6/5FF972AC" Ref="R?"  Part="1" 
F 0 "R?" H 5770 4046 50  0000 L CNN
F 1 "10k" H 5770 3955 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 5630 4000 50  0001 C CNN
F 3 "~" H 5700 4000 50  0001 C CNN
F 4 "RT0402FRE0710KL" H 5700 4000 50  0001 C CNN "#manf"
	1    5700 4000
	1    0    0    -1  
$EndComp
Wire Wire Line
	6600 3400 6750 3400
Wire Wire Line
	6750 3400 6750 3450
Wire Wire Line
	5450 3400 5700 3400
Wire Wire Line
	5700 3700 5700 3750
Wire Wire Line
	6050 3750 5700 3750
Connection ~ 5700 3750
Wire Wire Line
	5700 3750 5700 3850
$Comp
L Connector_Generic:Conn_01x05 J?
U 1 1 5FF972B9
P 6600 4850
AR Path="/5E2C3773/5FF972B9" Ref="J?"  Part="1" 
AR Path="/5E080FD6/5FF972B9" Ref="J?"  Part="1" 
F 0 "J?" H 6680 4892 50  0000 L CNN
F 1 "BIOS_SWD" H 6680 4801 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x03_P2.54mm_Vertical" H 6600 4850 50  0001 C CNN
F 3 "~" H 6600 4850 50  0001 C CNN
	1    6600 4850
	1    0    0    -1  
$EndComp
Text GLabel 6400 4850 0    50   Input ~ 0
SWRST
Text GLabel 6400 4650 0    50   Input ~ 0
SWCLK
Text GLabel 6400 4750 0    50   Input ~ 0
SWDIO
Wire Wire Line
	6350 5100 6350 5050
Wire Wire Line
	6350 5050 6400 5050
Wire Wire Line
	6050 4950 6400 4950
Text HLabel 6350 5100 3    50   Input ~ 0
GND
Text HLabel 6050 4950 0    50   Input ~ 0
3V3
Text HLabel 6600 3400 0    50   Input ~ 0
USB_C_VDD
Text HLabel 5450 3400 0    50   Input ~ 0
INT
Text HLabel 6750 4150 3    50   Input ~ 0
GND
Text HLabel 5700 4150 3    50   Input ~ 0
GND
Text Label 6950 3800 0    50   ~ 0
USB_VOLTAGE
Text Label 5900 3750 0    50   ~ 0
BUS_VOLTAGE
Text GLabel 3450 2950 2    50   Input ~ 0
TXD0
Text GLabel 7850 1950 2    50   Input ~ 0
RXD0
Text GLabel 9750 1900 2    50   Input ~ 0
ESP_EN
Text GLabel 7850 1250 2    50   Input ~ 0
BIOS_RX
Text GLabel 7850 2550 2    50   Input ~ 0
BIOS_TX
Wire Wire Line
	7850 1350 8250 1350
Wire Wire Line
	8250 1450 7850 1450
$Comp
L Device:R R?
U 1 1 600E42B4
P 7700 3650
AR Path="/5DFADF1E/600E42B4" Ref="R?"  Part="1" 
AR Path="/5E2C3773/600E42B4" Ref="R?"  Part="1" 
AR Path="/5E080FD6/600E42B4" Ref="R?"  Part="1" 
F 0 "R?" H 7770 3696 50  0000 L CNN
F 1 "20k" H 7770 3605 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 7630 3650 50  0001 C CNN
F 3 "~" H 7700 3650 50  0001 C CNN
F 4 "RR0510P-203-D" H 7700 3650 50  0001 C CNN "#manf"
F 5 "C25765" H 7700 3650 50  0001 C CNN "LCSC"
	1    7700 3650
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 600E42BC
P 7700 3950
AR Path="/5DFADF1E/600E42BC" Ref="R?"  Part="1" 
AR Path="/5E2C3773/600E42BC" Ref="R?"  Part="1" 
AR Path="/5E080FD6/600E42BC" Ref="R?"  Part="1" 
F 0 "R?" H 7770 3996 50  0000 L CNN
F 1 "10k" H 7770 3905 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 7630 3950 50  0001 C CNN
F 3 "~" H 7700 3950 50  0001 C CNN
F 4 "RT0402FRE0710KL" H 7700 3950 50  0001 C CNN "#manf"
F 5 "C25744" H 7700 3950 50  0001 C CNN "LCSC"
	1    7700 3950
	1    0    0    -1  
$EndComp
Wire Wire Line
	7700 3800 8350 3800
Connection ~ 7700 3800
$Comp
L Connector:TestPoint TP?
U 1 1 600E42C4
P 7700 3350
AR Path="/5DFADF1E/600E42C4" Ref="TP?"  Part="1" 
AR Path="/5E2C3773/600E42C4" Ref="TP?"  Part="1" 
AR Path="/5E080FD6/600E42C4" Ref="TP?"  Part="1" 
F 0 "TP?" H 7758 3468 50  0000 L CNN
F 1 "TP_BAT" H 7758 3377 50  0000 L CNN
F 2 "TestPoint:TestPoint_Pad_D2.0mm" H 7900 3350 50  0001 C CNN
F 3 "~" H 7900 3350 50  0001 C CNN
	1    7700 3350
	1    0    0    -1  
$EndComp
Wire Wire Line
	7600 3350 7700 3350
Wire Wire Line
	7700 3350 7700 3500
Connection ~ 7700 3350
Text HLabel 7600 3350 0    50   Input ~ 0
BAT_VDD
Text HLabel 7700 4100 3    50   UnSpc ~ 0
GND
Text Label 8350 3800 0    50   ~ 0
BATT_VOLTAGE
Text Label 8250 1550 0    50   ~ 0
BATT_VOLTAGE
Wire Wire Line
	7850 1550 8250 1550
$EndSCHEMATC
