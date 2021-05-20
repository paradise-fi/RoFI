# RoFI HAL Python bindings

This example shows you how to use Python bindings for the RoFI HAL interface. It
allows you to control the modules in Gazebo or Simplesim simulation via Python
without a need to write any C++ code.

## How to run it:

First, make sure that you have setup the RoFI environment. Then start gazebo and
the script `oscilate.py`:

```
$ source setup.sh -f Debug              # Setup your terminal for compiling RoFI in Debug mode.
🤖 D $ rcfg desktop                     # Configure the desktop suite
🤖 D $ rmake --all                      # Build everything you have configured
🤖 D $ gazebo worlds/two_modules.world  # Run gazebo. We recommend using --verbose for more info
🤖 D $ python3 examples/pythonInterface/oscialte.py
```