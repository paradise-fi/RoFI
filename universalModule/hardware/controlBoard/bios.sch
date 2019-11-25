EESchema Schematic File Version 4
LIBS:controlBoard-cache
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 4 6
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
L Device:R R?
U 1 1 5E2D4CCB
P 1900 6050
AR Path="/5E080FD6/5E2D4CCB" Ref="R?"  Part="1" 
AR Path="/5E2C3773/5E2D4CCB" Ref="R38"  Part="1" 
F 0 "R38" H 1970 6096 50  0000 L CNN
F 1 "2k2" H 1970 6005 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 1830 6050 50  0001 C CNN
F 3 "~" H 1900 6050 50  0001 C CNN
F 4 "RR0510P-222-D" H 1900 6050 50  0001 C CNN "#manf"
	1    1900 6050
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 5E2D4CD1
P 2300 6050
AR Path="/5E080FD6/5E2D4CD1" Ref="R?"  Part="1" 
AR Path="/5E2C3773/5E2D4CD1" Ref="R40"  Part="1" 
F 0 "R40" H 2370 6096 50  0000 L CNN
F 1 "2k2" H 2370 6005 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 2230 6050 50  0001 C CNN
F 3 "~" H 2300 6050 50  0001 C CNN
F 4 "RR0510P-222-D" H 2300 6050 50  0001 C CNN "#manf"
	1    2300 6050
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 5E2D4CD7
P 1900 6550
AR Path="/5E080FD6/5E2D4CD7" Ref="R?"  Part="1" 
AR Path="/5E2C3773/5E2D4CD7" Ref="R39"  Part="1" 
F 0 "R39" H 1970 6596 50  0000 L CNN
F 1 "10k" H 1970 6505 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 1830 6550 50  0001 C CNN
F 3 "~" H 1900 6550 50  0001 C CNN
F 4 "RT0402FRE0710KL" H 1900 6550 50  0001 C CNN "#manf"
	1    1900 6550
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 5E2D4CDD
P 2300 6550
AR Path="/5E080FD6/5E2D4CDD" Ref="R?"  Part="1" 
AR Path="/5E2C3773/5E2D4CDD" Ref="R41"  Part="1" 
F 0 "R41" H 2370 6596 50  0000 L CNN
F 1 "10k" H 2370 6505 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 2230 6550 50  0001 C CNN
F 3 "~" H 2300 6550 50  0001 C CNN
F 4 "RT0402FRE0710KL" H 2300 6550 50  0001 C CNN "#manf"
	1    2300 6550
	1    0    0    -1  
$EndComp
Text GLabel 2500 6300 2    50   Input ~ 0
USB_C_D+
Text GLabel 1800 6300 0    50   Input ~ 0
USB_C_D-
Wire Wire Line
	2300 6200 2300 6300
Wire Wire Line
	2300 6300 2500 6300
Connection ~ 2300 6300
Wire Wire Line
	2300 6300 2300 6400
Wire Wire Line
	1900 6200 1900 6300
Wire Wire Line
	1900 6300 1800 6300
Wire Wire Line
	1900 6300 1900 6400
Connection ~ 1900 6300
Wire Wire Line
	1900 5900 1900 5800
Wire Wire Line
	1900 5800 1850 5800
Wire Wire Line
	2300 5900 2300 5800
Wire Wire Line
	2300 5800 2350 5800
Wire Wire Line
	2300 6700 2300 6900
Wire Wire Line
	2300 6900 2350 6900
Wire Wire Line
	1900 6700 1900 6900
Wire Wire Line
	1900 6900 1850 6900
