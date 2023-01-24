add_rofi_firmware(umConsole.elf
    FLASH_SIZE 4MB
    SOURCES main.cpp)
target_link_libraries(umConsole.elf
    idf::freertos idf::spi_flash idf::console rofi::hal)
set_partition_table(umConsole.elf partitions.csv)
