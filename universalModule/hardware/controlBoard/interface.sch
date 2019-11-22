EESchema Schematic File Version 4
LIBS:controlBoard-cache
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
L Connector_Generic:Conn_01x04 J5
U 1 1 5E8D0EF2
P 3300 2150
F 0 "J?" H 3380 2142 50  0000 L CNN
F 1 "MOTOR_BUS" H 3380 2051 50  0000 L CNN
F 2 "Connector_PinHeader_2.00mm:PinHeader_1x04_P2.00mm_Vertical" H 3300 2150 50  0001 C CNN
F 3 "~" H 3300 2150 50  0001 C CNN
	1    3300 2150
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J6
U 1 1 5E8D1385
P 3300 2700
F 0 "J?" H 3380 2742 50  0000 L CNN
F 1 "MOTOR_BUS_ALT" H 3380 2651 50  0000 L CNN
F 2 "Connector_PinHeader_2.00mm:PinHeader_1x03_P2.00mm_Vertical" H 3300 2700 50  0001 C CNN
F 3 "~" H 3300 2700 50  0001 C CNN
	1    3300 2700
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR045
U 1 1 5E8D1CC7
P 3100 2050
F 0 "#PWR?" H 3100 1800 50  0001 C CNN
F 1 "GND" V 3105 1922 50  0000 R CNN
F 2 "" H 3100 2050 50  0001 C CNN
F 3 "" H 3100 2050 50  0001 C CNN
	1    3100 2050
	0    1    1    0   
$EndComp
Text GLabel 3100 2150 0    50   Input ~ 0
BATT_VDD
Text GLabel 3100 2350 0    50   Input ~ 0
MOTORS_TX
Text GLabel 3100 2700 0    50   Input ~ 0
BATT_VDD
$Comp
L power:GND #PWR046
U 1 1 5E8D4C43
P 3100 2600
F 0 "#PWR?" H 3100 2350 50  0001 C CNN
F 1 "GND" V 3105 2472 50  0000 R CNN
F 2 "" H 3100 2600 50  0001 C CNN
F 3 "" H 3100 2600 50  0001 C CNN
	1    3100 2600
	0    1    1    0   
$EndComp
Text GLabel 3100 2800 0    50   Input ~ 0
MOTORS_TX
$Comp
L Connector_Generic:Conn_02x05_Odd_Even J4
U 1 1 5E8D7CD3
P 3250 4150
F 0 "J?" H 3300 4567 50  0000 C CNN
F 1 "ShoeAConnectors" H 3300 4476 50  0000 C CNN
F 2 "Connector_PinHeader_2.00mm:PinHeader_2x05_P2.00mm_Vertical" H 3250 4150 50  0001 C CNN
F 3 "~" H 3250 4150 50  0001 C CNN
	1    3250 4150
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR044
U 1 1 5E8D8579
P 3050 4350
F 0 "#PWR?" H 3050 4100 50  0001 C CNN
F 1 "GND" H 3055 4177 50  0000 C CNN
F 2 "" H 3050 4350 50  0001 C CNN
F 3 "" H 3050 4350 50  0001 C CNN
	1    3050 4350
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR047
U 1 1 5E8D8BD2
P 3550 4350
F 0 "#PWR?" H 3550 4100 50  0001 C CNN
F 1 "GND" H 3555 4177 50  0000 C CNN
F 2 "" H 3550 4350 50  0001 C CNN
F 3 "" H 3550 4350 50  0001 C CNN
	1    3550 4350
	1    0    0    -1  
$EndComp
Text GLabel 3050 3950 0    50   Input ~ 0
BATT_VDD
Text GLabel 2400 4050 0    50   Input ~ 0
INT
Wire Wire Line
	2400 4050 2450 4050
$Comp
L power:+12V #PWR043
U 1 1 5E8D94AA
P 2450 3950
F 0 "#PWR?" H 2450 3800 50  0001 C CNN
F 1 "+12V" H 2465 4123 50  0000 C CNN
F 2 "" H 2450 3950 50  0001 C CNN
F 3 "" H 2450 3950 50  0001 C CNN
	1    2450 3950
	1    0    0    -1  
$EndComp
Wire Wire Line
	2450 3950 2450 4050
Connection ~ 2450 4050
Wire Wire Line
	2450 4050 3050 4050
Text GLabel 3050 4150 0    50   Input ~ 0
EXT
Text GLabel 3050 4250 0    50   Input ~ 0
SPI_SCK
Text GLabel 3550 4250 2    50   Input ~ 0
SPI_MISO_MOSI
Text GLabel 3550 4150 2    50   Input ~ 0
DOCK_1_CE
Text GLabel 3550 4050 2    50   Input ~ 0
DOCK_2_CE
Text GLabel 3550 3950 2    50   Input ~ 0
DOCK_3_CE
$Comp
L Connector_Generic:Conn_01x13 J7
U 1 1 5E8E8B82
P 6850 3000
F 0 "J?" H 6930 3042 50  0000 L CNN
F 1 "ShoeBConnectors" H 6930 2951 50  0000 L CNN
F 2 "Connector_PinHeader_1.27mm:PinHeader_1x13_P1.27mm_Vertical" H 6850 3000 50  0001 C CNN
F 3 "~" H 6850 3000 50  0001 C CNN
	1    6850 3000
	1    0    0    -1  
$EndComp
Text GLabel 6650 2500 0    50   Input ~ 0
BATT_VDD
$Comp
L power:GND #PWR049
U 1 1 5E8E9AE7
P 6650 2400
F 0 "#PWR?" H 6650 2150 50  0001 C CNN
F 1 "GND" V 6655 2272 50  0000 R CNN
F 2 "" H 6650 2400 50  0001 C CNN
F 3 "" H 6650 2400 50  0001 C CNN
	1    6650 2400
	0    1    1    0   
$EndComp
Text GLabel 3100 2250 0    50   Input ~ 0
MOTORS_RX
Text GLabel 6650 2600 0    50   Input ~ 0
MOTORS_RX
Text GLabel 6650 2700 0    50   Input ~ 0
MOTORS_TX
Text GLabel 6650 2800 0    50   Input ~ 0
INT
Text GLabel 6650 2900 0    50   Input ~ 0
EXT
Text GLabel 6650 3000 0    50   Input ~ 0
SPI_SCK
Text GLabel 6650 3100 0    50   Input ~ 0
SPI_MISO_MOSI
Text GLabel 6650 3200 0    50   Input ~ 0
DOCK_4_CE
Text GLabel 6650 3300 0    50   Input ~ 0
DOCK_5_CE
Text GLabel 6650 3400 0    50   Input ~ 0
DOCK_6_CE
Wire Wire Line
	6650 3500 6550 3500
Wire Wire Line
	6550 3500 6550 3600
Wire Wire Line
	6550 3600 6650 3600
$Comp
L power:GND #PWR048
U 1 1 5E8F0432
P 6550 3600
F 0 "#PWR?" H 6550 3350 50  0001 C CNN
F 1 "GND" H 6555 3427 50  0000 C CNN
F 2 "" H 6550 3600 50  0001 C CNN
F 3 "" H 6550 3600 50  0001 C CNN
	1    6550 3600
	1    0    0    -1  
$EndComp
Connection ~ 6550 3600
$EndSCHEMATC
