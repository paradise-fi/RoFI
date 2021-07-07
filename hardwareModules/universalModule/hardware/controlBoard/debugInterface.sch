EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 6 14
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
L Connector_Generic:Conn_01x10 J?
U 1 1 617A288F
P 6300 4100
AR Path="/5E080FD6/617A288F" Ref="J?"  Part="1" 
AR Path="/6179C49B/617A288F" Ref="J21"  Part="1" 
F 0 "J21" H 6218 3375 50  0000 C CNN
F 1 "DEBUG" H 6218 3466 50  0000 C CNN
F 2 "Connector_FFC-FPC:Hirose_FH12-10S-0.5SH_1x10-1MP_P0.50mm_Horizontal" H 6300 4100 50  0001 C CNN
F 3 "~" H 6300 4100 50  0001 C CNN
F 4 "FH12-10S-0.5SH(55)" H 6300 4100 50  0001 C CNN "MFR"
F 5 "1" H 6300 4100 50  0001 C CNN "MFR_QTY"
F 6 "YES" H 6300 4100 50  0001 C CNN "JLCPCB_IGNORE"
	1    6300 4100
	1    0    0    1   
$EndComp
Text Label 6100 4500 2    50   ~ 0
GND
Text Label 6100 4400 2    50   ~ 0
3V3
Text Label 6100 4300 2    50   ~ 0
SWRST
Text Label 6100 4200 2    50   ~ 0
SWDIO
Text Label 6100 4100 2    50   ~ 0
SWCLK
Text Label 6100 4000 2    50   ~ 0
ESP_TDI
Text Label 6100 3900 2    50   ~ 0
ESP_TDO
Text Label 6100 3800 2    50   ~ 0
ESP_TCK
Text Label 6100 3700 2    50   ~ 0
ESP_TMS
Text Label 6100 3600 2    50   ~ 0
ESP_EN
$Comp
L Connector_Generic:Conn_02x10_Odd_Even J18
U 1 1 617A4FAB
P 4850 4000
F 0 "J18" H 4900 4617 50  0000 C CNN
F 1 "Conn_02x10_Odd_Even" H 4900 4526 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_2x10_P2.54mm_Vertical" H 4850 4000 50  0001 C CNN
F 3 "~" H 4850 4000 50  0001 C CNN
F 4 "YES" H 4850 4000 50  0001 C CNN "JLCPCB_IGNORE"
	1    4850 4000
	1    0    0    -1  
$EndComp
Wire Wire Line
	5150 3600 6100 3600
Wire Wire Line
	5150 3700 6100 3700
Wire Wire Line
	6100 3800 5150 3800
Wire Wire Line
	5150 3900 6100 3900
Wire Wire Line
	6100 4000 5150 4000
Wire Wire Line
	5150 4100 6100 4100
Wire Wire Line
	6100 4200 5150 4200
Wire Wire Line
	5150 4300 6100 4300
Wire Wire Line
	6100 4400 5150 4400
Wire Wire Line
	5150 4500 6100 4500
Wire Wire Line
	4650 3600 4300 3600
Wire Wire Line
	4650 3700 4300 3700
Wire Wire Line
	4650 3800 4300 3800
Wire Wire Line
	4650 3900 4300 3900
Wire Wire Line
	4650 4000 4300 4000
Wire Wire Line
	4650 4100 4300 4100
Wire Wire Line
	4650 4200 4300 4200
Wire Wire Line
	4650 4300 4300 4300
Wire Wire Line
	4650 4400 4300 4400
Wire Wire Line
	4650 4500 4300 4500
Text Label 4300 4500 2    50   ~ 0
GND
Text Label 4300 4400 2    50   ~ 0
3V3
Text Label 4300 4300 2    50   ~ 0
SWRST
Text Label 4300 4200 2    50   ~ 0
SWDIO
Text Label 4300 4100 2    50   ~ 0
SWCLK
Text Label 4300 4000 2    50   ~ 0
ESP_TDI
Text Label 4300 3900 2    50   ~ 0
ESP_TDO
Text Label 4300 3800 2    50   ~ 0
ESP_TCK
Text Label 4300 3700 2    50   ~ 0
ESP_TMS
Text Label 4300 3600 2    50   ~ 0
ESP_EN
$Comp
L Connector_Generic:Conn_02x05_Odd_Even J16
U 1 1 617AD537
P 4300 1800
F 0 "J16" H 4350 2217 50  0000 C CNN
F 1 "ST-LINK A" H 4350 2126 50  0000 C CNN
F 2 "Connector_IDC:IDC-Header_2x05_P2.54mm_Vertical" H 4300 1800 50  0001 C CNN
F 3 "~" H 4300 1800 50  0001 C CNN
F 4 "75869-101LF" H 4300 1800 50  0001 C CNN "MFR"
F 5 "1" H 4300 1800 50  0001 C CNN "MFR_QTY"
F 6 "YES" H 4300 1800 50  0001 C CNN "JLCPCB_IGNORE"
	1    4300 1800
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_02x05_Odd_Even J17
U 1 1 617AE7AE
P 4300 2650
F 0 "J17" H 4350 3067 50  0000 C CNN
F 1 "ST-LINK_B" H 4350 2976 50  0000 C CNN
F 2 "Connector_IDC:IDC-Header_2x05_P2.54mm_Vertical" H 4300 2650 50  0001 C CNN
F 3 "~" H 4300 2650 50  0001 C CNN
F 4 "75869-101LF" H 4300 2650 50  0001 C CNN "MFR"
F 5 "1" H 4300 2650 50  0001 C CNN "MFR_QTY"
F 6 "YES" H 4300 2650 50  0001 C CNN "JLCPCB_IGNORE"
	1    4300 2650
	1    0    0    -1  
