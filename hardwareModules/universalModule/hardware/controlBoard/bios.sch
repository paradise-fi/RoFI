EESchema Schematic File Version 4
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
Text GLabel 3550 2900 0    50   Input ~ 0
USB_CLK
Text GLabel 3550 3100 0    50   Input ~ 0
USB_SUSPEND
Text HLabel 4150 4500 3    50   Input ~ 0
GND
Text HLabel 3850 1000 0    50   Input ~ 0
3V3
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
Text HLabel 4750 3300 2    50   Input ~ 0
BATT_TO_BUS_EN
Text Label 4750 2900 0    50   ~ 0
USB_VOLTAGE
Wire Wire Line
	4750 2800 5300 2800
Text Label 4750 2800 0    50   ~ 0
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
$EndSCHEMATC
