# The Configuration 2.0

Library for the new configuration representation. The new
configuration is not limited to the universal module only
so that we are able to describe various module shapes and
sizes.

Currently, the user can create and work with the whole
RoFIbot, which is represented by the class `Rofibot`. The
bot consists of one or more modules. A module is represented
by its own class called `Module`.

## Making custom module

There is a convention of providing a function for creating
a module starting with `build` (i.e., `buildUniversalModule`,
`buildPad`).

The module is required to contain its components – all parts
that it is consisted of, where at first are specified all of its
connectors (i.e., RoFICoMs) and then the other parts (i.e., body
components, shoes, etc.) -- and joints that represent all
connections between its components. The last required parameter
is the number of connectors. There is also an optional parameter
called `rootComponent`, with which you can specify the original
component within the module and save execution of some
computations when working with the module.

For more details and concrete examples, please consult files with
our defined modules `Universal Module` in `universalModule.cpp`
and `Pad` in `pad.cpp`.

## File Format

The new configuration is compatible with the old file format
(see [below](#old-format)). The new file format is yet to be
determined.

* * *

# The Original Configuration

Library for the old configuration representation.

## File Format {#old-format}

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