cmake_minimum_required(VERSION 3.6)

add_rofi_firmware(hal_example_servo.elf
    FLASH_SIZE 4MB
    SOURCES main.cpp)
target_link_libraries(hal_example_servo.elf
    idf::esp32 idf::freertos idf::spi_flash esp32++ rofi::hal::esp32)
set_partition_table(hal_example_servo.elf partitions.csv)

