# Configuration

Library for configuration representation.

## File Format

The configuration file format is pretty straightforward. So far, it can
represent configuration containing only universal modules.

The file is a plain text file. You can use empty lines and line comments
(starting with `#`) to structure the file. Otherwise, each line of the file
defines either a new module or module connection:

- `M <id> <alpha> <beta> <gamma>` - definition of a module. `id` is an unique
  positive integer, `alpha`, `beta` and `gamma` are joint positions in degrees.
- `E <aId> <aShoe> <aConn> <orientation> <bConn> <bShoe> <bId>` - definition of
  a connection between modules. Shoe is either `A` or `B`, connector is `-Z`,
  `+X` or `-X`, orientation is `N`, `S`, `W` or `E`. See the [grid
  system](https://rofi.fi.muni.cz/platform/gridSystem/) for meaning of the shoe,
  connector and orientation.

See examples in the `../data` directory.

## Usage

The configuration is represented by the `Configuration` class. See its definiton
for more usage. The serialization/deserialization functions ara available in
`IO.h`.