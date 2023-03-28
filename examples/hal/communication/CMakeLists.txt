cmake_minimum_required(VERSION 3.11)

set(EXTRA_COMPONENT_DIRS "../../RoFIDriver")

add_rofi_firmware(communication.elf
    FLASH_SIZE 4MB
    SOURCES communication.cpp)
target_link_libraries(communication.elf
    idf::freertos idf::spi_flash rofi::hal networking)
