# AvrDock

AvrDock is a prototype implementation of the RoFI Docking System. The prototype
does not feature any mechanical hardware; it only provides firmware for ATMega
328p (Arduino Nano). This setup is used to prove a concept of the inter-module
communication in the RoFI systems, which is implementing a custom physical layer
for a traditional TCP/IP stack.

## Limitations

The AvrDock is limited by the ATMega microcontroller. Compared to the proposal;
there are the following limitations:
- SPI clock frequency is limited to 100 kHz,
- dock-to-dock communication is limited to 250 kBaud/s,
- maximal payload per packet is 128 bytes and
- the implementation ignores any commands related to a mechanical construction (e.g., sensing the power lines).

## How to use it

The firmware is distributed as a PlatformIO project; therefore PlatformIO is the
only dependency. The pinout for Arduino Nano is as follows:

The dock-to-dock interface:
- pin 0 is TX for the mating dock,
- pin 1 is RX for the mating dock.

The control unit interface:
- pin 10 is CS for the SPI interface,
- pin 11 is MOSI
- pin 12 MISO
- pin 13 SCK
