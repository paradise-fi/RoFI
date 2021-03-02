# Dock to Dock Communication Example

The example shows simple point-to-point communication using the dock driver.

To run the example, use following setup:

Wire two ESP32 DevKitCs to provides SPI bus:

- MOSI 5
- MISO 18
- SCK 19

There should be one RoFI dock connected on each bus with CS 14. The two docks
should be connected. The ESP32 should run the same firmware.