Text GLabel 1850 5800 0    50   Input ~ 0
QC_2_M
Text GLabel 1850 6900 0    50   Input ~ 0
QC_10_N
Text GLabel 2350 6900 2    50   Input ~ 0
QC_10_P
Text GLabel 2350 5800 2    50   Input ~ 0
QC_2_P
Text Notes 1850 5650 0    50   ~ 0
QuickCharge 3
Text Notes 1400 7200 0    50   ~ 0
https://github.com/Crypter/QC3Client/\nhttp://blog.deconinck.info/post/2017/08/09/Turning-a-Quick-Charge-3.0-charger-into-a-variable-voltage-power-supply
Text GLabel 3550 3500 0    50   Input ~ 0
BIOS_SDA
Text GLabel 3550 3400 0    50   Input ~ 0
BIOS_SCL
Text GLabel 3550 3200 0    50   Input ~ 0
BIOS_TX
Text GLabel 3550 3300 0    50   Input ~ 0
BIOS_RX
Text GLabel 4750 3000 2    50   Input ~ 0
QC_2_M
Text GLabel 4750 3200 2    50   Input ~ 0
QC_10_N
Text GLabel 3550 2700 0    50   Input ~ 0
QC_2_P
Text GLabel 3550 2800 0    50   Input ~ 0
QC_10_P
Text GLabel 3550 3000 0    50   Input ~ 0
USB_BRIDGE_EN
Text GLabel 3550 1300 0    50   Input ~ 0
SWRST
Wire Wire Line
	4050 4300 4050 4400
Wire Wire Line
	4050 4400 4150 4400
Wire Wire Line
	4150 4400 4150 4300
Wire Wire Line
	4250 4300 4250 4400
Wire Wire Line
	4250 4400 4150 4400
Connection ~ 4150 4400
Wire Wire Line
	4150 4500 4150 4400
NoConn ~ 3550 1500
$Comp
L Device:R R42
U 1 1 5E461392
P 6600 1200
F 0 "R42" H 6670 1246 50  0000 L CNN
F 1 "68k" H 6670 1155 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 6530 1200 50  0001 C CNN
F 3 "~" H 6600 1200 50  0001 C CNN
F 4 "RR0510P-683-D" H 6600 1200 50  0001 C CNN "#manf"
	1    6600 1200
	1    0    0    -1  
$EndComp
$Comp
L Device:R R43
U 1 1 5E4619AD
P 6600 1600
F 0 "R43" H 6670 1646 50  0000 L CNN
F 1 "10k" H 6670 1555 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 6530 1600 50  0001 C CNN
F 3 "~" H 6600 1600 50  0001 C CNN
F 4 "RT0402FRE0710KL" H 6600 1600 50  0001 C CNN "#manf"
	1    6600 1600
	1    0    0    -1  
$EndComp
Wire Wire Line
	6600 1350 6600 1400
Wire Wire Line
	6600 1400 7150 1400
Connection ~ 6600 1400
Wire Wire Line
	6600 1400 6600 1450
$Comp
L Device:R R44
U 1 1 5E466449
P 6600 2700
F 0 "R44" H 6670 2746 50  0000 L CNN
F 1 "68k" H 6670 2655 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 6530 2700 50  0001 C CNN
F 3 "~" H 6600 2700 50  0001 C CNN
F 4 "RR0510P-683-D" H 6600 2700 50  0001 C CNN "#manf"
	1    6600 2700
	1    0    0    -1  
$EndComp
$Comp
L Device:R R45
U 1 1 5E466EEE
P 6600 3150
F 0 "R45" H 6670 3196 50  0000 L CNN
F 1 "10k" H 6670 3105 50  0000 L CNN
F 2 "Resistor_SMD:R_0402_1005Metric" V 6530 3150 50  0001 C CNN
F 3 "~" H 6600 3150 50  0001 C CNN
F 4 "RT0402FRE0710KL" H 6600 3150 50  0001 C CNN "#manf"
	1    6600 3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	6450 1000 6600 1000
Wire Wire Line
	6600 1000 6600 1050
Wire Wire Line
	6350 2550 6600 2550
Wire Wire Line
	6600 2850 6600 2900
Wire Wire Line
	6950 2900 6600 2900
Connection ~ 6600 2900
Wire Wire Line
	6600 2900 6600 3000
Text GLabel 1900 2150 2    50   Input ~ 0
USB_C_D+
Text GLabel 1900 1950 2    50   Input ~ 0
USB_C_D-
NoConn ~ 1900 1750
NoConn ~ 1900 1650
$Comp
L Connector_Generic:Conn_01x04 J3
U 1 1 5E52BC10
P 1050 3700
F 0 "J3" H 968 3275 50  0000 C CNN
F 1 "USB_ALT" H 968 3366 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 1050 3700 50  0001 C CNN
F 3 "~" H 1050 3700 50  0001 C CNN
	1    1050 3700
	-1   0    0    1   
