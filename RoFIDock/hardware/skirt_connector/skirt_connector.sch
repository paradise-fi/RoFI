EESchema Schematic File Version 4
LIBS:skirt_connector-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
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
L Connector:Conn_01x08_Male J1
U 1 1 5C124CC6
P 5150 4150
F 0 "J1" H 5256 4628 50  0000 C CNN
F 1 "Cable" H 5256 4537 50  0000 C CNN
F 2 "rofi:skirt_8_pin" H 5150 4150 50  0001 C CNN
F 3 "~" H 5150 4150 50  0001 C CNN
	1    5150 4150
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x01_Male J2
U 1 1 5C124E33
P 7400 2200
F 0 "J2" H 7372 2130 50  0000 R CNN
F 1 "48V_A" H 7372 2221 50  0000 R CNN
F 2 "rofi:pogo_pin_2mm" H 7400 2200 50  0001 C CNN
F 3 "~" H 7400 2200 50  0001 C CNN
	1    7400 2200
	-1   0    0    1   
$EndComp
$Comp
L Connector:Conn_01x01_Female J3
U 1 1 5C124F14
P 7400 2350
F 0 "J3" H 7427 2376 50  0000 L CNN
F 1 "48V_1" H 7427 2285 50  0000 L CNN
F 2 "rofi:pogo_pad_2mm" H 7400 2350 50  0001 C CNN
F 3 "~" H 7400 2350 50  0001 C CNN
	1    7400 2350
	1    0    0    -1  
$EndComp
Wire Wire Line
	7200 2200 7150 2200
Wire Wire Line
	7150 2200 7150 2350
Wire Wire Line
	7150 2350 7200 2350
Wire Wire Line
	7150 2200 6950 2200
Connection ~ 7150 2200
Text GLabel 6950 2200 0    50   Input ~ 0
48V
$Comp
L Connector:Conn_01x01_Male J4
U 1 1 5C124FA9
P 7400 3050
F 0 "J4" H 7372 2980 50  0000 R CNN
F 1 "GND_A" H 7372 3071 50  0000 R CNN
F 2 "rofi:pogo_pin_2mm" H 7400 3050 50  0001 C CNN
F 3 "~" H 7400 3050 50  0001 C CNN
	1    7400 3050
	-1   0    0    1   
$EndComp
$Comp
L Connector:Conn_01x01_Female J5
U 1 1 5C124FD1
P 7400 3200
F 0 "J5" H 7427 3226 50  0000 L CNN
F 1 "GND_1" H 7427 3135 50  0000 L CNN
F 2 "rofi:pogo_pad_2mm" H 7400 3200 50  0001 C CNN
F 3 "~" H 7400 3200 50  0001 C CNN
	1    7400 3200
	1    0    0    -1  
$EndComp
Wire Wire Line
	7200 3200 7150 3200
Wire Wire Line
	7150 3200 7150 3050
Wire Wire Line
	7150 3050 7200 3050
Wire Wire Line
	7150 3050 6950 3050
Connection ~ 7150 3050
Text GLabel 6950 3050 0    50   Input ~ 0
GND
$Comp
L Connector:Conn_01x01_Male J6
U 1 1 5C1250C4
P 7400 3950
F 0 "J6" H 7372 3880 50  0000 R CNN
F 1 "RX_A" H 7372 3971 50  0000 R CNN
F 2 "rofi:pogo_pin_2mm" H 7400 3950 50  0001 C CNN
F 3 "~" H 7400 3950 50  0001 C CNN
	1    7400 3950
	-1   0    0    1   
$EndComp
Text GLabel 7150 3950 0    50   Input ~ 0
RX
Text GLabel 7150 4500 0    50   Input ~ 0
TX
$Comp
L Connector:Conn_01x01_Male J8
U 1 1 5C125264
P 5450 2200
F 0 "J8" H 5422 2130 50  0000 R CNN
F 1 "SENSE_A" H 5422 2221 50  0000 R CNN
F 2 "rofi:pogo_pin_2mm" H 5450 2200 50  0001 C CNN
F 3 "~" H 5450 2200 50  0001 C CNN
	1    5450 2200
	-1   0    0    1   
$EndComp
$Comp
L Connector:Conn_01x01_Male J9
U 1 1 5C12529A
P 5450 2400
F 0 "J9" H 5423 2330 50  0000 R CNN
F 1 "SENSE_B" H 5423 2421 50  0000 R CNN
F 2 "rofi:pogo_pin_2mm" H 5450 2400 50  0001 C CNN
F 3 "~" H 5450 2400 50  0001 C CNN
	1    5450 2400
	-1   0    0    1   
$EndComp
$Comp
L Connector:Conn_01x01_Female J10
U 1 1 5C1253F8
P 5450 2600
F 0 "J10" H 5477 2626 50  0000 L CNN
F 1 "SENSE_1" H 5477 2535 50  0000 L CNN
F 2 "rofi:pogo_pad_2mm" H 5450 2600 50  0001 C CNN
F 3 "~" H 5450 2600 50  0001 C CNN
	1    5450 2600
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x01_Female J11
U 1 1 5C125489
P 5450 2800
F 0 "J11" H 5477 2826 50  0000 L CNN
F 1 "SENSE_2" H 5477 2735 50  0000 L CNN
F 2 "rofi:pogo_pad_2mm" H 5450 2800 50  0001 C CNN
F 3 "~" H 5450 2800 50  0001 C CNN
	1    5450 2800
	1    0    0    -1  
