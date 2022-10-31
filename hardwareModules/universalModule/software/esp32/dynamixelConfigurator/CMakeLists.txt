add_rofi_firmware(dynamixelConfigurator.elf
    FLASH_SIZE 4MB
    SOURCES main.cpp)
target_link_libraries(dynamixelConfigurator.elf
    idf::freertos idf::spi_flash idf::console rofi::hal
    magic_enum libfort::fort)
set_partition_table(dynamixelConfigurator.elf partitions.csv)
