# RoFICoM

![Connector Photo](../media/connector_photo.jpg)

RoFI Connection Mechanism, RoFICoM for short, is an open-hardware genderless
90-degree symmetric connection mechanism for modular robots. It can establish a
firm mechanical, data and power connection between two modules without any a
priori synchronization.

See the [connector page](https://rofi.fi.muni.cz/hardware/connector/) for more
details.

## Sources Organization

There are two directories:

- `hardware`: contains all the CAD files (Fusion 360 format) and PCBs (KiCAD
  format)
- `software`: contains the firmware for the connector. Currently, there are two
  versions of the firmware:
  - `AvrDock` for Arduino which should simulate the connector
  - `control_board` which runs on the actual connector






