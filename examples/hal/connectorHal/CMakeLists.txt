cmake_minimum_required(VERSION 3.6)

add_rofi_firmware(hal_connector_demo.elf
    FLASH_SIZE 4MB
    SOURCES main.cpp)
target_link_libraries(hal_connector_demo.elf
    idf::freertos idf::spi_flash rofi::hal::esp32_jig)
set_partition_table(hal_connector_demo.elf partitions.csv)
