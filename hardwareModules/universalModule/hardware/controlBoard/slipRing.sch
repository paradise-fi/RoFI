EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 8 14
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
L Connector_Generic:Conn_01x40 J?
U 1 1 619838EC
P 5200 3850
AR Path="/5E080FD6/619838EC" Ref="J?"  Part="1" 
AR Path="/619227C7/619838EC" Ref="J?"  Part="1" 
AR Path="/61981EC7/619838EC" Ref="J13"  Part="1" 
F 0 "J13" H 5118 5967 50  0000 C CNN
F 1 "SLIP_RING_OUT" H 5118 5876 50  0000 C CNN
F 2 "Connector_FFC-FPC:Hirose_FH12-40S-0.5SH_1x40-1MP_P0.50mm_Horizontal" H 5200 3850 50  0001 C CNN
F 3 "~" H 5200 3850 50  0001 C CNN
F 4 "FH12-40S-0.5SH(55)" H 5200 3850 50  0001 C CNN "MFR"
F 5 "1" H 5200 3850 50  0001 C CNN "MFR_QTY"
F 6 "YES" H 5200 3850 50  0001 C CNN "JLCPCB_IGNORE"
	1    5200 3850
	-1   0    0    -1  
$EndComp
Text Label 5400 4550 0    50   ~ 0
EXT
Text Label 5400 4650 0    50   ~ 0
EXT
Text Label 5400 4750 0    50   ~ 0
EXT
Text Label 5400 4450 0    50   ~ 0
EXT
Text Label 5400 2050 0    50   ~ 0
SPI_SCK
Text Label 5400 1950 0    50   ~ 0
GND
Text Label 5400 2150 0    50   ~ 0
GND
Text Label 5400 2250 0    50   ~ 0
SPI_MISO_MOSI
Text Label 5400 2350 0    50   ~ 0
GND
Text Label 5400 2550 0    50   ~ 0
DOCK_2_CE
Text Label 5400 2650 0    50   ~ 0
DOCK_3_CE
Text Label 5400 2750 0    50   ~ 0
GND
Text Label 5400 2850 0    50   ~ 0
MOTORS_TX
Text Label 5400 3050 0    50   ~ 0
GND
Text Label 5400 3150 0    50   ~ 0
GND
Text Label 5400 3250 0    50   ~ 0
BATT_VDD
Text Label 5400 3350 0    50   ~ 0
BATT_VDD
Text Label 5400 3450 0    50   ~ 0
BATT_VDD
Text Label 5400 3550 0    50   ~ 0
BATT_VDD
Text Label 5400 3650 0    50   ~ 0
BATT_VDD
Text Label 5400 3750 0    50   ~ 0
BATT_VDD
Text Label 5400 3850 0    50   ~ 0
BATT_VDD
Text Label 5400 3950 0    50   ~ 0
BATT_VDD
Text Label 5400 4050 0    50   ~ 0
INT
Text Label 5400 4150 0    50   ~ 0
INT
Text Label 5400 4250 0    50   ~ 0
INT
Text Label 5400 4350 0    50   ~ 0
INT
Text Label 5400 4850 0    50   ~ 0
GND
Text Label 5400 4950 0    50   ~ 0
GND
Text Label 5400 5050 0    50   ~ 0
GND
Text Label 5400 5150 0    50   ~ 0
GND
Text Label 5400 5250 0    50   ~ 0
GND
Text Label 5400 5350 0    50   ~ 0
GND
Text Label 5400 5450 0    50   ~ 0
GND
Text Label 5400 5550 0    50   ~ 0
GND
Text Label 5400 5650 0    50   ~ 0
GND
Text Label 5400 5750 0    50   ~ 0
GND
Text Label 5400 5850 0    50   ~ 0
GND
Text Label 5400 2950 0    50   ~ 0
MOTORS_RX
$Comp
L rofi:Servo U?
U 1 1 61987323
P 4300 3750
AR Path="/61987323" Ref="U?"  Part="1" 
AR Path="/61981EC7/61987323" Ref="U3"  Part="1" 
F 0 "U3" H 4478 3796 50  0000 L CNN
F 1 "Servo" H 4478 3705 50  0000 L CNN
F 2 "rofi:Dynamixel-XL-430-TOP" H 4300 3750 50  0001 C CNN
F 3 "" H 4300 3750 50  0001 C CNN
F 4 "YES" H 4300 3750 50  0001 C CNN "JLCPCB_IGNORE"
	1    4300 3750
	1    0    0    -1  
$EndComp
Wire Wire Line
	7300 4600 7100 4600
$Comp
L Connector_Generic:Conn_01x15 J?
U 1 1 619965FD
P 7500 4000
AR Path="/5E080FD6/619965FD" Ref="J?"  Part="1" 
AR Path="/61981EC7/619965FD" Ref="J14"  Part="1" 
F 0 "J14" H 7450 4950 50  0000 L CNN
F 1 "REFERENCE_PINS" H 7250 4850 50  0000 L CNN
F 2 "rofi:SlipRing-120220-0161" H 7500 4000 50  0001 C CNN
F 3 "~" H 7500 4000 50  0001 C CNN
F 4 "YES" H 7500 4000 50  0001 C CNN "JLCPCB_IGNORE"
	1    7500 4000
	1    0    0    1   
$EndComp
Text Label 7100 4600 0    50   ~ 0
EXT
Text Label 7300 3400 2    50   ~ 0
SPI_SCK
Text Label 7300 3300 2    50   ~ 0
GND
Text Label 7300 3500 2    50   ~ 0
GND
Text Label 7300 3600 2    50   ~ 0
SPI_MISO_MOSI
Text Label 7300 4000 2    50   ~ 0
GND
Text Label 7300 4100 2    50   ~ 0
BATT_VDD
Text Label 7300 4200 2    50   ~ 0
BATT_VDD
Text Label 7300 4300 2    50   ~ 0
MOTORS_RX
Text Label 7300 4400 2    50   ~ 0
MOTORS_TX
Text Label 7300 4500 2    50   ~ 0
INT
Text Label 7300 4700 2    50   ~ 0
GND
Text Label 5400 2450 0    50   ~ 0
DOCK_1_CE
Text Label 7300 3700 2    50   ~ 0
DOCK_1_CE
Text Label 7300 3800 2    50   ~ 0
DOCK_2_CE
Text Label 7300 3900 2    50   ~ 0
DOCK_3_CE
$EndSCHEMATC
