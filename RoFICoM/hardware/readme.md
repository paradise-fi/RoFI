# RoFiCoM Hardware

- `connector.f3z` - CAD drawing of connector (Fusion 360)
- `control_board` - PCB for the connector
- `debug_interface` - interface between connectors used on the control board and
  pin headers
- `pogo_pad` - counterpart for the pogo based connector on the connector
- `skirt_connector` - connector-to-connector interface (mounted on top the
  connector's skirt)

## Building

The provided makefile exports all Gerber files for PCB manufacturing as zip
files.