$EndComp
Text GLabel 1250 3600 2    50   Input ~ 0
USB_C_D+
Text GLabel 1250 3700 2    50   Input ~ 0
USB_C_D-
Wire Wire Line
	1000 2950 1300 2950
Wire Wire Line
	4050 1100 4050 1000
Wire Wire Line
	4050 1000 4150 1000
Wire Wire Line
	4150 1000 4150 1100
Wire Wire Line
	4150 1000 4250 1000
Wire Wire Line
	4250 1000 4250 1100
Connection ~ 4150 1000
Wire Wire Line
	4250 1000 4350 1000
Wire Wire Line
	4350 1000 4350 1100
Connection ~ 4250 1000
Wire Wire Line
	4050 1000 3850 1000
Connection ~ 4050 1000
Wire Wire Line
	4350 1000 5000 1000
Connection ~ 4350 1000
$Comp
L Device:C C51
U 1 1 5E55A60F
P 5000 1150
F 0 "C51" H 5115 1196 50  0000 L CNN
F 1 "22u" H 5115 1105 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 5038 1000 50  0001 C CNN
F 3 "~" H 5000 1150 50  0001 C CNN
F 4 "GRM21BR61C226ME44L" H 5000 1150 50  0001 C CNN "#manf"
	1    5000 1150
	1    0    0    -1  
$EndComp
Connection ~ 5000 1000
Wire Wire Line
	5000 1000 5400 1000
$Comp
L Device:C C52
U 1 1 5E55BC62
P 5400 1150
F 0 "C52" H 5515 1196 50  0000 L CNN
F 1 "470n" H 5515 1105 50  0000 L CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 5438 1000 50  0001 C CNN
F 3 "~" H 5400 1150 50  0001 C CNN
F 4 "JMK105BJ474KV-F" H 5400 1150 50  0001 C CNN "#manf"
	1    5400 1150
	1    0    0    -1  
$EndComp
$Comp
L MCU_ST_STM32F0:STM32F030C8Tx U9
U 1 1 5E2C5D19
P 4150 2700
F 0 "U9" H 4550 4250 50  0000 C CNN
F 1 "STM32F030C8Tx" H 4300 3750 50  0000 C CNN
F 2 "Package_QFP:LQFP-48_7x7mm_P0.5mm" H 3650 1200 50  0001 R CNN
F 3 "http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/DM00088500.pdf" H 4150 2700 50  0001 C CNN
F 4 "STM32F030C8T6TR" H 4150 2700 50  0001 C CNN "#manf"
	1    4150 2700
	1    0    0    -1  
$EndComp
Wire Wire Line
	2200 1450 2050 1450
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 5E92975E
P 2050 1450
F 0 "#FLG0101" H 2050 1525 50  0001 C CNN
F 1 "PWR_FLAG" H 2050 1623 50  0000 C CNN
F 2 "" H 2050 1450 50  0001 C CNN
F 3 "~" H 2050 1450 50  0001 C CNN
	1    2050 1450
	1    0    0    -1  
$EndComp
Connection ~ 2050 1450
Wire Wire Line
	2050 1450 1900 1450
NoConn ~ 3550 1700
NoConn ~ 3550 1800
NoConn ~ 3550 1900
NoConn ~ 3550 2000
NoConn ~ 4750 3500
NoConn ~ 4750 2700
NoConn ~ 4750 3100
Text GLabel 4750 4000 2    50   Input ~ 0
SWCLK
Text GLabel 4750 3900 2    50   Input ~ 0
SWDIO
$Comp
L Connector_Generic:Conn_01x05 J8
U 1 1 5E94226A
P 5400 5100
F 0 "J8" H 5480 5142 50  0000 L CNN
F 1 "BIOS_SWD" H 5480 5051 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x03_P2.54mm_Vertical" H 5400 5100 50  0001 C CNN
F 3 "~" H 5400 5100 50  0001 C CNN
	1    5400 5100
	1    0    0    -1  