$EndComp
Text GLabel 5250 2200 0    50   Input ~ 0
S_A_PIN
Text GLabel 5250 2400 0    50   Input ~ 0
S_B_PIN
Text GLabel 5250 2600 0    50   Input ~ 0
S_1_PAD
Text GLabel 5250 2800 0    50   Input ~ 0
S_2_PAD
Text GLabel 5350 4250 2    50   Input ~ 0
48V
Text GLabel 5350 4350 2    50   Input ~ 0
GND
Text GLabel 5350 3850 2    50   Input ~ 0
RX
Text GLabel 5350 4150 2    50   Input ~ 0
TX
Text GLabel 5350 3950 2    50   Input ~ 0
S_A_PIN
Text GLabel 5350 4550 2    50   Input ~ 0
S_B_PIN
Text GLabel 5350 4050 2    50   Input ~ 0
S_1_PAD
Text GLabel 5350 4450 2    50   Input ~ 0
S_2_PAD
$Comp
L Connector:Conn_01x01_Female J7
U 1 1 5C125879
P 7400 4500
F 0 "J7" H 7428 4526 50  0000 L CNN
F 1 "TX_1" H 7428 4435 50  0000 L CNN
F 2 "rofi:pogo_pad_2mm" H 7400 4500 50  0001 C CNN
F 3 "~" H 7400 4500 50  0001 C CNN
	1    7400 4500
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x01_Male J12
U 1 1 5C126036
P 7400 2600
F 0 "J12" H 7373 2530 50  0000 R CNN
F 1 "48V_B" H 7373 2621 50  0000 R CNN
F 2 "rofi:pogo_pin_2mm" H 7400 2600 50  0001 C CNN
F 3 "~" H 7400 2600 50  0001 C CNN
	1    7400 2600
	-1   0    0    1   
$EndComp
$Comp
L Connector:Conn_01x01_Female J13
U 1 1 5C12603C
P 7400 2750
F 0 "J13" H 7427 2776 50  0000 L CNN
F 1 "48V_2" H 7427 2685 50  0000 L CNN
F 2 "rofi:pogo_pad_2mm" H 7400 2750 50  0001 C CNN
F 3 "~" H 7400 2750 50  0001 C CNN
	1    7400 2750
	1    0    0    -1  
$EndComp
Wire Wire Line
	7150 2350 7150 2600
Wire Wire Line
	7150 2600 7200 2600
Connection ~ 7150 2350
Wire Wire Line
	7150 2600 7150 2750
Wire Wire Line
	7150 2750 7200 2750
Connection ~ 7150 2600
$Comp
L Connector:Conn_01x01_Male J14
U 1 1 5C126248
P 7400 3450
F 0 "J14" H 7373 3380 50  0000 R CNN
F 1 "GND_B" H 7373 3471 50  0000 R CNN
F 2 "rofi:pogo_pin_2mm" H 7400 3450 50  0001 C CNN
F 3 "~" H 7400 3450 50  0001 C CNN
	1    7400 3450
	-1   0    0    1   
$EndComp
$Comp
L Connector:Conn_01x01_Female J15
U 1 1 5C12624E
P 7400 3600
F 0 "J15" H 7427 3626 50  0000 L CNN
F 1 "GND_2" H 7427 3535 50  0000 L CNN
F 2 "rofi:pogo_pad_2mm" H 7400 3600 50  0001 C CNN
F 3 "~" H 7400 3600 50  0001 C CNN
	1    7400 3600
	1    0    0    -1  
$EndComp
Wire Wire Line
	7200 3600 7150 3600
Wire Wire Line
	7150 3450 7200 3450
Wire Wire Line
	7150 3200 7150 3450
Connection ~ 7150 3200
Wire Wire Line
	7150 3450 7150 3600
Connection ~ 7150 3450
$Comp
L Connector:Conn_01x01_Male J16
U 1 1 5C126914
P 7400 4150
F 0 "J16" H 7373 4080 50  0000 R CNN
F 1 "RX_B" H 7373 4171 50  0000 R CNN
F 2 "rofi:pogo_pin_2mm" H 7400 4150 50  0001 C CNN
F 3 "~" H 7400 4150 50  0001 C CNN
	1    7400 4150
	-1   0    0    1   
$EndComp
Wire Wire Line
	7200 3950 7150 3950
Wire Wire Line
	7200 3950 7200 4150
Connection ~ 7200 3950
$Comp
L Connector:Conn_01x01_Female J17
U 1 1 5C127090
P 7400 4700
F 0 "J17" H 7428 4726 50  0000 L CNN
F 1 "TX_2" H 7428 4635 50  0000 L CNN
F 2 "rofi:pogo_pad_2mm" H 7400 4700 50  0001 C CNN
F 3 "~" H 7400 4700 50  0001 C CNN
	1    7400 4700
	1    0    0    -1  
$EndComp
Wire Wire Line
	7150 4500 7200 4500
Wire Wire Line
	7200 4500 7200 4700
Connection ~ 7200 4500
$EndSCHEMATC
