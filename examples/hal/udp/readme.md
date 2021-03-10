# UDB Communication Example

The example shows simple UDP communication example using the lwIP raw API.

To run the example, use following setup:

Wire two ESP32 DevKitCs to provides SPI bus:

- MOSI 5
- MISO 18
- SCK 19

There should be one RoFI dock connected on each bus with CS 14. The two docks
should be connected. The ESP32 should run the same firmware.

Note that the detection of server/client is based on MAC address and it might be needed to modify the decision procedure according to your modules.
