![Header](media/header.jpg)

# RoFI - Metamorphic Distributed Robots

Project website: [https://rofi.fi.muni.cz/](https://rofi.fi.muni.cz/)

RoFI is a platform of modular robots developed at [Faculty of Informatics at
Masaryk University](https://fi.muni.cz/). The platform is designed for building
larger robots (RoFIbots) from a small number of module types.

The modules of the platform can connect together using RoFI dock. Mechanically
connected modules can communicate over TCP/IP and share power.

For an in-depth description of the RoFI platform see the [project
website](https://rofi.fi.muni.cz/).

## Repository Structure & Getting Started

If you are interested in building your modules or development of the platform,
please refer to the project documentation:
[https://paradise-fi.github.io/RoFI/](https://paradise-fi.github.io/RoFI/).

You will find all the necessary information about building and using the project
there. For a brief reminder of the compilation process, see:

```.bash
$ source setup.sh <Configuration> # Setup your shell for compiling the
                                  # project in a configuration
$ rcfg --list                     # Show available compilation suites
$ rcfg <suiteName>                # Configure given suite
$ rmake --all                     # Compile all configured suites or...
$ rmake <targetName>              # Compile given target or...
$ rmake tools/                    # Compile all configured targets from source prefix
```

## Licensing

See [licensing information](licence.md).


