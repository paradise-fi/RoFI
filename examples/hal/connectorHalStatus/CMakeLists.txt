cmake_minimum_required(VERSION 3.6)

add_rofi_firmware(hal_example_connector_status.elf
    FLASH_SIZE 4MB
    SOURCES main.cpp)
target_link_libraries(hal_example_connector_status.elf
    idf::freertos idf::spi_flash rofi::hal::esp32_jig)
set_partition_table(hal_example_connector_status.elf partitions.csv)