$EndComp
Text GLabel 5200 5100 0    50   Input ~ 0
SWRST
Text GLabel 5200 4900 0    50   Input ~ 0
SWCLK
Text GLabel 5200 5000 0    50   Input ~ 0
SWDIO
Wire Wire Line
	5150 5350 5150 5300
Wire Wire Line
	5150 5300 5200 5300
Wire Wire Line
	4850 5200 5200 5200
Text GLabel 3550 2900 0    50   Input ~ 0
USB_CLK
Text GLabel 3550 3100 0    50   Input ~ 0
USB_SUSPEND
$Comp
L Connector:USB_C_Receptacle_USB2.0 J9
U 1 1 5DDC7481
P 1300 2050
F 0 "J9" H 1407 2917 50  0000 C CNN
F 1 "USB_C_Receptacle_USB2.0" H 1407 2826 50  0000 C CNN
F 2 "rofi:USB_C_Female-16Pin-HPJF" H 1450 2050 50  0001 C CNN
F 3 "https://www.usb.org/sites/default/files/documents/usb_type-c.zip" H 1450 2050 50  0001 C CNN
	1    1300 2050
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 1950 1900 2050
Wire Wire Line
	1900 2150 1900 2250
NoConn ~ 1900 2550
NoConn ~ 1900 2650
Text HLabel 1300 2950 3    50   Input ~ 0
GND
Text HLabel 1250 3800 2    50   Input ~ 0
GND
Text HLabel 4150 4500 3    50   Input ~ 0
GND
Text HLabel 5150 5350 3    50   Input ~ 0
GND
Text HLabel 4850 5200 0    50   Input ~ 0
3V3
Text HLabel 3850 1000 0    50   Input ~ 0
3V3
Text HLabel 2200 1450 2    50   Input ~ 0
USB_C_VDD
Text HLabel 6450 1000 0    50   Input ~ 0
USB_C_VDD
Text HLabel 6350 2550 0    50   Input ~ 0
INT
Text HLabel 6600 1750 3    50   Input ~ 0
GND
Text HLabel 6600 3300 3    50   Input ~ 0
GND
Text HLabel 5000 1300 3    50   Input ~ 0
GND
Text HLabel 5400 1300 3    50   Input ~ 0
GND
Text HLabel 3550 2400 0    50   Input ~ 0
CHG_EN
Text HLabel 3550 2200 0    50   Input ~ 0
CHG_OK
Text HLabel 3550 2300 0    50   Input ~ 0
CHG_AC_OK
Text HLabel 4750 2600 2    50   Input ~ 0
BATT_VOLTAGE
Text HLabel 8300 4600 0    50   Input ~ 0
PWR_START
Text HLabel 8300 4750 0    50   Input ~ 0
PWR_SHUTDOWN
Text HLabel 1250 3500 2    50   Input ~ 0
USB_C_VDD
Text HLabel 4750 3300 2    50   Input ~ 0
BATT_TO_BUS_EN
Text Label 4750 2900 0    50   ~ 0
USB_VOLTAGE
Wire Wire Line
	4750 2800 5300 2800
Text Label 4750 2800 0    50   ~ 0
BUS_VOLTAGE
Text Label 6800 1400 0    50   ~ 0
USB_VOLTAGE
Text Label 6800 2900 0    50   ~ 0
BUS_VOLTAGE
Text HLabel 3550 3600 0    50   Input ~ 0
PWR_SHUTDOWN
Text HLabel 4750 3700 2    50   Input ~ 0
SW_RIGHT
Text HLabel 4750 3400 2    50   Input ~ 0
SW_LEFT
Text HLabel 4750 3600 2    50   Input ~ 0
SW_MID
NoConn ~ 3550 3800
NoConn ~ 4750 3800
NoConn ~ 4750 4100
Wire Wire Line
	4750 2900 5300 2900
NoConn ~ 3550 3700
Text HLabel 3550 2600 0    50   Input ~ 0
USB_TO_BUS_EN
NoConn ~ 3550 3900
NoConn ~ 3550 4000
NoConn ~ 3550 4100
$EndSCHEMATC
