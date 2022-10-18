add_rofi_firmware(dynamixelConfigurator.elf
    FLASH_SIZE 4MB
    SOURCES main.cpp)
target_link_libraries(dynamixelConfigurator.elf
    idf::esp32 idf::freertos idf::spi_flash idf::console esp32++ rofi::hal::esp32
    magic_enum libfort::fort)
set_partition_table(dynamixelConfigurator.elf partitions.csv)
