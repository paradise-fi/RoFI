# Control Board Firwmware

## Dependencies

- `gcc-arm-none-eabi` supporting C++17
- `cmake` >= 3.9

For debugging and flashing the following is required:

- `arm-none-eabi-gdb` (on Ubuntu gdb multi arch with a symlink is required)
- `stlink` utility - [https://github.com/texane/stlink](https://github.com/texane/stlink)

## Building

There are several targets for make:

- `make {debug|release}` - build given version of firmware
- `make flash_{debug|release}` - flash given version
- `make logic` - build & run automated logic tests running on build machine
- `make native` - build & flash semi-automated tests running on the target