$EndComp
Text Label 4100 1600 2    50   ~ 0
SWRST
Text Label 4100 1800 2    50   ~ 0
GND
Text Label 4100 1900 2    50   ~ 0
3V3
Text Label 4600 1900 0    50   ~ 0
3V3
Text Label 4600 1800 0    50   ~ 0
GND
Text Label 4600 1700 0    50   ~ 0
SWDIO
Text Label 4600 1600 0    50   ~ 0
SWCLK
NoConn ~ 4100 1700
NoConn ~ 4600 2000
NoConn ~ 4100 2000
Text Label 4100 2450 2    50   ~ 0
SWRST
Text Label 4600 2450 0    50   ~ 0
SWDIO
Text Label 4600 2550 0    50   ~ 0
GND
Text Label 4100 2550 2    50   ~ 0
GND
Text Label 4600 2650 0    50   ~ 0
SWCLK
Text Label 4600 2750 0    50   ~ 0
3V3
Text Label 4100 2750 2    50   ~ 0
3V3
NoConn ~ 4100 2650
NoConn ~ 4100 2850
NoConn ~ 4600 2850
Text Notes 3750 1300 0    50   ~ 0
There are two chineese ST-Links\nwith two pinouts
Text Label 6650 1850 0    50   ~ 0
SWCLK
Text Label 6650 1950 0    50   ~ 0
SWDIO
NoConn ~ 6650 2150
Text Label 6150 1450 0    50   ~ 0
3V3
Text Label 6650 1650 0    50   ~ 0
SWRST
$Comp
L Connector:Conn_ST_STDC14 J20
U 1 1 617C19F2
P 6150 2150
F 0 "J20" H 5707 2196 50  0000 R CNN
F 1 "Conn_ST_STDC14" H 5707 2105 50  0000 R CNN
F 2 "Connector_PinSocket_1.27mm:PinSocket_2x07_P1.27mm_Vertical_SMD" H 6150 2150 50  0001 C CNN
F 3 "https://www.st.com/content/ccc/resource/technical/document/user_manual/group1/99/49/91/b6/b2/3a/46/e5/DM00526767/files/DM00526767.pdf/jcr:content/translations/en.DM00526767.pdf" V 5800 900 50  0001 C CNN
F 4 "FTSH-107-01-L-DV-K-A" H 6150 2150 50  0001 C CNN "MFR"
F 5 "1" H 6150 2150 50  0001 C CNN "MFR_QTY"
F 6 "YES" H 6150 2150 50  0001 C CNN "JLCPCB_IGNORE"
	1    6150 2150
	1    0    0    -1  
$EndComp
NoConn ~ 6650 2550
NoConn ~ 6650 2650
Wire Wire Line
	6050 2850 6050 2950
Wire Wire Line
	6050 2950 6150 2950
Wire Wire Line
	6150 2950 6150 2850
Text Label 6150 2950 0    50   ~ 0
GND
NoConn ~ 6650 2350
NoConn ~ 6650 2050
$Comp
L Connector_Generic:Conn_02x05_Odd_Even J19
U 1 1 617C9EC3
P 5250 6050
F 0 "J19" H 5300 6467 50  0000 C CNN
F 1 "ESP-DBG" H 5300 6376 50  0000 C CNN
F 2 "Connector_IDC:IDC-Header_2x05_P2.54mm_Vertical" H 5250 6050 50  0001 C CNN
F 3 "~" H 5250 6050 50  0001 C CNN
F 4 "75869-101LF" H 5250 6050 50  0001 C CNN "MFR"
F 5 "1" H 5250 6050 50  0001 C CNN "MFR_QTY"
F 6 "YES" H 5250 6050 50  0001 C CNN "JLCPCB_IGNORE"
	1    5250 6050
	1    0    0    -1  
$EndComp
Text Label 5050 5850 2    50   ~ 0
3V3
Text Label 5050 5950 2    50   ~ 0
GND
Text Label 5050 6050 2    50   ~ 0
GND
Text Label 5050 6150 2    50   ~ 0
GND
Text Label 5050 6250 2    50   ~ 0
GND
NoConn ~ 5550 6250
Text Label 5550 6150 0    50   ~ 0
ESP_TDI
Text Label 5550 6050 0    50   ~ 0
ESP_TDO
Text Label 5550 5950 0    50   ~ 0
ESP_TCK
Text Label 5550 5850 0    50   ~ 0
ESP_TMS
$Comp
L power:PWR_FLAG #FLG0105
U 1 1 61B5B847
P 6050 2950
F 0 "#FLG0105" H 6050 3025 50  0001 C CNN
F 1 "PWR_FLAG" H 6050 3123 50  0000 C CNN
F 2 "" H 6050 2950 50  0001 C CNN
F 3 "~" H 6050 2950 50  0001 C CNN
	1    6050 2950
	-1   0    0    1   
$EndComp
Connection ~ 6050 2950
$Comp
L power:PWR_FLAG #FLG0108
U 1 1 61B5BD53
P 6150 1250
F 0 "#FLG0108" H 6150 1325 50  0001 C CNN
F 1 "PWR_FLAG" H 6150 1423 50  0000 C CNN
F 2 "" H 6150 1250 50  0001 C CNN
F 3 "~" H 6150 1250 50  0001 C CNN
	1    6150 1250
	1    0    0    -1  
$EndComp
Wire Wire Line
	6150 1250 6150 1450
$EndSCHEMATC
